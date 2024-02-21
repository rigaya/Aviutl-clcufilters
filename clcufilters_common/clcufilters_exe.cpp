// -----------------------------------------------------------------------------------------
// clfilters by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2024 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// ------------------------------------------------------------------------------------------

#include "clcufilters_exe.h"

clcuFiltersExe::clcuFiltersExe() :
    m_filter(),
    m_aviutlHandle(),
    m_eventMesStart(nullptr),
    m_eventMesEnd(nullptr),
    m_sharedMessage(),
    m_sharedPrms(),
    m_sharedFramesIn(),
    m_ppid(0),
    m_maxWidth(0),
    m_maxHeight(0),
    m_pitchBytes(0),
    m_log() { }
clcuFiltersExe::~clcuFiltersExe() {
    m_sharedFramesIn.reset();
}

int clcuFiltersExe::init(AviutlAufExeParams& prms) {
    m_eventMesStart = prms.eventMesStart;
    m_eventMesEnd = prms.eventMesEnd;
    if (!m_eventMesStart || !m_eventMesEnd) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid event handles."));
        return 1;
    }
    WaitForSingleObject(m_eventMesStart, INFINITE);
    m_log = std::make_shared<RGYLog>(prms.logfile.c_str(), prms.log_level, false, true);

    m_ppid = prms.ppid;
    m_aviutlHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, handle_deleter>(OpenProcess(SYNCHRONIZE | PROCESS_VM_READ, FALSE, prms.ppid), handle_deleter());
    if (!m_aviutlHandle) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to open Aviutl process handle %d.\n"), prms.ppid);
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Opened Aviutl process handle %d.\n"), prms.ppid);

    m_sharedMessage = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_MESSAGE, prms.ppid).c_str(), sizeof(clfitersSharedMesData));
    if (!m_sharedMessage || !m_sharedMessage->is_open()) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to open shared mem for messages.\n"));
        m_sharedMessage.reset();
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Opened shared mem for messages.\n"));

    m_sharedPrms = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_PRMS, prms.ppid).c_str(), sizeof(clfitersSharedPrms));
    if (!m_sharedPrms || !m_sharedPrms->is_open()) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to open shared mem for parameters.\n"));
        m_sharedPrms.reset();
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Opened shared mem for parameters.\n"));

    m_maxWidth = prms.max_w;
    m_maxHeight = prms.max_h;
    m_pitchBytes = get_shared_frame_pitch(m_maxWidth);
    const int frameSize = m_pitchBytes * m_maxHeight;
    AddMessage(RGY_LOG_DEBUG, _T("Frame max %dx%d, pitch %d, size %d.\n"), m_maxWidth, m_maxHeight, m_pitchBytes, frameSize);

    m_sharedFramesIn = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_FRAMES_IN, m_ppid).c_str(), frameSize);
    if (!m_sharedFramesIn || !m_sharedFramesIn->is_open()) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to open shared mem for frame(out).\n"));
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Opened shared mem for frame(out).\n"));

    //デバッグ用

    initDevices();
    // プロセス初期化処理の終了を通知
    SetEvent(m_eventMesEnd);
    return 0;
}

RGYFrameInfo clcuFiltersExe::setFrameInfo(const int iframeID, const int width, const int height, void *frame) {
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

int clcuFiltersExe::funcProc() {
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
    const auto frameInFin = sharedPrms->frameInFin;
    auto frameProc = sharedPrms->frameProc;
    //auto frameOut = sharedPrms->frameOut;
    prm.setPrmFromCmd(char_to_tstring(sharedPrms->prms));
    if (!m_filter
        || m_filter->platformID() != dev_pd.s.platform
        || m_filter->deviceID() != dev_pd.s.device) {
        int ret = 0;
        std::string mes = AUF_FULL_NAME;
        mes += ": ";
        auto sts = initDevice(sharedPrms, prm);
        if (sts != RGY_ERR_NONE) {
            mes += LB_WND_OPENCL_UNAVAIL;
            getMessagePtr()->ret = ret;
            strcpy_s(getMessagePtr()->data, mes.c_str());
            return sts;
        }
    }

    // 保存モード(is_saving=true)の時に、どのくらい先まで処理をしておくべきか?
    static_assert(frameInOffset < clcuFilterFrameBuffer::bufSize);
    static_assert(frameInOffset < _countof(clfitersSharedPrms::srcFrame));
    if (resetPipeline
        || m_filter->getNextOutFrameId() != current_frame) { // 出てくる予定のフレームがずれていたらリセット
        m_filter->resetPipeline();
    }

    // -- フレームの転送 -----------------------------------------------------------------
    // frameIn の終了フレーム
    //const int frameInFin = (is_saving) ? std::min(current_frame + frameInOffset, frame_n - 1) : current_frame;
    // フレーム転送の実行
    auto sts = RGY_ERR_NONE;
    for (int i = 0; frameIn <= frameInFin; frameIn++, i++) {
        if (i > 0) {
            // 2フレーム目以降は、プラグイン側の処理完了を待つ必要がある
            while (WaitForSingleObject(m_eventMesStart, 100) == WAIT_TIMEOUT) {
                // Aviutl側が終了していたら終了
                if (m_aviutlHandle && WaitForSingleObject(m_aviutlHandle.get(), 0) != WAIT_TIMEOUT) {
                    return FALSE;
                }
            }
        }
        const auto& srcFrame = sharedPrms->srcFrame[i];
        const RGYFrameInfo in = setFrameInfo(frameIn, srcFrame.width, srcFrame.height, m_sharedFramesIn->ptr());
        // 受け取り中のエラーは保持するが、まずは最後まで受け取ることを優先する
        if (sts == RGY_ERR_NONE) {
            // 成功していた場合のみGPUに転送する
            sts = m_filter->sendInFrame(&in);
        }
        if (frameIn < frameInFin) {
            // まだ受け取るフレームがあるなら、GPUへの転送完了を通知
            SetEvent(m_eventMesEnd);
        } 
    }
    if (sts != RGY_ERR_NONE) {
        return sts;
    }
    if (frame_n <= 0 || current_frame < 0) {
        return TRUE; // 何もしない
    }
    // -- フレームの処理 -----------------------------------------------------------------
    if (prm != m_filter->getPrm()) { // パラメータが変更されていたら、
        frameProc = current_frame;   // 現在のフレームから処理をやり直す
    }
    // frameProc の終了フレーム
    const int frameProcFin = (is_saving) ? std::min(current_frame + frameProcOffset, frame_n - 1) : current_frame;
    // フレーム処理の実行
    for (; frameProc <= frameProcFin; frameProc++) {
        if (m_filter->proc(frameProc, prm) != RGY_ERR_NONE) {
            return FALSE;
        }
    }
    // -- フレームの取得 -----------------------------------------------------------------
    RGYFrameInfo out = setFrameInfo(current_frame, prm.outWidth, prm.outHeight, m_sharedFramesIn->ptr());
    if (m_filter->getOutFrame(&out) != RGY_ERR_NONE) {
        return FALSE;
    }
    // 本体が必要なデータをセット
    sharedPrms = (clfitersSharedPrms *)m_sharedPrms->ptr();
    sharedPrms->is_saving = is_saving;
    sharedPrms->nextOutFrameId = m_filter->getNextOutFrameId();
    sharedPrms->pd.s.platform = (decltype(sharedPrms->pd.s.platform))m_filter->platformID();
    sharedPrms->pd.s.device = (decltype(sharedPrms->pd.s.device))m_filter->deviceID();
    return TRUE;
}

int clcuFiltersExe::run() {
    bool abort = false;
    while (!abort && m_aviutlHandle && WaitForSingleObject(m_aviutlHandle.get(), 0) == WAIT_TIMEOUT) {
        if (WaitForSingleObject(m_eventMesStart, 5000) == WAIT_TIMEOUT) {
            continue;
        }
        int ret = 0;
        const auto mes_type = ((clfitersSharedMesData*)m_sharedMessage->ptr())->type;
        switch (mes_type) {
        case clfitersMes::FuncProc:
            ret = funcProc();
            break;
        case clfitersMes::Abort:
            abort = true;
            break;
        case clfitersMes::None:
            break;
        default:
            break;
        }
        ((clfitersSharedMesData*)m_sharedMessage->ptr())->ret = ret;
        SetEvent(m_eventMesEnd);
    }
    return 0;
}
