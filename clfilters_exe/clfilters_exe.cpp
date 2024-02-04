#include <memory>
#include <iostream>
#include "rgy_osdep.h"
#include "rgy_tchar.h"
#include "rgy_log.h"
#include "rgy_event.h"
#include "rgy_shared_mem.h"
#include "clfilters_shared.h"
#include "clfilters_chain.h"
#include "clfilters_chain_prm.h"
#include "clfilters_exe_cmd.h"
#include "clfilters_version.h"
#include "rgy_util.h"
#include "rgy_opencl.h"
#include "rgy_cmd.h"

//---------------------------------------------------------------------
//        ラベル
//---------------------------------------------------------------------
#if !CLFILTERS_EN
static const char *LB_WND_OPENCL_UNAVAIL = "フィルタは無効です: OpenCLを使用できません。";
static const char *LB_WND_OPENCL_AVAIL = "OpenCL 有効";
static const char *LB_CX_OPENCL_DEVICE = "デバイス選択";
#else
static const char *LB_WND_OPENCL_UNAVAIL = "Filter disabled, OpenCL could not be used.";
static const char *LB_WND_OPENCL_AVAIL = "OpenCL Enabled";
static const char *LB_CX_OPENCL_DEVICE = "Device";
#endif

class clFiltersExe {
public:
    clFiltersExe();
    ~clFiltersExe();
    int init(AviutlAufExeParams& prms);
    int run();
    void AddMessage(RGYLogLevel log_level, const tstring &str) {
        if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_APP)) {
            return;
        }
        if (log_level >= RGY_LOG_ERROR && m_sharedMessage && m_sharedMessage->is_open()) {
            strcat_s(getMessagePtr()->data, tchar_to_string(str).c_str());
        }
        auto lines = split(str, _T("\n"));
        for (const auto &line : lines) {
            if (line[0] != _T('\0')) {
                m_log->write(log_level, RGY_LOGT_APP, (_T("clfilters[exe]: ") + line + _T("\n")).c_str());
            }
        }
    }
    void AddMessage(RGYLogLevel log_level, const TCHAR *format, ...) {
        va_list args;
        va_start(args, format);
        int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
        tstring buffer;
        buffer.resize(len, _T('\0'));
        _vstprintf_s(&buffer[0], len, format, args);
        va_end(args);
        AddMessage(log_level, buffer);
    }
protected:
    void checkClPlatforms();
    int funcProc();
    clfitersSharedMesData *getMessagePtr() { return (clfitersSharedMesData*)m_sharedMessage->ptr(); }
    RGYFrameInfo setFrameInfo(const int iframeID, const int width, const int height, void *frame);
    std::unique_ptr<clFilterChain> m_clfilter;
    std::vector<std::shared_ptr<RGYOpenCLPlatform>> m_clplatforms;
    std::unique_ptr<std::remove_pointer<HANDLE>::type, handle_deleter> m_aviutlHandle;
    HANDLE m_eventMesStart;
    HANDLE m_eventMesEnd;
    std::unique_ptr<RGYSharedMemWin> m_sharedMessage;
    std::unique_ptr<RGYSharedMemWin> m_sharedPrms;
    std::vector<std::unique_ptr<RGYSharedMemWin>> m_sharedFramesIn;
    std::unique_ptr<RGYSharedMemWin> m_sharedFramesOut;
    size_t m_ppid;
    int m_maxWidth;
    int m_maxHeight;
    int m_pitchBytes;
    std::shared_ptr<RGYLog> m_log;
};

clFiltersExe::clFiltersExe() :
    m_clfilter(),
    m_clplatforms(),
    m_aviutlHandle(),
    m_eventMesStart(nullptr),
    m_eventMesEnd(nullptr),
    m_sharedMessage(),
    m_sharedPrms(),
    m_sharedFramesIn(),
    m_sharedFramesOut(),
    m_ppid(0),
    m_maxWidth(0),
    m_maxHeight(0),
    m_pitchBytes(0),
    m_log() { }
clFiltersExe::~clFiltersExe() { }

int clFiltersExe::init(AviutlAufExeParams& prms) {
    m_log = std::make_shared<RGYLog>(prms.logfile.c_str(), prms.log_level);
    m_eventMesStart = prms.eventMesStart;
    m_eventMesEnd = prms.eventMesEnd;
    WaitForSingleObject(m_eventMesStart, INFINITE);

    m_ppid = prms.ppid;
    m_aviutlHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, handle_deleter>(OpenProcess(SYNCHRONIZE | PROCESS_VM_READ, FALSE, prms.ppid), handle_deleter());
    m_sharedMessage = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_MESSAGE, prms.ppid).c_str(), sizeof(clfitersSharedMesData));
    m_sharedPrms = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_PRMS, prms.ppid).c_str(), sizeof(clfitersSharedPrms));
    m_maxWidth = prms.max_w;
    m_maxHeight = prms.max_h;
    m_pitchBytes = get_shared_frame_pitch(m_maxWidth);

    const int frameSize = m_pitchBytes * m_maxHeight;
    for (int i = 0; i < 3; i++) {
        m_sharedFramesIn.push_back(std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_FRAMES_IN, m_ppid, i).c_str(), frameSize));
    }
    m_sharedFramesOut = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_FRAMES_OUT, m_ppid).c_str(), frameSize);
    checkClPlatforms();
    // プロセス初期化処理の終了を通知
    SetEvent(m_eventMesEnd);
    return 0;
}

void clFiltersExe::checkClPlatforms() {
    if (m_clplatforms.size() == 0) {
        RGYOpenCL cl(m_log);
        m_clplatforms = cl.getPlatforms(nullptr);
        for (auto& platform : m_clplatforms) {
            platform->createDeviceList(CL_DEVICE_TYPE_GPU);
        }
    }
    std::string devices;
    for (size_t ip = 0; ip < m_clplatforms.size(); ip++) {
        for (int idev = 0; idev < (int)m_clplatforms[ip]->devs().size(); idev++) {
            CL_PLATFORM_DEVICE pd;
            pd.s.platform = (int16_t)ip;
            pd.s.device = (int16_t)idev;
            const auto devInfo = m_clplatforms[ip]->dev(idev).info();
            auto devName = (devInfo.board_name_amd.length() > 0) ? devInfo.board_name_amd : devInfo.name;
            devName = str_replace(devName, "(TM)", "");
            devName = str_replace(devName, "(R)", "");
            devName = str_replace(devName, "  ", " ");
            devices += strsprintf("%x/%s\n", pd.i, devName.c_str());
        }
    }
    auto mes = getMessagePtr();
    strcpy_s(mes->data, devices.c_str());
}

RGYFrameInfo clFiltersExe::setFrameInfo(const int iframeID, const int width, const int height, void *frame) {
    RGYFrameInfo in;
    //入力フレーム情報
    in.width = width;
    in.height = height;
    in.pitch[0] = m_pitchBytes;
    in.csp = RGY_CSP_YC48;
    in.picstruct = RGY_PICSTRUCT_FRAME;
    in.ptr[0] = (uint8_t *)frame;
    in.mem_type = RGY_MEM_TYPE_CPU;
    in.inputFrameId = iframeID;
    return in;
}

int clFiltersExe::funcProc() {
    // エラーメッセージ用の領域を初期化
    getMessagePtr()->data[0] = '\0';

    clFilterChainParam prm;
    auto sharedPrms = (clfitersSharedPrms *)m_sharedPrms->ptr();
    const auto dev_pd = sharedPrms->pd;
    const auto current_frame = sharedPrms->currentFrameId;
    const auto frame_n = sharedPrms->frame_n;
    const auto resetPipeline = sharedPrms->resetPipeLine;
    auto is_saving = sharedPrms->is_saving;
    // どのフレームから処理を開始すべきか?
    auto frameIn = sharedPrms->frameIn;
    auto frameProc = sharedPrms->frameProc;
    //auto frameOut = sharedPrms->frameOut;
    prm.setPrmFromCmd(char_to_tstring(sharedPrms->prms));
    if (!m_clfilter
        || m_clfilter->platformID() != dev_pd.s.platform
        || m_clfilter->deviceID()   != dev_pd.s.device) {
        int ret = 0;
        m_clfilter = std::make_unique<clFilterChain>();
        std::string mes = AUF_FULL_NAME;
        mes += ": ";
        if (m_clfilter->init(dev_pd.s.platform, dev_pd.s.device, CL_DEVICE_TYPE_GPU, prm.log_level.get(RGY_LOGT_APP), prm.log_to_file)) {
            mes += LB_WND_OPENCL_UNAVAIL;
            getMessagePtr()->ret = ret;
            strcpy_s(getMessagePtr()->data, mes.c_str());
            return FALSE;
        }
        is_saving = FALSE;
    }

    // 保存モード(is_saving=true)の時に、どのくらい先まで処理をしておくべきか?
    static_assert(frameInOffset < clFilterFrameBuffer::bufSize);
    static_assert(frameInOffset < _countof(clfitersSharedPrms::srcFrame));
    if (frame_n <= 0 || current_frame < 0) {
        return TRUE; // 何もしない
    }
    if (resetPipeline
        || m_clfilter->getNextOutFrameId() != current_frame) { // 出てくる予定のフレームがずれていたらリセット
        m_clfilter->resetPipeline();
    }

    // -- フレームの転送 -----------------------------------------------------------------
    // frameIn の終了フレーム
    const int frameInFin = (is_saving) ? std::min(current_frame + frameInOffset, frame_n - 1) : current_frame;
    // フレーム転送の実行
    for (int i = 0; frameIn <= frameInFin; frameIn++, i++) {
        const auto& srcFrame = sharedPrms->srcFrame[i];
        const RGYFrameInfo in = setFrameInfo(frameIn, srcFrame.width, srcFrame.height, m_sharedFramesIn[i]->ptr());
        if (m_clfilter->sendInFrame(&in) != RGY_ERR_NONE) {
            return FALSE;
        }
    }
    // -- フレームの処理 -----------------------------------------------------------------
    if (prm != m_clfilter->getPrm()) { // パラメータが変更されていたら、
        frameProc = current_frame;   // 現在のフレームから処理をやり直す
    }
    // frameProc の終了フレーム
    const int frameProcFin = (is_saving) ? std::min(current_frame + frameProcOffset, frame_n - 1) : current_frame;
    // フレーム処理の実行
    for (; frameProc <= frameProcFin; frameProc++) {
        if (m_clfilter->proc(frameProc, prm) != RGY_ERR_NONE) {
            return FALSE;
        }
    }
    // -- フレームの取得 -----------------------------------------------------------------
    RGYFrameInfo out = setFrameInfo(current_frame, prm.outWidth, prm.outHeight, m_sharedFramesOut->ptr());
    if (m_clfilter->getOutFrame(&out) != RGY_ERR_NONE) {
        return FALSE;
    }
    // 本体が必要なデータをセット
    sharedPrms = (clfitersSharedPrms *)m_sharedPrms->ptr();
    sharedPrms->is_saving = is_saving;
    sharedPrms->nextOutFrameId = m_clfilter->getNextOutFrameId();
    sharedPrms->pd.s.platform = (decltype(sharedPrms->pd.s.platform))m_clfilter->platformID();
    sharedPrms->pd.s.device = (decltype(sharedPrms->pd.s.device))m_clfilter->deviceID();
    return TRUE;
}

int clFiltersExe::run() {
    bool abort = false;
    while (!abort && m_aviutlHandle && WaitForSingleObject(m_aviutlHandle.get(), 0) == WAIT_TIMEOUT) {
        if (WaitForSingleObject(m_eventMesStart, 5000) == WAIT_TIMEOUT) {
            continue;
        }
        const auto mes_type = ((clfitersSharedMesData*)m_sharedMessage->ptr())->type;
        switch (mes_type) {
        case clfitersMes::FuncProc:
            funcProc();
            break;
        case clfitersMes::Abort:
            abort = true;
            break;
        case clfitersMes::None:
            break;
        default:
            break;
        }
        SetEvent(m_eventMesEnd);
    }
    return 0;
}

int _tmain(const int argc, const TCHAR **argv) {
    AviutlAufExeParams prms;
    if (parse_cmd(prms, false, argc, argv) != 0) {
        return 1;
    }
    if (prms.clinfo) {
        const auto str = getOpenCLInfo(CL_DEVICE_TYPE_GPU);
        _ftprintf(stdout, _T("%s\n"), str.c_str());
        return 0;
    }
    // 構造体サイズの一致を確認
    if (prms.sizeSharedPrm != sizeof(clfitersSharedPrms)) {
        _ftprintf(stderr, _T("Invalid size for shared param: %d\n"), prms.sizeSharedPrm);
        return 1;
    }
    if (prms.sizeSharedMesData != sizeof(clfitersSharedMesData)) {
        _ftprintf(stderr, _T("Invalid size for shared message data: %d\n"), prms.sizeSharedMesData);
        return 1;
    }
    if (prms.sizePIXELYC != SIZE_PIXEL_YC) {
        _ftprintf(stderr, _T("Invalid size for PIXEL_YC: %d\n"), prms.sizeSharedMesData);
        return 1;
    }
    if (!prms.eventMesStart) {
        _ftprintf(stderr, _T("Event handle(start) not set.\n"));
        return 1;
    }
    if (!prms.eventMesEnd) {
        _ftprintf(stderr, _T("Event handle(end) not set.\n"));
        return 1;
    }
    // 実行開始
    clFiltersExe clfilterexe;
    clfilterexe.init(prms);
    int ret = clfilterexe.run();
    return ret;
}


