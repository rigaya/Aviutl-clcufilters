// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
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

clFilterChainParam::clFilterChainParam() :
    hModule(NULL),
    colorspace(),
    nnedi(),
    knn(),
    pmd(),
    smooth(),
    resize_algo(RGY_VPP_RESIZE_SPLINE36),
    resize_mode(RGY_VPP_RESIZE_MODE_DEFAULT),
    unsharp(),
    edgelevel(),
    warpsharp(),
    tweak(),
    deband() {

}

std::vector<clFilter> clFilterChainParam::getFilterChain(const bool resizeRequired) const {
    std::vector<clFilter>  filters;
    if (colorspace.enable) filters.push_back(clFilter::COLORSPACE);
    if (nnedi.enable)      filters.push_back(clFilter::NNEDI);
    if (knn.enable)        filters.push_back(clFilter::KNN);
    if (pmd.enable)        filters.push_back(clFilter::PMD);
    if (smooth.enable)     filters.push_back(clFilter::SMOOTH);
    if (resizeRequired)    filters.push_back(clFilter::RESIZE);
    if (unsharp.enable)    filters.push_back(clFilter::UNSHARP);
    if (edgelevel.enable)  filters.push_back(clFilter::EDGELEVEL);
    if (warpsharp.enable)  filters.push_back(clFilter::WARPSHARP);
    if (tweak.enable)      filters.push_back(clFilter::TWEAK);
    if (deband.enable)     filters.push_back(clFilter::DEBAND);
    return filters;
}

bool clFilterChainParam::filtersEqual(const clFilterChainParam& obj, const bool resizeRequired) const {
    const auto current = this->getFilterChain(resizeRequired);
    const auto objchain = obj.getFilterChain(resizeRequired);
    return current.size() == objchain.size() && std::equal(current.cbegin(), current.cend(), objchain.cbegin());
}

clFilterChain::clFilterChain() :
    m_log(),
    m_prm(),
    m_cl(),
    m_platformID(-1),
    m_deviceID(-1),
    m_deviceName(),
    m_dev(),
    m_filters(),
    m_convert_yc48_to_yuv444_16(),
    m_convert_yuv444_16_to_yc48() {

}

clFilterChain::~clFilterChain() {
    close();
}

void clFilterChain::close() {
    m_filters.clear();
    for (auto& frame : m_dev) {
        frame.reset();
    }
    m_cl.reset();
    m_deviceName.clear();
    m_convert_yc48_to_yuv444_16.reset();
    m_convert_yuv444_16_to_yc48.reset();
    m_log.reset();
    m_platformID = -1;
    m_deviceID = -1;
}

void clFilterChain::PrintMes(int logLevel, const TCHAR *format, ...) {
    if (logLevel < RGY_LOG_ERROR) {
        return;
    }

    va_list args;
    va_start(args, format);

    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    vector<TCHAR> buffer(len, 0);
    _vstprintf_s(buffer.data(), len, format, args);
    va_end(args);

    MessageBoxA(NULL, buffer.data(), AUF_FULL_NAME, MB_OK | MB_ICONEXCLAMATION);
}

RGY_ERR clFilterChain::init(const int platformID, const int deviceID, const cl_device_type device_type) {
    m_log = std::make_shared<RGYLog>("clfiters.auf.log", RGY_LOG_DEBUG);

    if (auto err = initOpenCL(platformID, deviceID, device_type); err != RGY_ERR_NONE) {
        return err;
    }

    m_convert_yc48_to_yuv444_16 = std::make_unique<RGYConvertCSP>();
    if (m_convert_yc48_to_yuv444_16->getFunc(RGY_CSP_YC48, RGY_CSP_YUV444_16, false, RGY_SIMD::SIMD_ALL) == nullptr) {
        PrintMes(RGY_LOG_ERROR, _T("color conversion not supported: %s -> %s.\n"),
                 RGY_CSP_NAMES[RGY_CSP_YC48], RGY_CSP_NAMES[RGY_CSP_YUV444_16]);
        return RGY_ERR_INVALID_COLOR_FORMAT;
    }
    m_convert_yuv444_16_to_yc48 = std::make_unique<RGYConvertCSP>();
    if (m_convert_yuv444_16_to_yc48->getFunc(RGY_CSP_YUV444_16, RGY_CSP_YC48, false, RGY_SIMD::SIMD_ALL) == nullptr) {
        PrintMes(RGY_LOG_ERROR, _T("unsupported color format conversion, %s -> %s\n"), RGY_CSP_NAMES[RGY_CSP_YUV444_16], RGY_CSP_NAMES[RGY_CSP_YC48]);
        return RGY_ERR_INVALID_COLOR_FORMAT;
    }
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::initOpenCL(const int platformID, const int deviceID, const cl_device_type device_type) {
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

    auto& platform = platforms[std::max(platformID, 0)];

    auto err = err_cl_to_rgy(platform->createDeviceList(device_type));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to create device list: %s\n"), get_err_mes(err));
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    PrintMes(RGY_LOG_DEBUG, _T("created device list: %d device(s).\n"), platform->devs().size());

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
    m_cl->setModuleHandle(GetModuleHandleA(AUF_NAME));
    const auto devInfo = platform->dev(0).info();
    m_deviceName = devInfo.name;
    m_platformID = platformID;
    m_deviceID = deviceID;

    return RGY_ERR_NONE;
}

std::string clFilterChain::getDeviceName() const {
    return m_deviceName;
}

bool clFilterChain::resizeRequired(const RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame) const {
    return pInputFrame->width != pOutputFrame->width || pInputFrame->height != pOutputFrame->height;
}

RGY_ERR clFilterChain::allocateBuffer(const RGYFrameInfo *pInputFrame, const RGYFrameInfo *pOutputFrame) {
    if (!m_dev[0]
        || pInputFrame->width  != m_dev[0]->frame.width
        || pInputFrame->height != m_dev[0]->frame.height) {
        m_dev[0] = m_cl->createFrameBuffer(pInputFrame->width, pInputFrame->height, RGY_CSP_YUV444_16, 16, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR);
        if (!m_dev[0]) {
            return RGY_ERR_NULL_PTR;
        }
    }
    if (!m_dev[1]
        || pOutputFrame->width  != m_dev[1]->frame.width
        || pOutputFrame->height != m_dev[1]->frame.height) {
        m_dev[1] = m_cl->createFrameBuffer(pOutputFrame->width, pOutputFrame->height, RGY_CSP_YUV444_16, 16, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR);
        if (!m_dev[1]) {
            return RGY_ERR_NULL_PTR;
        }
    }
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
        param->hModule = m_prm.hModule;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
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
        auto sts = filter->init(param, nullptr);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init deband.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::filterChainCreate(const RGYFrameInfo *pInputFrame, const RGYFrameInfo *pOutputFrame, const bool reset) {
    RGYFrameInfo inputFrame = *pInputFrame;
    for (size_t i = 0; i < _countof(inputFrame.ptr); i++) {
        inputFrame.ptr[i] = nullptr;
    }

    const auto filterChain = m_prm.getFilterChain(resizeRequired(pOutputFrame, pInputFrame));
    if (reset || m_filters.size() != filterChain.size()) {
        m_filters.clear();
        m_filters.resize(filterChain.size());
    }

    for (size_t ifilter = 0; ifilter < m_filters.size(); ifilter++) {
        auto err = configureOneFilter(m_filters[ifilter], inputFrame, filterChain[ifilter], pOutputFrame->width, pOutputFrame->height);
        if (err != RGY_ERR_NONE) {
            return err;
        }
    }
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::proc(RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame, const clFilterChainParam& prm) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }

    //解像度チェック、メモリ確保
    auto err = allocateBuffer(pInputFrame, pOutputFrame);
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to allocate buffer.\n"));
        close();
        return err;
    }

    const bool recreate_filter_chain = !prm.filtersEqual(m_prm, resizeRequired(pOutputFrame, pInputFrame));
    m_prm = prm;
    if (m_prm.getFilterChain(resizeRequired(pOutputFrame, pInputFrame)).size() == 0) {
        memcpy(pOutputFrame->ptr[0], pInputFrame->ptr[0], pInputFrame->pitch[0] * pInputFrame->height);
        return RGY_ERR_NONE;
    }
    //フィルタチェーン更新
    auto frameDevIn  = m_dev[0].get();
    auto frameDevOut = m_dev[1].get();
    if ((err = filterChainCreate(&frameDevIn->frame, &frameDevOut->frame, recreate_filter_chain)) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to update filter chain.\n"));
        return err;
    }

    if ((err = frameDevIn->queueMapBuffer(m_cl->queue(), CL_MAP_WRITE)) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to queue map input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    frameDevIn->mapEvent().wait();

    {
        auto& frameHostIn = frameDevIn->mappedHost();

        //YC48->YUV444(16bit)
        int crop[4] = { 0 };
        m_convert_yc48_to_yuv444_16->run(false,
            (void **)frameHostIn.ptr, (const void **)&pInputFrame->ptr[0],
            pInputFrame->width, pInputFrame->pitch[0], pInputFrame->pitch[0],
            frameHostIn.pitch[0], pInputFrame->height, frameHostIn.height, crop);
    }

    if ((err = frameDevIn->unmapBuffer()) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to unmap input buffer: %s.\n"), get_err_mes(err));
        return err;
    }

    //通常は、最後のひとつ前のフィルタまで実行する
    //上書き型のフィルタが最後の場合は、そのフィルタまで実行する(最後はコピーが必須)
    const auto filterfin = (m_filters.back()->GetFilterParam()->bOutOverwrite) ? m_filters.size() : m_filters.size() - 1;
    //フィルタチェーン実行
    auto frameInfo = frameDevIn->frame;
    for (size_t ifilter = 0; ifilter < filterfin; ifilter++) {
        int nOutFrames = 0;
        RGYFrameInfo *outInfo[16] = { 0 };
        err = m_filters[ifilter]->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error while running filter \"%s\": %s.\n"), m_filters[ifilter]->name().c_str(), get_err_mes(err));
            return err;
        }
        if (nOutFrames > 1) {
            PrintMes(RGY_LOG_ERROR, _T("Currently only simple filters are supported.\n"));
            return RGY_ERR_UNSUPPORTED;
        }
        frameInfo = *(outInfo[0]);
    }
    //最後のフィルタ
    if (m_filters.back()->GetFilterParam()->bOutOverwrite) {
        if ((err = m_cl->copyFrame(&frameDevOut->frame, &frameInfo)) != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error in frame copy: %s.\n"), get_err_mes(err));
            return err;
        }
    } else {
        auto& lastFilter = m_filters[m_filters.size() - 1];
        int nOutFrames = 0;
        RGYFrameInfo *outInfo[16] = { 0 };
        outInfo[0] = &frameDevOut->frame;
        err = lastFilter->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error while running filter \"%s\": %s.\n"), lastFilter->name().c_str(), get_err_mes(err));
            return err;
        }
    }
    if ((err = frameDevOut->queueMapBuffer(m_cl->queue(), CL_MAP_READ)) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to queue map input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    frameDevOut->mapEvent().wait();
    {
        auto& frameHostOut = frameDevOut->mappedHost();

        //YUV444(16bit)->YC48
        int crop[4] = { 0 };
        m_convert_yuv444_16_to_yc48->run(false,
            (void **)&pOutputFrame->ptr[0], (const void **)frameHostOut.ptr,
            frameHostOut.width, frameHostOut.pitch[0], frameHostOut.pitch[0],
            pOutputFrame->pitch[0], frameHostOut.height, pOutputFrame->height, crop);
    }

    if ((err = frameDevOut->unmapBuffer()) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to unmap output buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}
