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

#include "clcufilters_version.h"
#include "cufilters_chain.h"
#include "rgy_cmd.h"
#include "NVEncFilterColorspace.h"
#include "NVEncFilterNnedi.h"
#include "NVEncFilterDenoiseKnn.h"
#include "NVEncFilterDenoisePmd.h"
#include "NVEncFilterSmooth.h"
#include "NVEncFilterUnsharp.h"
#include "NVEncFilterEdgelevel.h"
#include "NVEncFilterWarpsharp.h"
#include "NVEncFilterDeband.h"
#include "NVEncFilterTweak.h"

cuFilterFrameBuffer::cuFilterFrameBuffer() :
    clcuFilterFrameBuffer() {

}

cuFilterFrameBuffer::~cuFilterFrameBuffer() {
}

std::unique_ptr<RGYFrame> cuFilterFrameBuffer::allocateFrame(const int width, const int height) {
    auto uptr = std::make_unique<CUFrameBuf>(width, height, RGY_CSP_YUV444_16);
    if (uptr->alloc() != RGY_ERR_NONE) {
        uptr.reset();
    }
    return uptr;
}

void cuFilterFrameBuffer::resetMappedFrame(RGYFrame *frame) {

}

cuDevice::cuDevice() :
    m_deviceID(-1),
    m_deviceName(),
    m_cuda_driver_version(0),
    m_cuda_version() {}

cuDevice::~cuDevice() {
    close();
}

void cuDevice::close() {
    m_log.reset();
    m_deviceID = -1;
    m_deviceName.clear();
}

void cuDevice::PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...) {
    va_list args;
    va_start(args, format);

    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    vector<TCHAR> buffer(len, 0);
    _vstprintf_s(buffer.data(), len, format, args);
    va_end(args);

    m_log->write_log(logLevel, RGY_LOGT_APP, buffer.data());
}

RGY_ERR cuDevice::init(const int deviceID, std::shared_ptr<RGYLog> log) {
    m_log = log;
    m_deviceID = deviceID;

    //ひとまず、これまでのすべてのエラーをflush
    cudaGetLastError();

    int deviceCount = 0;
    auto sts = err_to_rgy(cuDeviceGetCount(&deviceCount));
    if (sts != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("cuDeviceGetCount error: %s\n"), get_err_mes(sts));
        return sts;
    }
    if (deviceCount == 0) {
        PrintMes(RGY_LOG_ERROR, _T("Error: no CUDA device.\n"));
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    PrintMes(RGY_LOG_DEBUG, _T("cuDeviceGetCount: Success, %d.\n"), deviceCount);

    if (m_deviceID > deviceCount - 1) {
        PrintMes(RGY_LOG_ERROR, _T("Invalid Device Id = %d\n"), m_deviceID);
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    CUdevice cuDevice = 0;
    sts = err_to_rgy(cuDeviceGet(&cuDevice, m_deviceID));
    if (sts != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("  Error: cuDeviceGet(%d): %s\n"), m_deviceID, get_err_mes(sts));
        return RGY_ERR_DEVICE_NOT_FOUND;
    }
    PrintMes(RGY_LOG_DEBUG, _T("  cuDeviceGet(%d): success\n"), m_deviceID);

    char dev_name[256] = { 0 };
    if ((sts = err_to_rgy(cuDeviceGetName(dev_name, _countof(dev_name), cuDevice))) != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("  Error: cuDeviceGetName(%d): %s\n"), m_deviceID, get_err_mes(sts));
        return RGY_ERR_DEVICE_NOT_AVAILABLE;
    }
    PrintMes(RGY_LOG_DEBUG, _T("  cuDeviceGetName(%d): %s\n"), m_deviceID, char_to_tstring(dev_name).c_str());

#define GETATTRIB_CHECK(val, attrib, dev) { \
        auto cuErr = cudaDeviceGetAttribute(&(val), (attrib), (dev)); \
        if (cuErr == cudaErrorInvalidDevice || cuErr == cudaErrorInvalidValue) { \
            auto sts = err_to_rgy(cuErr); \
            PrintMes(RGY_LOG_ERROR, _T("  Error: cudaDeviceGetAttribute(): %s\n"), get_err_mes(sts)); \
            return sts; \
        } \
        if (cuErr != cudaSuccess) { \
            auto sts = err_to_rgy(cuErr); \
            PrintMes(RGY_LOG_ERROR, _T("  Warn: cudaDeviceGetAttribute(): %s\n"), get_err_mes(sts)); \
            val = 0; \
        } \
    }
    int cudaDevMajor = 0, cudaDevMinor = 0;
    GETATTRIB_CHECK(cudaDevMajor, cudaDevAttrComputeCapabilityMajor, m_deviceID);
    GETATTRIB_CHECK(cudaDevMinor, cudaDevAttrComputeCapabilityMinor, m_deviceID);

    if (((cudaDevMajor << 4) + cudaDevMinor) < 0x30) {
        PrintMes(RGY_LOG_ERROR, _T("  Error: device does not satisfy required CUDA version (>=3.0): %d.%d\n"), cudaDevMajor, cudaDevMinor);
        return RGY_ERR_UNSUPPORTED;
    }
    PrintMes(RGY_LOG_DEBUG, _T("  cudaDeviceGetAttribute: CUDA %d.%d\n"), cudaDevMajor, cudaDevMinor);
    m_cuda_version.first = cudaDevMajor;
    m_cuda_version.second = cudaDevMinor;

    m_cuda_driver_version = 0;
    if (RGY_ERR_NONE != (sts = err_to_rgy(cuDriverGetVersion(&m_cuda_driver_version)))) {
        m_cuda_driver_version = -1;
    }
    PrintMes(RGY_LOG_DEBUG, _T("  CUDA Driver version: %d.\n"), m_cuda_driver_version);

    m_deviceName = char_to_tstring(dev_name);
    PrintMes(RGY_LOG_INFO, _T("created initialized CUDA, selcted device %s.\n"), m_deviceName.c_str());
}

cuFilterChain::cuFilterChain() :
    clcuFilterChain(),
    m_cuDevice(),
    m_frameHostIn(),
    m_frameHostOut(),
    m_eventIn(),
    m_eventOut(),
    m_streamIn(),
    m_streamOut() {

}

cuFilterChain::~cuFilterChain() {
    close();
}

void cuFilterChain::close() {
    m_filters.clear();
    m_frameIn.reset();
    m_streamIn.reset();
    m_streamOut.reset();
    m_eventIn.reset();
    m_eventOut.reset();
    //m_queueSendIn.finish(); // m_frameIn.reset() のあと
    //m_queueSendIn.clear();  // m_frameIn.reset() のあと
    m_frameOut.reset();
    m_deviceName.clear();
    m_convert_yc48_to_yuv444_16.reset();
    m_convert_yuv444_16_to_yc48.reset();
    m_log.reset();
}

RGY_ERR cuFilterChain::initDevice(const clcuFilterDeviceParam *param) {
    PrintMes(RGY_LOG_INFO, _T("start init CUDA device %d\n"), param->deviceID);
    m_cuDevice = std::make_unique<cuDevice>();
    auto err = m_cuDevice->init(param->deviceID, m_log);
    if (err != RGY_ERR_NONE) {
        return err;
    }

    m_deviceID = param->deviceID;
    m_deviceName = m_cuDevice->getDeviceName();
    m_frameIn = std::make_unique<cuFilterFrameBuffer>();
    m_frameOut = std::make_unique<cuFilterFrameBuffer>();
    m_streamIn = std::unique_ptr<cudaStream_t, cudastream_deleter>(new cudaStream_t(), cudastream_deleter());
    if (RGY_ERR_NONE != (err = err_to_rgy(cudaStreamCreateWithFlags(m_streamIn.get(), cudaStreamNonBlocking)))) {
        PrintMes(RGY_LOG_ERROR, _T("failed to cudaStreamCreateWithFlags: %s.\n"), get_err_mes(err));
        return err;
    }
    m_streamOut = std::unique_ptr<cudaStream_t, cudastream_deleter>(new cudaStream_t(), cudastream_deleter());
    if (RGY_ERR_NONE != (err = err_to_rgy(cudaStreamCreateWithFlags(m_streamOut.get(), cudaStreamNonBlocking)))) {
        PrintMes(RGY_LOG_ERROR, _T("failed to cudaStreamCreateWithFlags: %s.\n"), get_err_mes(err));
        return err;
    }
    const uint32_t cudaEventFlags = 0; // (pAfsParam->cudaSchedule & CU_CTX_SCHED_BLOCKING_SYNC) ? cudaEventBlockingSync : 0;
    m_eventIn = std::unique_ptr<cudaEvent_t, cudaevent_deleter>(new cudaEvent_t(), cudaevent_deleter());
    if (RGY_ERR_NONE != (err = err_to_rgy(cudaEventCreateWithFlags(m_eventIn.get(), cudaEventFlags | cudaEventDisableTiming)))) {
        PrintMes(RGY_LOG_ERROR, _T("failed to cudaEventCreateWithFlags: %s.\n"), get_err_mes(err));
        return err;
    }
    m_eventOut = std::unique_ptr<cudaEvent_t, cudaevent_deleter>(new cudaEvent_t(), cudaevent_deleter());
    if (RGY_ERR_NONE != (err = err_to_rgy(cudaEventCreateWithFlags(m_eventOut.get(), cudaEventFlags | cudaEventDisableTiming)))) {
        PrintMes(RGY_LOG_ERROR, _T("failed to cudaEventCreateWithFlags: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}

RGY_ERR cuFilterChain::configureOneFilter(std::unique_ptr<RGYFilterBase>& filter, RGYFrameInfo& inputFrame, const VppType filterType, const int resizeWidth, const int resizeHeight) {
    // colorspace
    if (filterType == VppType::CL_COLORSPACE) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new NVEncFilterColorspace());
        }
        std::shared_ptr<NVEncFilterParamColorspace> param(new NVEncFilterParamColorspace());
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
            filter.reset(new NVEncFilterNnedi());
        }
        std::shared_ptr<NVEncFilterParamNnedi> param(new NVEncFilterParamNnedi());
        param->nnedi = m_prm.vpp.nnedi;
        param->hModule = nullptr;
        if (m_cuDevice) param->compute_capability = m_cuDevice->getCUDAVer();
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
            filter.reset(new NVEncFilterDenoiseKnn());
        }
        std::shared_ptr<NVEncFilterParamDenoiseKnn> param(new NVEncFilterParamDenoiseKnn());
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
    //ノイズ除去 (pmd)
    if (filterType == VppType::CL_DENOISE_PMD) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new NVEncFilterDenoisePmd());
        }
        std::shared_ptr<NVEncFilterParamDenoisePmd> param(new NVEncFilterParamDenoisePmd());
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
    if (filterType == VppType::CL_DENOISE_SMOOTH) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new NVEncFilterSmooth());
        }
        std::shared_ptr<NVEncFilterParamSmooth> param(new NVEncFilterParamSmooth());
        param->smooth = m_prm.vpp.smooth;
        if (m_cuDevice) param->compute_capability = m_cuDevice->getCUDAVer();
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
            filter.reset(new NVEncFilterResize());
        }
        std::shared_ptr<NVEncFilterParamResize> param(new NVEncFilterParamResize());
        param->interp = m_prm.vpp.resize_algo;
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
    if (filterType == VppType::CL_UNSHARP) {
        if (!filter) {
            //フィルタチェーンに追加
            filter.reset(new NVEncFilterUnsharp());
        }
        std::shared_ptr<NVEncFilterParamUnsharp> param(new NVEncFilterParamUnsharp());
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
            filter.reset(new NVEncFilterEdgelevel());
        }
        std::shared_ptr<NVEncFilterParamEdgelevel> param(new NVEncFilterParamEdgelevel());
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
            filter.reset(new NVEncFilterWarpsharp());
        }
        std::shared_ptr<NVEncFilterParamWarpsharp> param(new NVEncFilterParamWarpsharp());
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
            filter.reset(new NVEncFilterTweak());
        }
        std::shared_ptr<NVEncFilterParamTweak> param(new NVEncFilterParamTweak());
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
            filter.reset(new NVEncFilterDeband());
        }
        std::shared_ptr<NVEncFilterParamDeband> param(new NVEncFilterParamDeband());
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
    return RGY_ERR_NONE;
}

RGY_ERR cuFilterChain::sendInFrame(const RGYFrameInfo *pInputFrame) {
    if (!m_cuDevice) {
        return RGY_ERR_NULL_PTR;
    }

    auto frameDevIn = dynamic_cast<CUFrameBuf*>(m_frameIn->get_in(pInputFrame->width, pInputFrame->height));
    if (!frameDevIn) {
        return RGY_ERR_NULL_PTR;
    }
    m_frameIn->in_to_next();

    if (!m_frameHostIn || cmpFrameInfoCspResolution(&frameDevIn->frame, &m_frameHostIn->frame)) {
        m_frameHostIn = std::make_unique<CUFrameBuf>(pInputFrame->width, pInputFrame->height, RGY_CSP_YUV444_16);
        auto sts = m_frameHostIn->allocHost();
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to allocate frame for input buffer: %s.\n"), get_err_mes(sts));
            return RGY_ERR_MEMORY_ALLOC;
        }
    }

    copyFramePropWithoutCsp(&frameDevIn->frame, pInputFrame);

    {
        auto frameHostIn = &m_frameHostIn->frame;

        //YC48->YUV444(16bit)
        int crop[4] = { 0 };
        m_convert_yc48_to_yuv444_16->run(false,
            frameHostIn->ptr().data(), (const void **)&pInputFrame->ptr[0],
            pInputFrame->width, pInputFrame->pitch[0], pInputFrame->pitch[0],
            frameHostIn->pitch(0), pInputFrame->height, frameHostIn->height(), crop);
    }

    auto err = copyFrameAsync(&frameDevIn->frame, &m_frameHostIn->frame, *m_streamIn.get());
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to queue map input buffer: %s.\n"), get_err_mes(err));
        return err;
    }
    cudaEventRecord(frameDevIn->event, *m_streamIn.get());
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("sendInFrame: cudaEventRecord: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}

RGY_ERR cuFilterChain::getOutFrame(RGYFrameInfo *pOutputFrame) {
    if (!m_cuDevice) {
        return RGY_ERR_NULL_PTR;
    }

    auto frameDevOut = dynamic_cast<CUFrameBuf*>(m_frameOut->get_out(pOutputFrame->inputFrameId));
    if (!frameDevOut) {
        return RGY_ERR_NULL_PTR;
    }
    m_frameOut->out_to_next();

    if (!frameDevOut) {
        return RGY_ERR_OUT_OF_RANGE;
    }
    copyFramePropWithoutCsp(pOutputFrame, &frameDevOut->frame);
    auto err = err_to_rgy(cudaEventSynchronize(frameDevOut->event));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("getOutFrame: cudaEventSynchronize: %s.\n"), get_err_mes(err));
        return err;
    }
    {
        auto frameHostOut = &m_frameHostOut->frame;
        //YUV444(16bit)->YC48
        int crop[4] = { 0 };
        m_convert_yuv444_16_to_yc48->run(false,
            (void **)&pOutputFrame->ptr[0], (const void **)frameHostOut->ptr().data(),
            frameHostOut->width(), frameHostOut->pitch(0), frameHostOut->pitch(0),
            pOutputFrame->pitch[0], frameHostOut->height(), pOutputFrame->height, crop);
    }
    return RGY_ERR_NONE;
}

RGY_ERR cuFilterChain::proc(const int frameID, const clFilterChainParam& prm) {
    if (!m_cuDevice) {
        return RGY_ERR_NULL_PTR;
    }
    auto frameDevIn = dynamic_cast<CUFrameBuf*>(m_frameIn->get_out(frameID));
    m_frameIn->out_to_next();

    if (!frameDevIn) {
        return RGY_ERR_OUT_OF_RANGE;
    }
    m_log->setLogLevelAll(prm.log_level);
    m_log->setLogFile(prm.log_to_file ? LOG_FILE_NAME : nullptr);
    m_prm = prm;

    auto frameDevOut = dynamic_cast<CUFrameBuf*>(m_frameOut->get_in(prm.outWidth, prm.outHeight));
    m_frameOut->in_to_next();

    //フィルタチェーン更新
    auto err = filterChainCreate(&frameDevIn->frame, prm.outWidth, prm.outHeight);
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to update filter chain.\n"));
        return err;
    }

    auto err = err_to_rgy(cudaStreamWaitEvent(cudaStreamDefault, frameDevIn->event, 0));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("proc: cudaStreamWaitEvent: %s.\n"), get_err_mes(err));
        return err;
    }

    //通常は、最後のひとつ前のフィルタまで実行する
    //上書き型のフィルタが最後の場合は、そのフィルタまで実行する(最後はコピーが必須)
    const auto filterfin = (m_filters.back().second->GetFilterParam()->bOutOverwrite) ? m_filters.size() : m_filters.size() - 1;
    //フィルタチェーン実行
    auto frameInfo = frameDevIn->frame;
    for (size_t ifilter = 0; ifilter < filterfin; ifilter++) {
        int nOutFrames = 0;
        RGYFrameInfo *outInfo[16] = { 0 };
        auto cufilter = dynamic_cast<NVEncFilter*>(m_filters[ifilter].second.get());
        err = cufilter->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames, cudaStreamDefault);
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
        auto cufilter = dynamic_cast<NVEncFilter*>(lastFilter.second.get());
        err = cufilter->filter(&frameInfo, (RGYFrameInfo **)&outInfo, &nOutFrames, cudaStreamDefault);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Error while running filter \"%s\": %s.\n"), lastFilter.second->name().c_str(), get_err_mes(err));
            return err;
        }
    }

    if (!m_frameHostOut || cmpFrameInfoCspResolution(&frameDevOut->frame, &m_frameHostOut->frame)) {
        m_frameHostOut = std::make_unique<CUFrameBuf>(frameDevOut->frame.width, frameDevOut->frame.height, RGY_CSP_YUV444_16);
        auto sts = m_frameHostOut->allocHost();
        if (sts != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("failed to allocate frame for output buffer: %s.\n"), get_err_mes(sts));
            return RGY_ERR_MEMORY_ALLOC;
        }
    }

    err = err_to_rgy(cudaEventRecord(*m_eventOut.get(), cudaStreamDefault));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("proc: cudaEventRecord: %s.\n"), get_err_mes(err));
        return err;
    }
    err = err_to_rgy(cudaStreamWaitEvent(*m_streamOut.get(), *m_eventOut.get(), 0));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("proc: cudaStreamWaitEvent: %s.\n"), get_err_mes(err));
        return err;
    }
    err = copyFrameAsync(&m_frameHostOut->frame, &frameDevOut->frame, *m_streamOut.get());
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("failed to copy output frame: %s.\n"), get_err_mes(err));
        return err;
    }
    err = err_to_rgy(cudaEventRecord(m_frameHostOut->event, *m_streamOut.get()));
    if (err != RGY_ERR_NONE) {
        PrintMes(RGY_LOG_ERROR, _T("proc: cudaEventRecord: %s.\n"), get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}
