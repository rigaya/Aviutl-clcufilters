// -----------------------------------------------------------------------------------------
// clfilters by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2022-2024 rigaya
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

#include "clcufilters_version.h"
#include "clfilters_chain.h"
#include "clcufilters_chain_prm.h"
#include "rgy_cmd.h"
#include "rgy_filter_colorspace.h"
#include "rgy_filter_nnedi.h"
#include "rgy_filter_denoise_knn.h"
#include "rgy_filter_denoise_nlmeans.h"
#include "rgy_filter_denoise_pmd.h"
#include "rgy_filter_denoise_dct.h"
#include "rgy_filter_libplacebo.h"
#include "rgy_filter_smooth.h"
#include "rgy_filter_unsharp.h"
#include "rgy_filter_resize.h"
#include "rgy_filter_edgelevel.h"
#include "rgy_filter_warpsharp.h"
#include "rgy_filter_deband.h"
#include "rgy_filter_tweak.h"
#include "rgy_device.h"

static tstring luidToString(const void *uuid) {
    tstring str;
    const uint8_t *buf = (const uint8_t *)uuid;
    for (size_t i = 0; i < CL_LUID_SIZE_KHR; ++i) {
        char tmp[4];
        _stprintf_s(tmp, "%02x", buf[i]);
        str += tmp;
    }
    return str;
};

clFilterFrameBuffer::clFilterFrameBuffer(std::shared_ptr<RGYOpenCLContext> cl) :
    clcuFilterFrameBuffer(),
    m_cl(cl) {

}

clFilterFrameBuffer::~clFilterFrameBuffer() {
    freeFrames();
}

std::unique_ptr<RGYFrame> clFilterFrameBuffer::allocateFrame(const int width, const int height) {
    return m_cl->createFrameBuffer(width, height, RGY_CSP_YUV444_16, 16, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR);
}

void clFilterFrameBuffer::resetMappedFrame(RGYFrame *frame) {
    if (auto clFrame = dynamic_cast<RGYCLFrame *>(frame); clFrame) {
        clFrame->resetMappedFrame();
    }
}

clFilterChain::clFilterChain() :
    clcuFilterChain(),
    m_cl(),
    m_dx11(),
    m_platformID(-1),
    m_queueSendIn() {

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

RGY_ERR clFilterChain::initDevice(const clcuFilterDeviceParam *param) {
    auto prm = dynamic_cast<const clFilterDeviceParam *>(param);
    const int platformID = prm->platformID;
    const int deviceID = prm->deviceID;
    const cl_device_type device_type = prm->deviceType;
    PrintMes(RGY_LOG_INFO, _T("start init OpenCL platform %d, device %d\n"), platformID, deviceID);

    RGYOpenCL cl(m_log);
    auto platforms = cl.getPlatforms(nullptr);
    if (platforms.size() == 0) {
        PrintMes(RGY_LOG_ERROR, _T("No OpenCL Platform found on this system.\n"));
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    if (prm->noNVCL) {
        PrintMes(RGY_LOG_DEBUG, _T("noNVCL = true.\n"));
        for (auto it = platforms.begin(); it != platforms.end();) {
            if ((*it)->isVendor("NVIDIA")) {
                it = platforms.erase(it);
            } else {
                it++;
            }
        }
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

    uint8_t devLUID[CL_LUID_SIZE_KHR];
    memcpy(devLUID, platform->dev(std::max(deviceID, 0)).info().luid, sizeof(devLUID));
    PrintMes(RGY_LOG_INFO, _T("created OpenCL context, selcted device %d, LUID=%s.\n"), deviceID, luidToString(devLUID).c_str());

    const int dx11AdaptorCount = DeviceDX11::adapterCount();
    for (int iadaptor = 0; iadaptor < dx11AdaptorCount; iadaptor++) {
        auto dx11 = std::make_unique<DeviceDX11>();
        err = dx11->Init(iadaptor, m_log);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_DEBUG, _T("Failed to init DX11 device #%d: %s\n"), iadaptor, get_err_mes(err));
            return err;
        }
        const auto dx11UUID = dx11->getLUID();
        PrintMes(RGY_LOG_DEBUG, _T("Initialized DX11 device %d, LUID=%s.\n"), deviceID, luidToString(&dx11UUID).c_str());

        if (memcmp(&dx11UUID, devLUID, sizeof(dx11UUID)) == 0) {
            m_dx11 = std::move(dx11);
            PrintMes(RGY_LOG_INFO, _T("Selected DX11 device %d.\n"), deviceID, luidToString(devLUID).c_str());
            break;
        }
    }

    platform->setDev(platform->devs()[std::max(deviceID, 0)], nullptr, m_dx11->GetDevice());
    m_cl = std::make_shared<RGYOpenCLContext>(platform, m_log);
    if (m_cl->createContext(0) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to create OpenCL context.\n"));
        return RGY_ERR_UNKNOWN;
    }
    const auto& devInfo = platform->dev(0).info();

    m_deviceName = (devInfo.board_name_amd.length() > 0) ? devInfo.board_name_amd : devInfo.name;
    m_platformID = platformID;
    m_deviceID = deviceID;
    m_queueSendIn = m_cl->createQueue(platform->dev(0).id(), 0 /*CL_QUEUE_PROFILING_ENABLE*/);

    m_frameIn = std::make_unique<clFilterFrameBuffer>(m_cl);
    m_frameOut = std::make_unique<clFilterFrameBuffer>(m_cl);

    PrintMes(RGY_LOG_INFO, _T("created OpenCL context, selcted device %s.\n"), m_deviceName.c_str());
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::configureOneFilter(std::unique_ptr<RGYFilterBase>& filter, RGYFrameInfo& inputFrame, const VppType filterType, const int resizeWidth, const int resizeHeight) {
    // colorspace
    if (filterType == VppType::CL_COLORSPACE) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterColorspace(m_cl));
        }
        std::shared_ptr<RGYFilterParamColorspace> param(new RGYFilterParamColorspace());
        param->colorspace = m_prm.vpp.colorspace;
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
    if (filterType == VppType::CL_NNEDI) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterNnedi(m_cl));
        }
        std::shared_ptr<RGYFilterParamNnedi> param(new RGYFilterParamNnedi());
        param->nnedi = m_prm.vpp.nnedi;
        param->hModule = nullptr;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->timebase = rgy_rational<int>(); // bobで使用するが、clfiltersではbobはサポートしない
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
    if (filterType == VppType::CL_DENOISE_KNN) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDenoiseKnn(m_cl));
        }
        std::shared_ptr<RGYFilterParamDenoiseKnn> param(new RGYFilterParamDenoiseKnn());
        param->knn = m_prm.vpp.knn;
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
    //ノイズ除去 (nlmeans)
    if (filterType == VppType::CL_DENOISE_NLMEANS) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDenoiseNLMeans(m_cl));
        }
        std::shared_ptr<RGYFilterParamDenoiseNLMeans> param(new RGYFilterParamDenoiseNLMeans());
        param->nlmeans = m_prm.vpp.nlmeans;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init nlmeans.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }
    //ノイズ除去 (pmd)
    if (filterType == VppType::CL_DENOISE_PMD) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDenoisePmd(m_cl));
        }
        std::shared_ptr<RGYFilterParamDenoisePmd> param(new RGYFilterParamDenoisePmd());
        param->pmd = m_prm.vpp.pmd;
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
    if (filterType == VppType::CL_DENOISE_DCT) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDenoiseDct(m_cl));
        }
        std::shared_ptr<RGYFilterParamDenoiseDct> param(new RGYFilterParamDenoiseDct());
        param->dct = m_prm.vpp.dct;
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
    //ノイズ除去 (smooth)
    if (filterType == VppType::CL_DENOISE_SMOOTH) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterSmooth(m_cl));
        }
        std::shared_ptr<RGYFilterParamSmooth> param(new RGYFilterParamSmooth());
        param->smooth = m_prm.vpp.smooth;
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
    if (filterType == VppType::CL_RESIZE) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterResize(m_cl));
        }
        std::shared_ptr<RGYFilterParamResize> param(new RGYFilterParamResize());
        param->interp = m_prm.vpp.resize_algo;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->frameOut.width = resizeWidth;
        param->frameOut.height = resizeHeight;
        if (isLibplaceboResizeFiter(m_prm.vpp.resize_algo)) {
            param->libplaceboResample = std::make_shared<RGYFilterParamLibplaceboResample>();
            param->libplaceboResample->resample = m_prm.vpp.resize_libplacebo;
            //param->libplaceboResample->vui = VuiFiltered;
            //param->libplaceboResample->dx11 = m_dx11.get();
            //param->libplaceboResample->vk = m_dev->vulkan();
            param->libplaceboResample->resize_algo = m_prm.vpp.resize_algo;
        }
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
    if (filterType == VppType::CL_UNSHARP) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterUnsharp(m_cl));
        }
        std::shared_ptr<RGYFilterParamUnsharp> param(new RGYFilterParamUnsharp());
        param->unsharp = m_prm.vpp.unsharp;
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
    if (filterType == VppType::CL_EDGELEVEL) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterEdgelevel(m_cl));
        }
        std::shared_ptr<RGYFilterParamEdgelevel> param(new RGYFilterParamEdgelevel());
        param->edgelevel = m_prm.vpp.edgelevel;
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
    if (filterType == VppType::CL_WARPSHARP) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterWarpsharp(m_cl));
        }
        std::shared_ptr<RGYFilterParamWarpsharp> param(new RGYFilterParamWarpsharp());
        param->warpsharp = m_prm.vpp.warpsharp;
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
    if (filterType == VppType::CL_TWEAK) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterTweak(m_cl));
        }
        std::shared_ptr<RGYFilterParamTweak> param(new RGYFilterParamTweak());
        param->tweak = m_prm.vpp.tweak;
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
    if (filterType == VppType::CL_DEBAND) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterDeband(m_cl));
        }
        std::shared_ptr<RGYFilterParamDeband> param(new RGYFilterParamDeband());
        param->deband = m_prm.vpp.deband;
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
    //libplacebo-deband
    if (filterType == VppType::CL_LIBPLACEBO_DEBAND) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new RGYFilterLibplaceboDeband(m_cl));
        }
        std::shared_ptr<RGYFilterParamLibplaceboDeband> param(new RGYFilterParamLibplaceboDeband());
        param->deband = m_prm.vpp.libplacebo_deband;
        param->frameIn = inputFrame;
        param->frameOut = inputFrame;
        param->bOutOverwrite = false;
        auto sts = filter->init(param, m_log);
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to init libplacebo-deband.\n"));
            return sts;
        }
        //入力フレーム情報を更新
        inputFrame = param->frameOut;
    }

    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::sendInFrame(const RGYFrameInfo *pInputFrame) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevIn = dynamic_cast<RGYCLFrame*>(m_frameIn->get_in(pInputFrame->width, pInputFrame->height));
    if (!frameDevIn) {
        return RGY_ERR_NULL_PTR;
    }
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
            frameHostIn->pitch(RGY_PLANE_Y), pInputFrame->height, frameHostIn->height(), crop);
    }

    if ((err = frameDevIn->unmapBuffer()) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to unmap input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}

RGY_ERR clFilterChain::getOutFrame(RGYFrameInfo *pOutputFrame) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevOut = dynamic_cast<RGYCLFrame*>(m_frameOut->get_out(pOutputFrame->inputFrameId));
    if (!frameDevOut) {
        return RGY_ERR_NULL_PTR;
    }
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
            frameHostOut->width(), frameHostOut->pitch(RGY_PLANE_Y), frameHostOut->pitch(RGY_PLANE_Y),
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

RGY_ERR clFilterChain::proc(const int frameID, const clFilterChainParam& prm) {
    if (!m_cl) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevIn = dynamic_cast<RGYCLFrame*>(m_frameIn->get_out(frameID));
    m_frameIn->out_to_next();

    if (!frameDevIn) {
        return RGY_ERR_OUT_OF_RANGE;
    }
    m_cl->setModuleHandle(prm.hModule);
    m_log->setLogLevelAll(prm.log_level);
    m_log->setLogFile(prm.log_to_file ? LOG_FILE_NAME : nullptr);
    m_prm = prm;

    auto frameDevOut = dynamic_cast<RGYCLFrame*>(m_frameOut->get_in(prm.outWidth, prm.outHeight));
    m_frameOut->in_to_next();

    //フィルタチェーン更新
    auto err = filterChainCreate(&frameDevIn->frame, prm.outWidth, prm.outHeight);
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
        auto clfilter = dynamic_cast<RGYFilter*>(m_filters[ifilter].second.get());
        err = clfilter->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames);
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
        auto clfilter = dynamic_cast<RGYFilter*>(lastFilter.second.get());
        err = clfilter->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames);
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
