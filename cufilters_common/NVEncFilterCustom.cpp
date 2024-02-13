﻿

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

#include <array>
#include "convert_csp.h"
#include "rgy_filesystem.h"
#include "NVEncFilterCustom.h"
#include "NVEncFilterParam.h"
#pragma warning (push)
#pragma warning (disable: 4819)
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#pragma warning (pop)

const char *NVEncFilterCustom::KERNEL_NAME = "kernel_filter";

NVEncFilterCustom::NVEncFilterCustom()
#if ENABLE_NVRTC
    : m_kernel_cache(), m_program()
#endif //#if ENABLE_NVRTC
{
    m_name = _T("custom");
}

NVEncFilterCustom::~NVEncFilterCustom() {
    close();
}

RGY_ERR NVEncFilterCustom::check_param(shared_ptr<NVEncFilterParamCustom> prm) {
    prm->frameOut.width  = (prm->custom.dstWidth <= 0)  ? prm->frameIn.width  : prm->custom.dstWidth;
    prm->frameOut.height = (prm->custom.dstHeight <= 0) ? prm->frameIn.height : prm->custom.dstHeight;
    if (prm->frameOut.height <= 0 || prm->frameOut.width <= 0) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->custom.kernel_interface <= VPP_CUSTOM_INTERFACE_PER_PLANE || VPP_CUSTOM_INTERFACE_MAX <= prm->custom.kernel_interface) {
        AddMessage(RGY_LOG_ERROR, _T("invalid value for param \"interface\": %d\n"), prm->custom.kernel_interface);
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->custom.interlace <= VPP_CUSTOM_INTERLACE_UNSUPPORTED || VPP_CUSTOM_INTERLACE_MAX <= prm->custom.interlace) {
        AddMessage(RGY_LOG_ERROR, _T("invalid value for param \"interlace\": %d\n"), prm->custom.interlace);
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->custom.threadPerBlockX <= 0) {
        AddMessage(RGY_LOG_WARN, _T("invalid value for param \"threadPerBlockX\": %d, changing to default value = 32\n"), prm->custom.threadPerBlockX);
        prm->custom.threadPerBlockX = FILTER_DEFAULT_CUSTOM_THREAD_PER_BLOCK_X;
    }
    if (prm->custom.threadPerBlockX <= 0) {
        int newVal = std::max(1, (FILTER_DEFAULT_CUSTOM_THREAD_PER_BLOCK_X * FILTER_DEFAULT_CUSTOM_THREAD_PER_BLOCK_Y) / prm->custom.threadPerBlockX);
        AddMessage(RGY_LOG_WARN, _T("invalid value for param \"threadPerBlockX\": %d, changing to %d\n"), prm->custom.threadPerBlockY, newVal);
        prm->custom.threadPerBlockY = newVal;
    }
    int device = 0;
    cudaGetDevice(&device);
    int maxThreadsPerBlock = 0;
    auto cuErr = cudaDeviceGetAttribute(&maxThreadsPerBlock, cudaDevAttrMaxThreadsPerBlock, device);
    if (cuErr == cudaErrorInvalidDevice || cuErr == cudaErrorInvalidValue) {
        auto sts = err_to_rgy(cuErr);
        AddMessage(RGY_LOG_ERROR, _T("Error on cudaDeviceGetAttribute(): %s\n"), get_err_mes(sts));
        return sts;
    }
    if (cuErr == cudaSuccess && maxThreadsPerBlock < prm->custom.threadPerBlockX * prm->custom.threadPerBlockY) {
        AddMessage(RGY_LOG_ERROR, _T("threadPerBlock is over limit of device: %d=%dx%d, limit=%d\n"),
            prm->custom.pixelPerThreadX * prm->custom.pixelPerThreadY,
            prm->custom.pixelPerThreadX, prm->custom.pixelPerThreadY,
            maxThreadsPerBlock);
        return RGY_ERR_INVALID_PARAM;
    }

    if (prm->custom.pixelPerThreadX <= 0) {
        AddMessage(RGY_LOG_ERROR, _T("invalid value for param \"pixelPerThreadX\": %d\n"), prm->custom.pixelPerThreadX);
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->custom.pixelPerThreadY <= 0) {
        AddMessage(RGY_LOG_ERROR, _T("invalid value for param \"pixelPerThreadY\": %d\n"), prm->custom.pixelPerThreadY);
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->custom.kernel.length() == 0 && !rgy_file_exists(prm->custom.kernel_path.c_str())) {
        AddMessage(RGY_LOG_ERROR, _T("custom kernel not specified.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    return RGY_ERR_NONE;
}

RGY_ERR NVEncFilterCustom::init(shared_ptr<NVEncFilterParam> pParam, shared_ptr<RGYLog> pPrintMes) {
    RGY_ERR sts = RGY_ERR_NONE;
    m_pLog = pPrintMes;
    auto prm = std::dynamic_pointer_cast<NVEncFilterParamCustom>(pParam);
    m_name = prm->custom.filter_name;
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
#if ENABLE_NVRTC
    if (initNVRTCGlobal()) {
        AddMessage(RGY_LOG_ERROR, _T("--vpp-custom(%s) requires \"%s\" and \"%s\", not available on your system.\n"), prm->custom.filter_name.c_str(), NVRTC_DLL_NAME_TSTR, NVRTC_BUILTIN_DLL_NAME_TSTR);
        return RGY_ERR_UNSUPPORTED;
    }
    AddMessage(RGY_LOG_DEBUG, _T("%s available.\n"), NVRTC_DLL_NAME_TSTR);

    sts = AllocFrameBuf(pParam->frameOut, 1);
    if (sts != RGY_ERR_NONE) {
        AddMessage(RGY_LOG_ERROR, _T("failed to allocate memory: %s.\n"), get_err_mes(sts));
        return sts;
    }
    for (int i = 0; i < RGY_CSP_PLANES[pParam->frameOut.csp]; i++) {
        pParam->frameOut.pitch[i] = m_frameBuf[0]->frame.pitch[i];
    }

    std::string program_source;
    if (prm->custom.kernel.length() > 0) {
        program_source = tchar_to_string(prm->custom.filter_name) + "\n" + prm->custom.kernel;
        AddMessage(RGY_LOG_DEBUG, _T("program source...\n%s\n"), prm->custom.kernel.c_str());
    } else {
        program_source = tchar_to_string(prm->custom.kernel_path);
        AddMessage(RGY_LOG_DEBUG, _T("program source will be read from \"%s\".\n"), prm->custom.kernel_path.c_str());
    }
    try {
        m_program.reset(new jitify::Program(m_kernel_cache, program_source, 0, split(prm->custom.compile_options, " ", true)));
    } catch (const std::exception& e) {
        AddMessage(RGY_LOG_ERROR, _T("failed to build program source.\n%s\n"), char_to_tstring(e.what()).c_str());
        return RGY_ERR_CUDA;
    }
    m_pLog->write_log(RGY_LOG_DEBUG, RGY_LOGT_VPP_BUILD, char_to_tstring(m_program->getLog()).c_str());

    //test compile
    std::string compile_log;
    try {
        if (RGY_CSP_BIT_DEPTH[pParam->frameOut.csp] > 8) {
            compile_log = m_program->kernel(KERNEL_NAME).instantiateLog(jitify::reflection::Type<uint16_t>());
        } else {
            compile_log = m_program->kernel(KERNEL_NAME).instantiateLog(jitify::reflection::Type<uint8_t>());
        }
    } catch (const std::exception& e) {
        AddMessage(RGY_LOG_ERROR, _T("failed to instantiate program source.\n%s\n"), char_to_tstring(e.what()).c_str());
        m_pLog->write_log(RGY_LOG_ERROR, RGY_LOGT_VPP_BUILD, char_to_tstring(compile_log).c_str());
        return RGY_ERR_CUDA;
    }
    m_pLog->write_log(RGY_LOG_DEBUG, RGY_LOGT_VPP_BUILD, char_to_tstring(compile_log).c_str());

    setFilterInfo(pParam->print());
    m_param = pParam;
    return sts;
#else
    AddMessage(RGY_LOG_ERROR, _T("--vpp-custom(%s) is not supported on this build.\n"), prm->custom.filter_name.c_str());
    return RGY_ERR_UNSUPPORTED;
#endif
}

tstring NVEncFilterParamCustom::print() const {
    tstring info = custom.print();
    info += strsprintf(_T("                    %dx%d %s\n"), frameIn.width, frameIn.height, RGY_CSP_NAMES[frameIn.csp]);
    if (custom.dstWidth > 0 || custom.dstHeight > 0) {
        info += strsprintf(_T("                    output res %dx%d\n"),
            frameOut.width, frameOut.height);
    };
    return info;
}

RGY_ERR NVEncFilterCustom::run_per_plane(RGYFrameInfo *pOutputPlane, const RGYFrameInfo *pInpuPlane, RGY_PLANE plane, cudaStream_t stream) {
#if ENABLE_NVRTC
    auto prm = std::dynamic_pointer_cast<NVEncFilterParamCustom>(m_param);
    const dim3 blockSize(prm->custom.threadPerBlockX, prm->custom.threadPerBlockY);
    const dim3 gridSize(
        divCeil(pOutputPlane->width, blockSize.x * prm->custom.pixelPerThreadX),
        divCeil(pOutputPlane->height, blockSize.y * prm->custom.pixelPerThreadY));
    AddMessage(RGY_LOG_TRACE, _T("thread/block(%d,%d), grid(%d,%d)\n"), blockSize.x, blockSize.y, gridSize.x, gridSize.y);

    CUresult err;
    if (RGY_CSP_BIT_DEPTH[pOutputPlane->csp] > 8) {
        AddMessage(RGY_LOG_TRACE, _T("run kernel_filter [type=uint16_t]\n"));
        err = m_program->kernel(KERNEL_NAME).instantiate(jitify::reflection::Type<uint16_t>()).configure(gridSize, blockSize, 0, stream).launch(
            pOutputPlane->ptr[0], pOutputPlane->pitch[0], pOutputPlane->width, pOutputPlane->height,
            pInpuPlane->ptr[0], pInpuPlane->pitch[0], pInpuPlane->width, pInpuPlane->height, interlaced(*pInpuPlane), prm->custom.dev_params, plane);
    } else {
        AddMessage(RGY_LOG_TRACE, _T("run kernel_filter [type=uint8_t]\n"));
        err = m_program->kernel(KERNEL_NAME).instantiate(jitify::reflection::Type<uint8_t>()).configure(gridSize, blockSize, 0, stream).launch(
            pOutputPlane->ptr[0], pOutputPlane->pitch[0], pOutputPlane->width, pOutputPlane->height,
            pInpuPlane->ptr[0], pInpuPlane->pitch[0], pInpuPlane->width, pInpuPlane->height, interlaced(*pInpuPlane), prm->custom.dev_params, plane);
    }
    if (err != CUDA_SUCCESS) {
        const char *ptr;
        cuGetErrorString(err, &ptr);
        AddMessage(RGY_LOG_ERROR, _T("error at run_per_plane(%s): %s.\n"),
            RGY_CSP_NAMES[pInpuPlane->csp], char_to_tstring(ptr).c_str());
        return err_to_rgy(err);
    }
    auto cudaerr = cudaGetLastError();
    if (cudaerr != cudaSuccess) {
        auto sts = err_to_rgy(cudaerr);
        AddMessage(RGY_LOG_ERROR, _T("error at run_per_plane(%s) kernel_filter: %s.\n"),
            RGY_CSP_NAMES[pInpuPlane->csp], get_err_mes(sts));
        return sts;
    }
    return RGY_ERR_NONE;
#else
    return RGY_ERR_UNSUPPORTED;
#endif
}

RGY_ERR NVEncFilterCustom::run_per_plane(RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame, cudaStream_t stream) {
    const auto planeInputY = getPlane(pInputFrame, RGY_PLANE_Y);
    const auto planeInputU = getPlane(pInputFrame, RGY_PLANE_U);
    const auto planeInputV = getPlane(pInputFrame, RGY_PLANE_V);
    auto planeOutputY = getPlane(pOutputFrame, RGY_PLANE_Y);
    auto planeOutputU = getPlane(pOutputFrame, RGY_PLANE_U);
    auto planeOutputV = getPlane(pOutputFrame, RGY_PLANE_V);

    auto err = run_per_plane(&planeOutputY, &planeInputY, RGY_PLANE_Y, stream);
    if (err != RGY_ERR_NONE) return err;

    err = run_per_plane(&planeOutputY, &planeInputY, RGY_PLANE_U, stream);
    if (err != RGY_ERR_NONE) return err;

    err = run_per_plane(&planeOutputY, &planeInputY, RGY_PLANE_V, stream);
    if (err != RGY_ERR_NONE) return err;

    return RGY_ERR_NONE;
}

RGY_ERR NVEncFilterCustom::run_planes(RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame, cudaStream_t stream) {
#if ENABLE_NVRTC
    auto prm = std::dynamic_pointer_cast<NVEncFilterParamCustom>(m_param);
    const auto planeInputY = getPlane(pInputFrame, RGY_PLANE_Y);
    const auto planeInputU = getPlane(pInputFrame, RGY_PLANE_U);
    const auto planeInputV = getPlane(pInputFrame, RGY_PLANE_V);
    auto planeOutputY = getPlane(pOutputFrame, RGY_PLANE_Y);
    auto planeOutputU = getPlane(pOutputFrame, RGY_PLANE_U);
    auto planeOutputV = getPlane(pOutputFrame, RGY_PLANE_V);
    const bool interlacedFrame = interlaced(*pInputFrame);
    const dim3 blockSize(prm->custom.threadPerBlockX, prm->custom.threadPerBlockY);
    const dim3 gridSize(
        divCeil(pOutputFrame->width, blockSize.x * prm->custom.pixelPerThreadX),
        divCeil(pOutputFrame->height, blockSize.y * prm->custom.pixelPerThreadY));
    AddMessage(RGY_LOG_TRACE, _T("thread/block(%d,%d), grid(%d,%d)\n"), blockSize.x, blockSize.y, gridSize.x, gridSize.y);

    CUresult err;
    if (RGY_CSP_BIT_DEPTH[pOutputFrame->csp] > 8) {
        AddMessage(RGY_LOG_TRACE, _T("run kernel_filter [type=uint16_t]\n"));
        err = m_program->kernel(KERNEL_NAME).instantiate(jitify::reflection::Type<uint16_t>()).configure(gridSize, blockSize, 0, stream).launch(
            planeOutputY.ptr[0], planeOutputU.ptr[0], planeOutputV.ptr[0], planeOutputY.pitch[0], planeOutputY.width, planeOutputY.height,
            planeInputY.ptr[0], planeInputU.ptr[0], planeInputV.ptr[0], planeInputY.pitch[0], planeInputY.width, planeInputY.height,
            interlacedFrame, prm->custom.dev_params);
    } else {
        AddMessage(RGY_LOG_TRACE, _T("run kernel_filter [type=uint8_t]\n"));
        err = m_program->kernel(KERNEL_NAME).instantiate(jitify::reflection::Type<uint8_t>()).configure(gridSize, blockSize, 0, stream).launch(
            planeOutputY.ptr[0], planeOutputU.ptr[0], planeOutputV.ptr[0], planeOutputY.pitch[0], planeOutputY.width, planeOutputY.height,
            planeInputY.ptr[0], planeInputU.ptr[0], planeInputV.ptr[0], planeInputY.pitch[0], planeInputY.width, planeInputY.height,
            interlacedFrame, prm->custom.dev_params);
    }
    if (err != CUDA_SUCCESS) {
        const char *ptr;
        cuGetErrorString(err, &ptr);
        AddMessage(RGY_LOG_ERROR, _T("error at run_planes(%s): %s.\n"),
            RGY_CSP_NAMES[pInputFrame->csp], char_to_tstring(ptr).c_str());
        return err_to_rgy(err);
    }
    auto cudaerr = cudaGetLastError();
    if (cudaerr != cudaSuccess) {
        auto sts = err_to_rgy(cudaerr);
        AddMessage(RGY_LOG_ERROR, _T("error at run_planes(%s) kernel_filter: %s.\n"),
            RGY_CSP_NAMES[pInputFrame->csp], get_err_mes(sts));
        return sts;
    }
    return RGY_ERR_NONE;
#else
    return RGY_ERR_UNSUPPORTED;
#endif
}

RGY_ERR NVEncFilterCustom::run_filter(const RGYFrameInfo *pInputFrame, RGYFrameInfo **ppOutputFrames, int *pOutputFrameNum, cudaStream_t stream) {
    RGY_ERR sts = RGY_ERR_NONE;

    if (pInputFrame->ptr[0] == nullptr) {
        return sts;
    }

    *pOutputFrameNum = 1;
    if (ppOutputFrames[0] == nullptr) {
        auto pOutFrame = m_frameBuf[m_nFrameIdx].get();
        ppOutputFrames[0] = &pOutFrame->frame;
        m_nFrameIdx = (m_nFrameIdx + 1) % m_frameBuf.size();
    }
    ppOutputFrames[0]->picstruct = pInputFrame->picstruct;
    const auto memcpyKind = getCudaMemcpyKind(pInputFrame->mem_type, ppOutputFrames[0]->mem_type);
    if (memcpyKind != cudaMemcpyDeviceToDevice) {
        AddMessage(RGY_LOG_ERROR, _T("only supported on device memory.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    auto prm = std::dynamic_pointer_cast<NVEncFilterParamCustom>(m_param);
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }

    auto pOutputFrame = ppOutputFrames[0];
    if (false) {//for debug
        sts = copyFrameAsync(pOutputFrame, pInputFrame, stream);
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("error to copy frames: %s.\n"),
                RGY_CSP_NAMES[pInputFrame->csp], get_err_mes(sts));
            return sts;
        }
    } else if (prm->custom.kernel_interface == VPP_CUSTOM_INTERFACE_PLANES) {
        sts = run_planes(pOutputFrame, pInputFrame, stream);
    } else {
        sts = run_per_plane(pOutputFrame, pInputFrame, stream);
    }
    return sts;
}

void NVEncFilterCustom::close() {
    m_frameBuf.clear();
}
