// -----------------------------------------------------------------------------------------
// clfilters by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2022 rigaya
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

#include "clfilters_version.h"
#include "clfilters_chain.h"
#include "rgy_filter_colorspace.h"
#include "rgy_filter_nnedi.h"
#include "rgy_filter_denoise_knn.h"
#include "rgy_filter_denoise_pmd.h"
#include "rgy_filter_smooth.h"
#include "rgy_filter_unsharp.h"
#include "rgy_filter_edgelevel.h"
#include "rgy_filter_warpsharp.h"
#include "rgy_filter_deband.h"
#include "rgy_filter_tweak.h"

static const TCHAR *LOG_FILE_NAME = "clfilters.auf.log";

static const auto CLFILTER_TO_STR = make_array<std::pair<clFilter, tstring>>(
    std::make_pair(clFilter::UNKNOWN,    _T("unknown")),
    std::make_pair(clFilter::COLORSPACE, _T("colorspace")),
    std::make_pair(clFilter::NNEDI,      _T("nnedi")),
    std::make_pair(clFilter::KNN,        _T("knn")),
    std::make_pair(clFilter::PMD,        _T("pmd")),
    std::make_pair(clFilter::SMOOTH,     _T("smooth")),
    std::make_pair(clFilter::RESIZE,     _T("resize")),
    std::make_pair(clFilter::UNSHARP,    _T("unsharp")),
    std::make_pair(clFilter::EDGELEVEL,  _T("edgelevel")),
    std::make_pair(clFilter::WARPSHARP,  _T("warpsharp")),
    std::make_pair(clFilter::TWEAK,      _T("tweak")),
    std::make_pair(clFilter::DEBAND,     _T("deband"))
);
MAP_PAIR_0_1(clfilter, cl, clFilter, str, tstring, CLFILTER_TO_STR, clFilter::UNKNOWN, _T("unknown"));

clFilterFrameBuffer::clFilterFrameBuffer(std::shared_ptr<RGYOpenCLContext> cl) :
    m_cl(cl),
    m_frame(),
    m_in(0),
    m_out(0) {

}

clFilterFrameBuffer::~clFilterFrameBuffer() {
    freeFrames();
}

void clFilterFrameBuffer::freeFrames() {
    for (auto& f : m_frame) {
        if (f) {
            f->resetMappedFrame();
        }
        f.reset();
    }
    m_in = 0;
    m_out = 0;
}

void clFilterFrameBuffer::resetCachedFrames() {
    for (auto& f : m_frame) {
        if (f) {
            f->resetMappedFrame();
        }
    }
    m_in = 0;
    m_out = 0;
};

RGYCLFrame *clFilterFrameBuffer::get_in(const int width, const int height) {
    if (!m_frame[m_in] || m_frame[m_in]->frame.width != width || m_frame[m_in]->frame.height != height) {
        m_frame[m_in] = m_cl->createFrameBuffer(width, height, RGY_CSP_YUV444_16, 16, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR);
    }
    return m_frame[m_in].get();
}
RGYCLFrame *clFilterFrameBuffer::get_out() {
    return m_frame[m_out].get();
}
RGYCLFrame *clFilterFrameBuffer::get_out(const int frameID) {
    for (auto& f : m_frame) {
        if (f && f->frame.inputFrameId == frameID) {
            return f.get();
        }
    }
    return nullptr;
}
void clFilterFrameBuffer::in_to_next() { m_in = (m_in + 1) % m_frame.size(); }
void clFilterFrameBuffer::out_to_next() { m_out = (m_out + 1) % m_frame.size(); }

clFilterChainParam::clFilterChainParam() :
    hModule(NULL),
    filterOrder(),
    colorspace(),
    nnedi(),
    smooth(),
    knn(),
    pmd(),
    resize_algo(RGY_VPP_RESIZE_SPLINE36),
    resize_mode(RGY_VPP_RESIZE_MODE_DEFAULT),
    unsharp(),
    edgelevel(),
    warpsharp(),
    tweak(),
    deband(),
    log_level(RGY_LOG_QUIET),
    log_to_file(false) {

}

bool clFilterChainParam::operator==(const clFilterChainParam &x) const {
    return hModule == x.hModule
        && colorspace == x.colorspace
        && smooth == x.smooth
        && knn == x.knn
        && pmd == x.pmd
        && resize_algo == x.resize_algo
        && resize_mode == x.resize_mode
        && unsharp == x.unsharp
        && edgelevel == x.edgelevel
        && warpsharp == x.warpsharp
        && tweak == x.tweak
        && deband == x.deband
        && log_level == x.log_level
        && log_to_file == x.log_to_file;
}
bool clFilterChainParam::operator!=(const clFilterChainParam &x) const {
    return !(*this == x);
}

std::vector<clFilter> clFilterChainParam::getFilterChain(const bool resizeRequired) const {
    std::vector<clFilter> allFilterOrder = filterOrder; // 指定の順序
    // 指定の順序にないフィルタを追加
    for (const auto& f : filterList) {
        if (std::find(allFilterOrder.begin(), allFilterOrder.end(), f.second) == allFilterOrder.end()) {
            allFilterOrder.push_back(f.second);
        }
    }
    // 有効なフィルタだけを抽出
    std::vector<clFilter> enabledFilterOrder;
    for (const auto filterType : allFilterOrder) {
        if (  (colorspace.enable && filterType == clFilter::COLORSPACE)
           || (nnedi.enable      && filterType == clFilter::NNEDI)
           || (smooth.enable     && filterType == clFilter::SMOOTH)
           || (knn.enable        && filterType == clFilter::KNN)
           || (pmd.enable        && filterType == clFilter::PMD)
           || (resizeRequired    && filterType == clFilter::RESIZE)
           || (unsharp.enable    && filterType == clFilter::UNSHARP)
           || (edgelevel.enable  && filterType == clFilter::EDGELEVEL)
           || (warpsharp.enable  && filterType == clFilter::WARPSHARP)
           || (tweak.enable      && filterType == clFilter::TWEAK)
           || (deband.enable     && filterType == clFilter::DEBAND)) {
            enabledFilterOrder.push_back(filterType);
        }
    }
    return enabledFilterOrder;
}

clFilterChain::clFilterChain() :
    m_log(),
    m_prm(),
    m_cl(),
    m_platformID(-1),
    m_deviceID(-1),
    m_deviceName(),
    m_frameIn(),
    m_frameOut(),
    m_queueSendIn(),
    m_filters(),
    m_convert_yc48_to_yuv444_16(),
    m_convert_yuv444_16_to_yc48() {

}

clFilterChain::~clFilterChain() {
    close();
}

void clFilterChain::close() {
    m_filters.clear();
    m_frameIn.reset();
    m_queueSendIn.finish(); // m_frameIn.reset() のあと
    m_queueSendIn.clear();  // m_frameIn.reset() のあと
    m_frameOut.reset();
    m_cl.reset();
    m_deviceName.clear();
    m_convert_yc48_to_yuv444_16.reset();
    m_convert_yuv444_16_to_yc48.reset();
    m_log.reset();
    m_platformID = -1;
    m_deviceID = -1;
}

void clFilterChain::PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...) {
    va_list args;
    va_start(args, format);

    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    vector<TCHAR> buffer(len, 0);
    _vstprintf_s(buffer.data(), len, format, args);
    va_end(args);

    m_log->write_log(logLevel, RGY_LOGT_APP, buffer.data());

    if (logLevel < RGY_LOG_ERROR) {
        return;
    }

    MessageBoxA(NULL, buffer.data(), AUF_FULL_NAME, MB_OK | MB_ICONEXCLAMATION);
}

RGY_ERR clFilterChain::init(const int platformID, const int deviceID, const cl_device_type device_type, const RGYLogLevel log_level, const bool log_to_file) {
    m_log = std::make_shared<RGYLog>(log_to_file ? LOG_FILE_NAME : nullptr, log_level);

    if (auto err = initOpenCL(platformID, deviceID, device_type); err != RGY_ERR_NONE) {
        return err;
    }

    m_frameIn = std::make_unique<clFilterFrameBuffer>(m_cl);
    m_frameOut = std::make_unique<clFilterFrameBuffer>(m_cl);

    m_convert_yc48_to_yuv444_16 = std::make_unique<RGYConvertCSP>();
    if (m_convert_yc48_to_yuv444_16->getFunc(RGY_CSP_YC48, RGY_CSP_YUV444_16, false, RGY_SIMD::SIMD_ALL) == nullptr) {
        PrintMes(RGY_LOG_ERROR, _T("color conversion not supported: %s -> %s.\n"),
                 RGY_CSP_NAMES[RGY_CSP_YC48], RGY_CSP_NAMES[RGY_CSP_YUV444_16]);
        return RGY_ERR_INVALID_COLOR_FORMAT;
    }
    PrintMes(RGY_LOG_INFO, _T("color conversion %s -> %s [%s].\n"),
        RGY_CSP_NAMES[RGY_CSP_YC48], RGY_CSP_NAMES[RGY_CSP_YUV444_16], get_simd_str(m_convert_yc48_to_yuv444_16->getFunc()->simd));

    m_convert_yuv444_16_to_yc48 = std::make_unique<RGYConvertCSP>();
    if (m_convert_yuv444_16_to_yc48->getFunc(RGY_CSP_YUV444_16, RGY_CSP_YC48, false, RGY_SIMD::SIMD_ALL) == nullptr) {
        PrintMes(RGY_LOG_ERROR, _T("unsupported color format conversion, %s -> %s\n"), RGY_CSP_NAMES[RGY_CSP_YUV444_16], RGY_CSP_NAMES[RGY_CSP_YC48]);
        return RGY_ERR_INVALID_COLOR_FORMAT;
    }
    PrintMes(RGY_LOG_INFO, _T("color conversion %s -> %s [%s].\n"),
        RGY_CSP_NAMES[RGY_CSP_YUV444_16], RGY_CSP_NAMES[RGY_CSP_YC48], get_simd_str(m_convert_yuv444_16_to_yc48->getFunc()->simd));
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::initOpenCL(const int platformID, const int deviceID, const cl_device_type device_type) {
    PrintMes(RGY_LOG_INFO, _T("start init OpenCL platform %d, device %d\n"), platformID, deviceID);

    RGYOpenCL cl(m_log);
    auto platforms = cl.getPlatforms(nullptr);
    if (platforms.size() == 0) {
        PrintMes(RGY_LOG_ERROR, _T("No OpenCL Platform found on this system.\n"));
        return RGY_ERR_DEVICE_NOT_FOUND;
    }

    if (platformID >= 0 && platformID >= (int)platforms.size()) {
        PrintMes(RGY_LOG_ERROR, _T("platform %d does not exist (platform count = %d)\n"), platformID, (int)platforms.size());
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    PrintMes(RGY_LOG_INFO, _T("OpenCL platform count %d\n"), (int)platforms.size());

    auto& platform = platforms[std::max(platformID, 0)];

    auto err = err_cl_to_rgy(platform->createDeviceList(device_type));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to create device list: %s\n"), get_err_mes(err));
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    PrintMes(RGY_LOG_INFO, _T("created device list: %d device(s).\n"), platform->devs().size());

    if (deviceID >= 0 && deviceID >= (int)platform->devs().size()) {
        PrintMes(RGY_LOG_ERROR, _T("Invalid device Id %d (device count %d)\n"), deviceID, (int)platform->devs().size());
        return RGY_ERR_INVALID_DEVICE;
    }

    platform->setDev(platform->devs()[std::max(deviceID, 0)]);
    m_cl = std::make_shared<RGYOpenCLContext>(platform, m_log);
    if (m_cl->createContext(0) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to create OpenCL context.\n"));
        return RGY_ERR_UNKNOWN;
    }

    const auto devInfo = platform->dev(0).info();
    m_deviceName = (devInfo.board_name_amd.length() > 0) ? devInfo.board_name_amd : devInfo.name;
    m_platformID = platformID;
    m_deviceID = deviceID;
    m_queueSendIn = m_cl->createQueue(platform->dev(0).id(), 0 /*CL_QUEUE_PROFILING_ENABLE*/);

    PrintMes(RGY_LOG_INFO, _T("created OpenCL context, selcted device %s.\n"), m_deviceName.c_str());
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::configureOneFilter(std::unique_ptr<RGYFilter>& filter, RGYFrameInfo& inputFrame, const clFilter filterType, const int resizeWidth, const int resizeHeight) {
    // colorspace
    if (filterType == clFilter::COLORSPACE) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterColorspace(m_cl));
        }
        std::shared_ptr<RGYFilterParamColorspace> param(new RGYFilterParamColorspace());
        param->colorspace = m_prm.colorspace;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init colorspace.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    // nnedi
    if (filterType == clFilter::NNEDI) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterNnedi(m_cl));
        }
        std::shared_ptr<RGYFilterParamNnedi> param(new RGYFilterParamNnedi());
        param->nnedi = m_prm.nnedi;
        param->hModule = m_prm.hModule;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init nnedi.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //ノイズ除去 (knn)
    if (filterType == clFilter::KNN) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDenoiseKnn(m_cl));
        }
        std::shared_ptr<RGYFilterParamDenoiseKnn> param(new RGYFilterParamDenoiseKnn());
        param->knn = m_prm.knn;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init knn.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //ノイズ除去 (pmd)
    if (filterType == clFilter::PMD) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDenoisePmd(m_cl));
        }
        std::shared_ptr<RGYFilterParamDenoisePmd> param(new RGYFilterParamDenoisePmd());
        param->pmd = m_prm.pmd;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init pmd.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //ノイズ除去 (smooth)
    if (filterType == clFilter::SMOOTH) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterSmooth(m_cl));
        }
        std::shared_ptr<RGYFilterParamSmooth> param(new RGYFilterParamSmooth());
        param->smooth = m_prm.smooth;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init smooth.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }

    //リサイズ
    if (filterType == clFilter::RESIZE) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterResize(m_cl));
        }
        std::shared_ptr<RGYFilterParamResize> param(new RGYFilterParamResize());
        param->interp = m_prm.resize_algo;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->frameOut.width = resizeWidth;
        param->frameOut.height = resizeHeight;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init resize.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //unsharp
    if (filterType == clFilter::UNSHARP) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterUnsharp(m_cl));
        }
        std::shared_ptr<RGYFilterParamUnsharp> param(new RGYFilterParamUnsharp());
        param->unsharp.radius = m_prm.unsharp.radius;
        param->unsharp.weight = m_prm.unsharp.weight;
        param->unsharp.threshold = m_prm.unsharp.threshold;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init unsharp.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //edgelevel
    if (filterType == clFilter::EDGELEVEL) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterEdgelevel(m_cl));
        }
        std::shared_ptr<RGYFilterParamEdgelevel> param(new RGYFilterParamEdgelevel());
        param->edgelevel = m_prm.edgelevel;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init edgelevel.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //warpsharp
    if (filterType == clFilter::WARPSHARP) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterWarpsharp(m_cl));
        }
        std::shared_ptr<RGYFilterParamWarpsharp> param(new RGYFilterParamWarpsharp());
        param->warpsharp = m_prm.warpsharp;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init warpsharp.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //tweak
    if (filterType == clFilter::TWEAK) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterTweak(m_cl));
        }
        std::shared_ptr<RGYFilterParamTweak> param(new RGYFilterParamTweak());
        param->tweak = m_prm.tweak;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = true;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init tweak.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //deband
    if (filterType == clFilter::DEBAND) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDeband(m_cl));
        }
        std::shared_ptr<RGYFilterParamDeband> param(new RGYFilterParamDeband());
        param->deband = m_prm.deband;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init deband.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    return RGY_ERR_NONE;
}

tstring clFilterChain::printFilterChain(const std::vector<clFilter>& filterChain) const {
    tstring str;
    for (auto& filter : filterChain) {
        if (str.length() > 0) str += _T(", ");
        str += clfilter_cl_to_str(filter);
    }
    return str;
}

bool clFilterChain::filterChainEqual(const std::vector<clFilter>& target) const {
    if (m_filters.size() != target.size()) {
        return false;
    }
    for (size_t ifilter = 0; ifilter < m_filters.size(); ifilter++) {
        if (m_filters[ifilter].first != target[ifilter]) {
            return false;
        }
    }
    return true;
}

RGY_ERR clFilterChain::filterChainCreate(const RGYFrameInfo *pInputFrame, const int outWidth, const int outHeight) {
    RGYFrameInfo inputFrame = *pInputFrame;
    for (size_t i = 0; i < _countof(inputFrame.ptr); i++) {
        inputFrame.ptr[i] = nullptr;
    }

    const bool resizeRequired = pInputFrame->width != outWidth || pInputFrame->height != outHeight;
    const auto filterChain = m_prm.getFilterChain(resizeRequired);
    if (!filterChainEqual(filterChain)) {
        PrintMes(RGY_LOG_INFO, _T("clFilterChain changed: %s\n"), printFilterChain(filterChain).c_str());

        decltype(m_filters) newFilters;
        newFilters.reserve(filterChain.size());
        for (const auto filterType : filterChain) {
            auto filter = std::make_pair(filterType, std::unique_ptr<RGYFilter>());
            for (auto& oldFilter : m_filters) {
                if (oldFilter.first == filterType && oldFilter.second) {
                    filter.second = std::move(oldFilter.second);
                    break;
                }
            }
            newFilters.push_back(std::move(filter));
        }
        m_filters.clear();
        m_filters = std::move(newFilters);
    }
    for (auto& fitler : m_filters) {
        auto err = configureOneFilter(fitler.second, inputFrame, fitler.first, outWidth, outHeight);
        if (err != RGY_ERR_NONE) {
            return err;
        }
    }
    return RGY_ERR_NONE;
}

void clFilterChain::resetPipeline() {
    m_frameIn->resetCachedFrames();
    m_frameOut->resetCachedFrames();
    PrintMes(RGY_LOG_DEBUG, _T("clFilterChain reset pipeline.\n"));
}

static void copyFramePropWithoutCsp(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    dst->width = src->width;
    dst->height = src->height;
    dst->picstruct = src->picstruct;
    dst->timestamp = src->timestamp;
    dst->duration = src->duration;
    dst->flags = src->flags;
    dst->inputFrameId = src->inputFrameId;
}

RGY_ERR clFilterChain::sendInFrame(const RGYFrameInfo *pInputFrame) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevIn = m_frameIn->get_in(pInputFrame->width, pInputFrame->height);
    m_frameIn->in_to_next();

    auto err = frameDevIn->queueMapBuffer(m_queueSendIn, CL_MAP_WRITE /*CL_​MAP_​WRITE_​INVALIDATE_​REGION*/);
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to queue map input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    frameDevIn->mapWait();
    copyFramePropWithoutCsp(&frameDevIn->frame, pInputFrame);

    {
        auto frameHostIn = frameDevIn->mappedHost();

        //YC48->YUV444(16bit)
        int crop[4] = { 0 };
        m_convert_yc48_to_yuv444_16->run(false,
            frameHostIn->ptr().data(), (const void **)&pInputFrame->ptr[0],
            pInputFrame->width, pInputFrame->pitch[0], pInputFrame->pitch[0],
            frameHostIn->pitch(0), pInputFrame->height, frameHostIn->height(), crop);
    }

    if ((err = frameDevIn->unmapBuffer()) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to unmap input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}

int clFilterChain::getNextOutFrameId() const {
    if (!m_frameOut) return -1;

    auto frameDevOut = m_frameOut->get_out();
    return (frameDevOut) ? frameDevOut->frame.inputFrameId : -1;
}

RGY_ERR clFilterChain::getOutFrame(RGYFrameInfo *pOutputFrame) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevOut = m_frameOut->get_out(pOutputFrame->inputFrameId);
    m_frameOut->out_to_next();

    if (!frameDevOut) {
        return RGY_ERR_OUT_OF_RANGE;
    }
    frameDevOut->mapWait();
    copyFramePropWithoutCsp(pOutputFrame, &frameDevOut->frame);
    {
        auto frameHostOut = frameDevOut->mappedHost();
        //YUV444(16bit)->YC48
        int crop[4] = { 0 };
        m_convert_yuv444_16_to_yc48->run(false,
            (void **)&pOutputFrame->ptr[0], (const void **)frameHostOut->ptr().data(),
            frameHostOut->width(), frameHostOut->pitch(0), frameHostOut->pitch(0),
            pOutputFrame->pitch[0], frameHostOut->height(), pOutputFrame->height, crop);
    }
    auto err = frameDevOut->unmapBuffer();
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to unmap output buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    frameDevOut->resetMappedFrame();
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::proc(const int frameID, const int outWidth, const int outHeight, const clFilterChainParam& prm) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevIn = m_frameIn->get_out(frameID);
    m_frameIn->out_to_next();

    if (!frameDevIn) {
        return RGY_ERR_OUT_OF_RANGE;
    }
    m_cl->setModuleHandle(prm.hModule);
    m_log->setLogLevel(prm.log_level, RGY_LOGT_ALL);
    m_log->setLogFile(prm.log_to_file ? LOG_FILE_NAME : nullptr);
    m_prm = prm;

    auto frameDevOut = m_frameOut->get_in(outWidth, outHeight);
    m_frameOut->in_to_next();

    //フィルタチェーン更新
    auto err = filterChainCreate(&frameDevIn->frame, outWidth, outHeight);
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to update filter chain.\n"));
        return err;
    }

    frameDevIn->mapWait();

    //通常は、最後のひとつ前のフィルタまで実行する
    //上書き型のフィルタが最後の場合は、そのフィルタまで実行する(最後はコピーが必須)
    const auto filterfin = (m_filters.back().second->GetFilterParam()->bOutOverwrite) ? m_filters.size() : m_filters.size() - 1;
    //フィルタチェーン実行
    auto frameInfo = frameDevIn->frame;
    for (size_t ifilter = 0; ifilter < filterfin; ifilter++) {
        int nOutFrames = 0;
        RGYFrameInfo *outInfo[16] = { 0 };
        err = m_filters[ifilter].second->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error while running filter \"%s\": %s.\n"), m_filters[ifilter].second->name().c_str(), get_err_mes(err));
            return err;
        }
        if (nOutFrames > 1) {
            PrintMes(RGY_LOG_ERROR, _T("Currently only simple filters are supported.\n"));
            return RGY_ERR_UNSUPPORTED;
        }
        frameInfo = *(outInfo[0]);
    }
    //最後のフィルタ
    if (m_filters.back().second->GetFilterParam()->bOutOverwrite) {
        if ((err = m_cl->copyFrame(&frameDevOut->frame, &frameInfo)) != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error in frame copy: %s.\n"), get_err_mes(err));
            return err;
        }
    } else {
        auto& lastFilter = m_filters[m_filters.size() - 1];
        int nOutFrames = 0;
        RGYFrameInfo *outInfo[16] = { 0 };
        outInfo[0] = &frameDevOut->frame;
        err = lastFilter.second->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error while running filter \"%s\": %s.\n"), lastFilter.second->name().c_str(), get_err_mes(err));
            return err;
        }
    }
    if ((err = frameDevOut->queueMapBuffer(m_cl->queue(), CL_MAP_READ)) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to queue map input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}
