﻿// -----------------------------------------------------------------------------------------
// QSVEnc/NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2011-2016 rigaya
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

#include "rgy_err.h"
#include "rgy_osdep.h"

#if ENCODER_QSV
struct RGYErrMapMFX {
    RGY_ERR rgy;
    mfxStatus mfx;
};

#define MFX_MAP(x) { RGY_ ##x, MFX_ ##x }
static const RGYErrMapMFX ERR_MAP_MFX[] = {
    MFX_MAP(ERR_NONE),
    MFX_MAP(ERR_UNKNOWN),
    MFX_MAP(ERR_NULL_PTR),
    MFX_MAP(ERR_UNSUPPORTED),
    MFX_MAP(ERR_MEMORY_ALLOC),
    MFX_MAP(ERR_NOT_ENOUGH_BUFFER),
    MFX_MAP(ERR_INVALID_HANDLE),
    MFX_MAP(ERR_LOCK_MEMORY),
    MFX_MAP(ERR_NOT_INITIALIZED),
    MFX_MAP(ERR_NOT_FOUND),
    MFX_MAP(ERR_MORE_DATA),
    MFX_MAP(ERR_MORE_SURFACE),
    MFX_MAP(ERR_ABORTED),
    MFX_MAP(ERR_DEVICE_LOST),
    MFX_MAP(ERR_INCOMPATIBLE_VIDEO_PARAM),
    MFX_MAP(ERR_INVALID_VIDEO_PARAM),
    MFX_MAP(ERR_UNDEFINED_BEHAVIOR),
    MFX_MAP(ERR_DEVICE_FAILED),
    MFX_MAP(ERR_MORE_BITSTREAM),
    MFX_MAP(ERR_GPU_HANG),
    MFX_MAP(ERR_REALLOC_SURFACE),

    MFX_MAP(WRN_IN_EXECUTION),
    MFX_MAP(WRN_DEVICE_BUSY),
    MFX_MAP(WRN_VIDEO_PARAM_CHANGED),
    MFX_MAP(WRN_PARTIAL_ACCELERATION),
    MFX_MAP(WRN_INCOMPATIBLE_VIDEO_PARAM),
    MFX_MAP(WRN_VALUE_NOT_CHANGED),
    MFX_MAP(WRN_OUT_OF_RANGE),
    MFX_MAP(WRN_FILTER_SKIPPED),
    MFX_MAP(ERR_NONE_PARTIAL_OUTPUT),

    //MFX_MAP(PRINT_OPTION_DONE),
    //MFX_MAP(PRINT_OPTION_ERR),

    //MFX_MAP(ERR_INVALID_COLOR_FORMAT),

    MFX_MAP(ERR_MORE_DATA_SUBMIT_TASK),
};
#undef MFX_MAP

mfxStatus err_to_mfx(RGY_ERR err) {
    const RGYErrMapMFX *ERR_MAP_FIN = (const RGYErrMapMFX *)ERR_MAP_MFX + _countof(ERR_MAP_MFX);
    auto ret = std::find_if((const RGYErrMapMFX *)ERR_MAP_MFX, ERR_MAP_FIN, [err](RGYErrMapMFX map) {
        return map.rgy == err;
    });
    return (ret == ERR_MAP_FIN) ? MFX_ERR_UNKNOWN : ret->mfx;
}

RGY_ERR err_to_rgy(mfxStatus err) {
    const RGYErrMapMFX *ERR_MAP_FIN = (const RGYErrMapMFX *)ERR_MAP_MFX + _countof(ERR_MAP_MFX);
    auto ret = std::find_if((const RGYErrMapMFX *)ERR_MAP_MFX, ERR_MAP_FIN, [err](RGYErrMapMFX map) {
        return map.mfx == err;
    });
    return (ret == ERR_MAP_FIN) ? RGY_ERR_UNKNOWN : ret->rgy;
}
#endif //#if ENCODER_QSV

#if ENCODER_NVENC
struct RGYErrMapNV {
    RGY_ERR rgy;
    NVENCSTATUS nv;
};

static const RGYErrMapNV ERR_MAP_NV[] = {
    { RGY_ERR_NONE, NV_ENC_SUCCESS },
    { RGY_ERR_UNKNOWN, NV_ENC_ERR_GENERIC },
    { RGY_ERR_ACCESS_DENIED, NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY },
    { RGY_ERR_INVALID_PARAM, NV_ENC_ERR_INVALID_EVENT },
    { RGY_ERR_INVALID_PARAM, NV_ENC_ERR_INVALID_PARAM },
    { RGY_ERR_INVALID_PARAM, NV_ENC_ERR_UNSUPPORTED_PARAM },
    { RGY_ERR_NOT_ENOUGH_BUFFER, NV_ENC_ERR_NOT_ENOUGH_BUFFER },
    { RGY_ERR_NULL_PTR, NV_ENC_ERR_INVALID_PTR },
    { RGY_ERR_NULL_PTR, NV_ENC_ERR_OUT_OF_MEMORY },
    { RGY_ERR_UNSUPPORTED, NV_ENC_ERR_UNIMPLEMENTED },
    { RGY_ERR_UNSUPPORTED, NV_ENC_ERR_UNSUPPORTED_DEVICE },
    { RGY_ERR_UNSUPPORTED, NV_ENC_ERR_INVALID_CALL },
    { RGY_WRN_DEVICE_BUSY, NV_ENC_ERR_LOCK_BUSY },
    { RGY_WRN_DEVICE_BUSY, NV_ENC_ERR_ENCODER_BUSY },
    { RGY_ERR_NO_DEVICE, NV_ENC_ERR_NO_ENCODE_DEVICE },
    { RGY_ERR_NOT_INITIALIZED, NV_ENC_ERR_ENCODER_NOT_INITIALIZED },
    { RGY_ERR_INVALID_VERSION, NV_ENC_ERR_INVALID_VERSION },
    { RGY_ERR_INVALID_DEVICE, NV_ENC_ERR_INVALID_ENCODERDEVICE },
    { RGY_ERR_INVALID_DEVICE, NV_ENC_ERR_INVALID_DEVICE },
    { RGY_ERR_NO_DEVICE, NV_ENC_ERR_DEVICE_NOT_EXIST },
    { RGY_ERR_MORE_DATA, NV_ENC_ERR_NEED_MORE_INPUT },
    { RGY_ERR_MAP_FAILED, NV_ENC_ERR_MAP_FAILED, }
};

NVENCSTATUS err_to_nv(RGY_ERR err) {
    const RGYErrMapNV *ERR_MAP_FIN = (const RGYErrMapNV *)ERR_MAP_NV + _countof(ERR_MAP_NV);
    auto ret = std::find_if((const RGYErrMapNV *)ERR_MAP_NV, ERR_MAP_FIN, [err](const RGYErrMapNV map) {
        return map.rgy == err;
    });
    return (ret == ERR_MAP_FIN) ? NV_ENC_ERR_GENERIC : ret->nv;
}

RGY_ERR err_to_rgy(NVENCSTATUS err) {
    const RGYErrMapNV *ERR_MAP_FIN = (const RGYErrMapNV *)ERR_MAP_NV + _countof(ERR_MAP_NV);
    auto ret = std::find_if((const RGYErrMapNV *)ERR_MAP_NV, ERR_MAP_FIN, [err](const RGYErrMapNV map) {
        return map.nv == err;
    });
    return (ret == ERR_MAP_FIN) ? RGY_ERR_UNKNOWN : ret->rgy;
}


#if ENABLE_NVVFX

struct RGYErrMapNVCV {
    RGY_ERR rgy;
    NvCV_Status nvcv;
};

#define NVCVERR_MAP(x) { RGY_ERR_NVCV_ ##x, NVCV_ERR_ ##x }
static const RGYErrMapNVCV ERR_MAP_NVCV[] = {
  NVCVERR_MAP(GENERAL),
  NVCVERR_MAP(UNIMPLEMENTED),
  NVCVERR_MAP(MEMORY),
  NVCVERR_MAP(EFFECT),
  NVCVERR_MAP(SELECTOR),
  NVCVERR_MAP(BUFFER),
  NVCVERR_MAP(PARAMETER),
  NVCVERR_MAP(MISMATCH),
  NVCVERR_MAP(PIXELFORMAT),
  NVCVERR_MAP(MODEL),
  NVCVERR_MAP(LIBRARY),
  NVCVERR_MAP(INITIALIZATION),
  NVCVERR_MAP(FILE),
  NVCVERR_MAP(FEATURENOTFOUND),
  NVCVERR_MAP(MISSINGINPUT),
  NVCVERR_MAP(RESOLUTION),
  NVCVERR_MAP(UNSUPPORTEDGPU),
  NVCVERR_MAP(WRONGGPU),
  NVCVERR_MAP(UNSUPPORTEDDRIVER),
  NVCVERR_MAP(MODELDEPENDENCIES),
  NVCVERR_MAP(PARSE),
  NVCVERR_MAP(MODELSUBSTITUTION),
  NVCVERR_MAP(READ),
  NVCVERR_MAP(WRITE),
  NVCVERR_MAP(PARAMREADONLY),
  NVCVERR_MAP(TRT_ENQUEUE),
  NVCVERR_MAP(TRT_BINDINGS),
  NVCVERR_MAP(TRT_CONTEXT),
  NVCVERR_MAP(TRT_INFER),
  NVCVERR_MAP(TRT_ENGINE),
  NVCVERR_MAP(NPP),
  NVCVERR_MAP(CONFIG),
  NVCVERR_MAP(TOOSMALL),
  NVCVERR_MAP(TOOBIG),
  NVCVERR_MAP(WRONGSIZE),
  NVCVERR_MAP(OBJECTNOTFOUND),
  NVCVERR_MAP(SINGULAR),
  NVCVERR_MAP(NOTHINGRENDERED),
  NVCVERR_MAP(CONVERGENCE),
  NVCVERR_MAP(OPENGL),
  NVCVERR_MAP(DIRECT3D),
  NVCVERR_MAP(CUDA_BASE),
  NVCVERR_MAP(CUDA_VALUE),
  NVCVERR_MAP(CUDA_MEMORY),
  NVCVERR_MAP(CUDA_PITCH),
  NVCVERR_MAP(CUDA_INIT),
  NVCVERR_MAP(CUDA_LAUNCH),
  NVCVERR_MAP(CUDA_KERNEL),
  NVCVERR_MAP(CUDA_DRIVER),
  NVCVERR_MAP(CUDA_UNSUPPORTED),
  NVCVERR_MAP(CUDA_ILLEGAL_ADDRESS),
  NVCVERR_MAP(CUDA)
};
#undef MFX_MAP

NvCV_Status err_to_nvcv(RGY_ERR err) {
    if (err == RGY_ERR_NONE) return NVCV_SUCCESS;
    const RGYErrMapNVCV *ERR_MAP_FIN = (const RGYErrMapNVCV *)ERR_MAP_NVCV + _countof(ERR_MAP_NVCV);
    auto ret = std::find_if((const RGYErrMapNVCV *)ERR_MAP_NVCV, ERR_MAP_FIN, [err](const RGYErrMapNVCV map) {
        return map.rgy == err;
        });
    return (ret == ERR_MAP_FIN) ? NVCV_ERR_GENERAL : ret->nvcv;
}

RGY_ERR err_to_rgy(NvCV_Status err) {
    if (err == NVCV_SUCCESS) return RGY_ERR_NONE;
    const RGYErrMapNVCV *ERR_MAP_FIN = (const RGYErrMapNVCV *)ERR_MAP_NVCV + _countof(ERR_MAP_NVCV);
    auto ret = std::find_if((const RGYErrMapNVCV *)ERR_MAP_NVCV, ERR_MAP_FIN, [err](const RGYErrMapNVCV map) {
        return map.nvcv == err;
        });
    return (ret == ERR_MAP_FIN) ? RGY_ERR_UNKNOWN : ret->rgy;
}

#endif //#if ENABLE_NVVFX

cudaError err_to_cuda(RGY_ERR err) {
    return (err == RGY_ERR_NONE) ? cudaSuccess : cudaErrorUnknown;
}
RGY_ERR err_to_rgy(cudaError err) {
    return (err == cudaSuccess) ? RGY_ERR_NONE : RGY_ERR_CUDA;
}

#endif //#if ENCODER_NVENC

#if ENCODER_VCEENC
struct RGYErrMapAMF {
    RGY_ERR rgy;
    AMF_RESULT amf;
};

static const RGYErrMapAMF ERR_MAP_AMF[] = {
    { RGY_ERR_NONE, AMF_OK },
    { RGY_ERR_UNKNOWN, AMF_FAIL },
    { RGY_ERR_UNDEFINED_BEHAVIOR, AMF_UNEXPECTED },
    { RGY_ERR_ACCESS_DENIED, AMF_ACCESS_DENIED },
    { RGY_ERR_INVALID_PARAM, AMF_INVALID_ARG },
    { RGY_ERR_OUT_OF_RANGE, AMF_OUT_OF_RANGE },
    { RGY_ERR_NULL_PTR, AMF_INVALID_POINTER },
    { RGY_ERR_NULL_PTR, AMF_OUT_OF_MEMORY },
    { RGY_ERR_UNSUPPORTED, AMF_NO_INTERFACE },
    { RGY_ERR_UNSUPPORTED, AMF_NOT_IMPLEMENTED },
    { RGY_ERR_UNSUPPORTED, AMF_NOT_SUPPORTED },
    { RGY_ERR_NOT_FOUND, AMF_NOT_FOUND },
    { RGY_ERR_ALREADY_INITIALIZED, AMF_ALREADY_INITIALIZED },
    { RGY_ERR_NOT_INITIALIZED, AMF_NOT_INITIALIZED },
    { RGY_ERR_INVALID_FORMAT, AMF_INVALID_FORMAT },
    { RGY_ERR_WRONG_STATE, AMF_WRONG_STATE },
    { RGY_ERR_FILE_OPEN, AMF_FILE_NOT_OPEN },
    { RGY_ERR_NO_DEVICE, AMF_NO_DEVICE },
    { RGY_ERR_DEVICE_FAILED, AMF_DIRECTX_FAILED },
    { RGY_ERR_DEVICE_FAILED, AMF_OPENCL_FAILED },
    { RGY_ERR_DEVICE_FAILED, AMF_GLX_FAILED },
    { RGY_ERR_DEVICE_FAILED, AMF_ALSA_FAILED },
    { RGY_ERR_MORE_DATA, AMF_EOF },
    { RGY_ERR_MORE_BITSTREAM, AMF_EOF },
    { RGY_ERR_UNKNOWN, AMF_REPEAT },
    { RGY_ERR_INPUT_FULL, AMF_INPUT_FULL },
    { RGY_WRN_VIDEO_PARAM_CHANGED, AMF_RESOLUTION_CHANGED },
    { RGY_WRN_VIDEO_PARAM_CHANGED, AMF_RESOLUTION_UPDATED },
    { RGY_ERR_INVALID_DATA_TYPE, AMF_INVALID_DATA_TYPE },
    { RGY_ERR_INVALID_RESOLUTION, AMF_INVALID_RESOLUTION },
    { RGY_ERR_INVALID_CODEC, AMF_CODEC_NOT_SUPPORTED },
    { RGY_ERR_INVALID_COLOR_FORMAT, AMF_SURFACE_FORMAT_NOT_SUPPORTED },
    { RGY_ERR_DEVICE_FAILED, AMF_SURFACE_MUST_BE_SHARED }
};

AMF_RESULT err_to_amf(RGY_ERR err) {
    const RGYErrMapAMF *ERR_MAP_FIN = (const RGYErrMapAMF *)ERR_MAP_AMF + _countof(ERR_MAP_AMF);
    auto ret = std::find_if((const RGYErrMapAMF *)ERR_MAP_AMF, ERR_MAP_FIN, [err](const RGYErrMapAMF map) {
        return map.rgy == err;
    });
    return (ret == ERR_MAP_FIN) ? AMF_FAIL : ret->amf;
}

RGY_ERR err_to_rgy(AMF_RESULT err) {
    const RGYErrMapAMF *ERR_MAP_FIN = (const RGYErrMapAMF *)ERR_MAP_AMF + _countof(ERR_MAP_AMF);
    auto ret = std::find_if((const RGYErrMapAMF *)ERR_MAP_AMF, ERR_MAP_FIN, [err](const RGYErrMapAMF map) {
        return map.amf == err;
    });
    return (ret == ERR_MAP_FIN) ? RGY_ERR_UNKNOWN : ret->rgy;
}

#include "rgy_vulkan.h"

#if ENABLE_VULKAN

struct RGYErrMapVK {
    RGY_ERR rgy;
    VkResult vk;
};

static const RGYErrMapVK ERR_MAP_VK[] = {
    { RGY_ERR_NONE, VK_SUCCESS },
    { RGY_ERR_VK_NOT_READY, VK_NOT_READY },
    { RGY_ERR_VK_TIMEOUT, VK_TIMEOUT },
    { RGY_ERR_VK_EVENT_SET, VK_EVENT_SET },
    { RGY_ERR_VK_EVENT_RESET, VK_EVENT_RESET },
    { RGY_ERR_VK_INCOMPLETE, VK_INCOMPLETE },
    { RGY_ERR_VK_OUT_OF_HOST_MEMORY, VK_ERROR_OUT_OF_HOST_MEMORY },
    { RGY_ERR_VK_OUT_OF_DEVICE_MEMORY, VK_ERROR_OUT_OF_DEVICE_MEMORY },
    { RGY_ERR_VK_INITIALIZATION_FAILED, VK_ERROR_INITIALIZATION_FAILED },
    { RGY_ERR_VK_DEVICE_LOST, VK_ERROR_DEVICE_LOST },
    { RGY_ERR_VK_MEMORY_MAP_FAILED, VK_ERROR_MEMORY_MAP_FAILED },
    { RGY_ERR_VK_LAYER_NOT_PRESENT, VK_ERROR_LAYER_NOT_PRESENT },
    { RGY_ERR_VK_EXTENSION_NOT_PRESENT, VK_ERROR_EXTENSION_NOT_PRESENT },
    { RGY_ERR_VK_FEATURE_NOT_PRESENT, VK_ERROR_FEATURE_NOT_PRESENT },
    { RGY_ERR_VK_INCOMPATIBLE_DRIVER, VK_ERROR_INCOMPATIBLE_DRIVER },
    { RGY_ERR_VK_TOO_MANY_OBJECTS, VK_ERROR_TOO_MANY_OBJECTS },
    { RGY_ERR_VK_FORMAT_NOT_SUPPORTED, VK_ERROR_FORMAT_NOT_SUPPORTED },
    { RGY_ERR_VK_FRAGMENTED_POOL, VK_ERROR_FRAGMENTED_POOL },
    { RGY_ERR_VK_UNKNOWN, VK_ERROR_UNKNOWN },
    { RGY_ERR_VK_OUT_OF_POOL_MEMORY, VK_ERROR_OUT_OF_POOL_MEMORY },
    { RGY_ERR_VK_INVALID_EXTERNAL_HANDLE, VK_ERROR_INVALID_EXTERNAL_HANDLE },
    { RGY_ERR_VK_FRAGMENTATION, VK_ERROR_FRAGMENTATION },
    { RGY_ERR_VK_INVALID_OPAQUE_CAPTURE_ADDRESS, VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS },
    { RGY_ERR_VK_SURFACE_LOST_KHR, VK_ERROR_SURFACE_LOST_KHR },
    { RGY_ERR_VK_NATIVE_WINDOW_IN_USE_KHR, VK_ERROR_NATIVE_WINDOW_IN_USE_KHR },
    { RGY_ERR_VK__SUBOPTIMAL_KHR, VK_SUBOPTIMAL_KHR },
    { RGY_ERR_VK_OUT_OF_DATE_KHR, VK_ERROR_OUT_OF_DATE_KHR },
    { RGY_ERR_VK_INCOMPATIBLE_DISPLAY_KHR, VK_ERROR_INCOMPATIBLE_DISPLAY_KHR },
    { RGY_ERR_VK_VALIDATION_FAILED_EXT, VK_ERROR_VALIDATION_FAILED_EXT },
    { RGY_ERR_VK_INVALID_SHADER_NV, VK_ERROR_INVALID_SHADER_NV },
    { RGY_ERR_VK_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT },
    { RGY_ERR_VK_NOT_PERMITTED_EXT, VK_ERROR_NOT_PERMITTED_EXT },
    { RGY_ERR_VK_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT },
    //{ RGY_VK_THREAD_IDLE_KHR, VK_THREAD_IDLE_KHR },
    //{ RGY_VK_THREAD_DONE_KHR, VK_THREAD_DONE_KHR },
    //{ RGY_VK_OPERATION_DEFERRED_KHR, VK_OPERATION_DEFERRED_KHR },
    //{ RGY_VK_OPERATION_NOT_DEFERRED_KHR, VK_OPERATION_NOT_DEFERRED_KHR },
    //{ RGY_VK_PIPELINE_COMPILE_REQUIRED_EXT, VK_PIPELINE_COMPILE_REQUIRED_EXT },
    { RGY_ERR_VK_OUT_OF_POOL_MEMORY_KHR, VK_ERROR_OUT_OF_POOL_MEMORY_KHR },
    { RGY_ERR_VK_INVALID_EXTERNAL_HANDLE_KHR, VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR },
    { RGY_ERR_VK_FRAGMENTATION_EXT, VK_ERROR_FRAGMENTATION_EXT },
    { RGY_ERR_VK_INVALID_DEVICE_ADDRESS_EXT, VK_ERROR_INVALID_DEVICE_ADDRESS_EXT },
    { RGY_ERR_VK_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR, VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR },
    //{ RGY_ERR_VK_PIPELINE_COMPILE_REQUIRED_EXT, VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT },
};

VkResult err_to_vk(RGY_ERR err) {
    const RGYErrMapVK *ERR_MAP_VK_FIN = (const RGYErrMapVK *)ERR_MAP_VK + _countof(ERR_MAP_VK);
    auto ret = std::find_if((const RGYErrMapVK *)ERR_MAP_VK, ERR_MAP_VK_FIN, [err](const RGYErrMapVK map) {
        return map.rgy == err;
        });
    return (ret == ERR_MAP_VK_FIN) ? VK_ERROR_UNKNOWN : ret->vk;
}

RGY_ERR err_to_rgy(VkResult err) {
    const RGYErrMapVK *ERR_MAP_VK_FIN = (const RGYErrMapVK *)ERR_MAP_VK + _countof(ERR_MAP_VK);
    auto ret = std::find_if((const RGYErrMapVK *)ERR_MAP_VK, ERR_MAP_VK_FIN, [err](const RGYErrMapVK map) {
        return map.vk == err;
        });
    return (ret == ERR_MAP_VK_FIN) ? RGY_ERR_VK_UNKNOWN : ret->rgy;
}
#endif //#if ENABLE_VULKAN
#endif //#if ENCODER_VCEENC

#if ENCODER_MPP

struct RGYErrMapMPP {
    RGY_ERR rgy;
    MPP_RET mpp;
};

#define MPPERR_MAP(x) { RGY_ERR_MPP_ERR_ ##x, MPP_ERR_ ##x }
static const RGYErrMapMPP ERR_MAP_MPP[] = {
    { RGY_ERR_NONE, MPP_SUCCESS },
    { RGY_ERR_NONE, MPP_OK },
    { RGY_ERR_UNKNOWN, MPP_NOK },
    MPPERR_MAP(UNKNOW),
    MPPERR_MAP(NULL_PTR),
    MPPERR_MAP(MALLOC),
    MPPERR_MAP(OPEN_FILE),
    MPPERR_MAP(VALUE),
    MPPERR_MAP(READ_BIT),
    MPPERR_MAP(TIMEOUT),
    MPPERR_MAP(PERM),
    MPPERR_MAP(BASE),
    MPPERR_MAP(LIST_STREAM),
    MPPERR_MAP(INIT),
    MPPERR_MAP(VPU_CODEC_INIT),
    MPPERR_MAP(STREAM),
    MPPERR_MAP(FATAL_THREAD),
    MPPERR_MAP(NOMEM),
    MPPERR_MAP(PROTOL),
    { RGY_ERR_MPP_FAIL_SPLIT_FRAME, MPP_FAIL_SPLIT_FRAME },
    MPPERR_MAP(VPUHW),
    { RGY_ERR_MPP_EOS_STREAM_REACHED, MPP_EOS_STREAM_REACHED },
    MPPERR_MAP(BUFFER_FULL),
    MPPERR_MAP(DISPLAY_FULL)
};
#undef MPPERR_MAP

MPP_RET err_to_mpp(RGY_ERR err) {
    const RGYErrMapMPP *ERR_MAP_MPP_FIN = (const RGYErrMapMPP *)ERR_MAP_MPP + _countof(ERR_MAP_MPP);
    auto ret = std::find_if((const RGYErrMapMPP *)ERR_MAP_MPP, ERR_MAP_MPP_FIN, [err](const RGYErrMapMPP map) {
        return map.rgy == err;
        });
    return (ret == ERR_MAP_MPP_FIN) ? MPP_ERR_UNKNOW : ret->mpp;
}

RGY_ERR err_to_rgy(MPP_RET err) {
    const RGYErrMapMPP *ERR_MAP_MPP_FIN = (const RGYErrMapMPP *)ERR_MAP_MPP + _countof(ERR_MAP_MPP);
    auto ret = std::find_if((const RGYErrMapMPP *)ERR_MAP_MPP, ERR_MAP_MPP_FIN, [err](const RGYErrMapMPP map) {
        return map.mpp == err;
        });
    return (ret == ERR_MAP_MPP_FIN) ? RGY_ERR_UNKNOWN : ret->rgy;
}

struct RGYErrMapIM2D {
    RGY_ERR rgy;
    IM_STATUS im2d;
};
#define MPPERR_IM2D(x) { RGY_ERR_IM_STATUS_ ##x, IM_STATUS_ ##x }
static const RGYErrMapIM2D ERR_MAP_IM2D[] = {
    { RGY_ERR_NONE, IM_STATUS_NOERROR },
    { RGY_ERR_NONE, IM_STATUS_SUCCESS },
    MPPERR_IM2D(NOT_SUPPORTED),
    MPPERR_IM2D(OUT_OF_MEMORY),
    MPPERR_IM2D(INVALID_PARAM),
    MPPERR_IM2D(ILLEGAL_PARAM),
    MPPERR_IM2D(FAILED)
};
#undef MPPERR_IM2D

IM_STATUS err_to_im2d(RGY_ERR err) {
    const RGYErrMapIM2D *ERR_MAP_IM2D_FIN = (const RGYErrMapIM2D *)ERR_MAP_IM2D + _countof(ERR_MAP_IM2D);
    auto ret = std::find_if((const RGYErrMapIM2D *)ERR_MAP_IM2D, ERR_MAP_IM2D_FIN, [err](const RGYErrMapIM2D map) {
        return map.rgy == err;
        });
    return (ret == ERR_MAP_IM2D_FIN) ? IM_STATUS_FAILED : ret->im2d;
}

RGY_ERR err_to_rgy(IM_STATUS err) {
    const RGYErrMapIM2D *ERR_MAP_IM2D_FIN = (const RGYErrMapIM2D *)ERR_MAP_IM2D + _countof(ERR_MAP_IM2D);
    auto ret = std::find_if((const RGYErrMapIM2D *)ERR_MAP_IM2D, ERR_MAP_IM2D_FIN, [err](const RGYErrMapIM2D map) {
        return map.im2d == err;
        });
    return (ret == ERR_MAP_IM2D_FIN) ? RGY_ERR_UNKNOWN : ret->rgy;
}

#endif //#if ENCODER_MPP

const TCHAR *get_err_mes(RGY_ERR sts) {
    switch (sts) {
    case RGY_ERR_NONE:                            return _T("no error.");
    case RGY_ERR_UNKNOWN:                         return _T("unknown error.");
    case RGY_ERR_NULL_PTR:                        return _T("null pointer.");
    case RGY_ERR_UNSUPPORTED:                     return _T("undeveloped feature.");
    case RGY_ERR_MEMORY_ALLOC:                    return _T("failed to allocate memory.");
    case RGY_ERR_NOT_ENOUGH_BUFFER:               return _T("insufficient buffer at input/output.");
    case RGY_ERR_INVALID_HANDLE:                  return _T("invalid handle.");
    case RGY_ERR_LOCK_MEMORY:                     return _T("failed to lock the memory block.");
    case RGY_ERR_NOT_INITIALIZED:                 return _T("member function called before initialization.");
    case RGY_ERR_NOT_FOUND:                       return _T("the specified object is not found.");
    case RGY_ERR_MORE_DATA:                       return _T("expect more data at input.");
    case RGY_ERR_MORE_SURFACE:                    return _T("expect more surface at output.");
    case RGY_ERR_ABORTED:                         return _T("operation aborted.");
    case RGY_ERR_DEVICE_LOST:                     return _T("lose the HW acceleration device.");
    case RGY_ERR_INCOMPATIBLE_VIDEO_PARAM:        return _T("incompatible video parameters.");
    case RGY_ERR_INVALID_VIDEO_PARAM:             return _T("invalid video parameters.");
    case RGY_ERR_UNDEFINED_BEHAVIOR:              return _T("undefined behavior.");
    case RGY_ERR_DEVICE_FAILED:                   return _T("device operation failure.");
    case RGY_ERR_MORE_BITSTREAM:                  return _T("more bitstream required.");
    case RGY_ERR_INCOMPATIBLE_AUDIO_PARAM:        return _T("incompatible audio param.");
    case RGY_ERR_INVALID_AUDIO_PARAM:             return _T("invalid audio param.");
    case RGY_ERR_GPU_HANG:                        return _T("gpu hang.");
    case RGY_ERR_REALLOC_SURFACE:                 return _T("failed to realloc surface.");
    case RGY_ERR_ACCESS_DENIED:                   return _T("access denied");
    case RGY_ERR_INVALID_PARAM:                   return _T("invalid param.");
    case RGY_ERR_OUT_OF_RANGE:                    return _T("out of range.");
    case RGY_ERR_ALREADY_INITIALIZED:             return _T("already initialized.");
    case RGY_ERR_INVALID_FORMAT:                  return _T("invalid format.");
    case RGY_ERR_WRONG_STATE:                     return _T("wrong state.");
    case RGY_ERR_FILE_OPEN:                       return _T("file open error.");
    case RGY_ERR_INPUT_FULL:                      return _T("input full.");
    case RGY_ERR_INVALID_CODEC:                   return _T("invalid codec.");
    case RGY_ERR_INVALID_DATA_TYPE:               return _T("invalid data type.");
    case RGY_ERR_INVALID_RESOLUTION:              return _T("invalid resolution.");
    case RGY_ERR_INVALID_DEVICE:                  return _T("invalid devices.");
    case RGY_ERR_INVALID_CALL:                    return _T("invalid call sequence.");
    case RGY_ERR_NO_DEVICE:                       return _T("no deivce found.");
    case RGY_ERR_INVALID_VERSION:                 return _T("invalid version.");
    case RGY_ERR_MAP_FAILED:                      return _T("map failed.");
    case RGY_ERR_CUDA:                            return _T("error in cuda.");
    case RGY_ERR_RUN_PROCESS:                     return _T("running process failed.");
    case RGY_WRN_IN_EXECUTION:                    return _T("the previous asynchrous operation is in execution.");
    case RGY_WRN_DEVICE_BUSY:                     return _T("the HW acceleration device is busy.");
    case RGY_WRN_VIDEO_PARAM_CHANGED:             return _T("the video parameters are changed during decoding.");
    case RGY_WRN_PARTIAL_ACCELERATION:            return _T("partial acceleration.");
    case RGY_WRN_INCOMPATIBLE_VIDEO_PARAM:        return _T("incompatible video parameters.");
    case RGY_WRN_VALUE_NOT_CHANGED:               return _T("the value is saturated based on its valid range.");
    case RGY_WRN_OUT_OF_RANGE:                    return _T("the value is out of valid range.");
    case RGY_ERR_INVALID_PLATFORM:                return _T("invalid platform.");
    case RGY_ERR_INVALID_DEVICE_TYPE:             return _T("invalid device type.");
    case RGY_ERR_INVALID_CONTEXT:                 return _T("invalid context.");
    case RGY_ERR_INVALID_QUEUE_PROPERTIES:        return _T("invalid queue properties.");
    case RGY_ERR_INVALID_COMMAND_QUEUE:           return _T("invalid command queue.");
    case RGY_ERR_DEVICE_NOT_FOUND:                return _T("device not found.");
    case RGY_ERR_DEVICE_NOT_AVAILABLE:            return _T("device not available.");
    case RGY_ERR_COMPILER_NOT_AVAILABLE:          return _T("compiler not available.");
    case RGY_ERR_COMPILE_PROGRAM_FAILURE:         return _T("compile program failure.");
    case RGY_ERR_MEM_OBJECT_ALLOCATION_FAILURE:   return _T("pbject allocation failure.");
    case RGY_ERR_OUT_OF_RESOURCES:                return _T("out of resources.");
    case RGY_ERR_OUT_OF_HOST_MEMORY:              return _T("out of hots memory.");
    case RGY_ERR_PROFILING_INFO_NOT_AVAILABLE:    return _T("profiling info not available.");
    case RGY_ERR_MEM_COPY_OVERLAP:                return _T("memcpy overlap.");
    case RGY_ERR_IMAGE_FORMAT_MISMATCH:           return _T("image format mismatch.");
    case RGY_ERR_IMAGE_FORMAT_NOT_SUPPORTED:      return _T("image format not supported.");
    case RGY_ERR_BUILD_PROGRAM_FAILURE:           return _T("build program failure.");
    case RGY_ERR_MAP_FAILURE:                     return _T("map failure.");
    case RGY_ERR_INVALID_HOST_PTR:                return _T("invalid host ptr.");
    case RGY_ERR_INVALID_MEM_OBJECT:              return _T("invalid mem obejct.");
    case RGY_ERR_INVALID_IMAGE_FORMAT_DESCRIPTOR: return _T("invalid image format descripter.");
    case RGY_ERR_INVALID_IMAGE_SIZE:              return _T("invalid image size.");
    case RGY_ERR_INVALID_SAMPLER:                 return _T("invalid sampler.");
    case RGY_ERR_INVALID_BINARY:                  return _T("invalid binary.");
    case RGY_ERR_INVALID_BUILD_OPTIONS:           return _T("invalid build options.");
    case RGY_ERR_INVALID_PROGRAM:                 return _T("invalid program.");
    case RGY_ERR_INVALID_PROGRAM_EXECUTABLE:      return _T("invalid program executable.");
    case RGY_ERR_INVALID_KERNEL_NAME:             return _T("invalid kernel name.");
    case RGY_ERR_INVALID_KERNEL_DEFINITION:       return _T("invalid kernel definition.");
    case RGY_ERR_INVALID_KERNEL:                  return _T("invalid kernel.");
    case RGY_ERR_INVALID_ARG_INDEX:               return _T("invalid arg index.");
    case RGY_ERR_INVALID_ARG_VALUE:               return _T("invalid arg value.");
    case RGY_ERR_INVALID_ARG_SIZE:                return _T("invalid arg size.");
    case RGY_ERR_INVALID_KERNEL_ARGS:             return _T("invalid kernel args.");
    case RGY_ERR_INVALID_WORK_DIMENSION:          return _T("invalid work dimension.");
    case RGY_ERR_INVALID_WORK_GROUP_SIZE:         return _T("invalid work group size.");
    case RGY_ERR_INVALID_WORK_ITEM_SIZE:          return _T("invalid work item size.");
    case RGY_ERR_INVALID_GLOBAL_OFFSET:           return _T("invalid global offset.");
    case RGY_ERR_INVALID_EVENT_WAIT_LIST:         return _T("invalid event wait list.");
    case RGY_ERR_INVALID_EVENT:                   return _T("invalid event.");
    case RGY_ERR_INVALID_OPERATION:               return _T("invalid operation.");
    case RGY_ERR_INVALID_GL_OBJECT:               return _T("invalid gl object.");
    case RGY_ERR_INVALID_BUFFER_SIZE:             return _T("invalid buffer size.");
    case RGY_ERR_INVALID_MIP_LEVEL:               return _T("invalid mip level.");
    case RGY_ERR_INVALID_GLOBAL_WORK_SIZE:        return _T("invalid global work size.");
    case RGY_ERR_OPENCL_CRUSH:                    return _T("OpenCL crushed.");
    case RGY_ERR_VK_NOT_READY:                                      return _T("VK_NOT_READY");
    case RGY_ERR_VK_TIMEOUT:                                        return _T("VK_TIMEOUT");
    case RGY_ERR_VK_EVENT_SET:                                      return _T("VK_EVENT_SET");
    case RGY_ERR_VK_EVENT_RESET:                                    return _T("VK_EVENT_RESET");
    case RGY_ERR_VK_INCOMPLETE:                                     return _T("VK_INCOMPLETE");
    case RGY_ERR_VK_OUT_OF_HOST_MEMORY:                             return _T("VK_OUT_OF_HOST_MEMORY");
    case RGY_ERR_VK_OUT_OF_DEVICE_MEMORY:                           return _T("VK_OUT_OF_DEVICE_MEMORY");
    case RGY_ERR_VK_INITIALIZATION_FAILED:                          return _T("VK_INITIALIZATION_FAILED");
    case RGY_ERR_VK_DEVICE_LOST:                                    return _T("VK_DEVICE_LOST");
    case RGY_ERR_VK_MEMORY_MAP_FAILED:                              return _T("VK_MEMORY_MAP_FAILED");
    case RGY_ERR_VK_LAYER_NOT_PRESENT:                              return _T("VK_LAYER_NOT_PRESENT");
    case RGY_ERR_VK_EXTENSION_NOT_PRESENT:                          return _T("VK_EXTENSION_NOT_PRESENT");
    case RGY_ERR_VK_FEATURE_NOT_PRESENT:                            return _T("VK_FEATURE_NOT_PRESENT");
    case RGY_ERR_VK_INCOMPATIBLE_DRIVER:                            return _T("VK_INCOMPATIBLE_DRIVER");
    case RGY_ERR_VK_TOO_MANY_OBJECTS:                               return _T("VK_TOO_MANY_OBJECTS");
    case RGY_ERR_VK_FORMAT_NOT_SUPPORTED:                           return _T("VK_FORMAT_NOT_SUPPORTED");
    case RGY_ERR_VK_FRAGMENTED_POOL:                                return _T("VK_FRAGMENTED_POOL");
    case RGY_ERR_VK_UNKNOWN:                                        return _T("VK_UNKNOWN");
    case RGY_ERR_VK_OUT_OF_POOL_MEMORY:                             return _T("VK_OUT_OF_POOL_MEMORY");
    case RGY_ERR_VK_INVALID_EXTERNAL_HANDLE:                        return _T("VK_INVALID_EXTERNAL_HANDLE");
    case RGY_ERR_VK_FRAGMENTATION:                                  return _T("VK_FRAGMENTATION");
    case RGY_ERR_VK_INVALID_OPAQUE_CAPTURE_ADDRESS:                 return _T("VK_INVALID_OPAQUE_CAPTURE_ADDRESS");
    case RGY_ERR_VK_SURFACE_LOST_KHR:                               return _T("VK_SURFACE_LOST_KHR");
    case RGY_ERR_VK_NATIVE_WINDOW_IN_USE_KHR:                       return _T("VK_NATIVE_WINDOW_IN_USE_KHR");
    case RGY_ERR_VK__SUBOPTIMAL_KHR:                                return _T("VK_SUBOPTIMAL_KHR");
    case RGY_ERR_VK_OUT_OF_DATE_KHR:                                return _T("VK_OUT_OF_DATE_KHR");
    case RGY_ERR_VK_INCOMPATIBLE_DISPLAY_KHR:                       return _T("VK_INCOMPATIBLE_DISPLAY_KHR");
    case RGY_ERR_VK_VALIDATION_FAILED_EXT:                          return _T("VK_VALIDATION_FAILED_EXT");
    case RGY_ERR_VK_INVALID_SHADER_NV:                              return _T("VK_INVALID_SHADER_NV");
    case RGY_ERR_VK_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:   return _T("VK_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT");
    case RGY_ERR_VK_NOT_PERMITTED_EXT:                              return _T("VK_NOT_PERMITTED_EXT");
    case RGY_ERR_VK_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:            return _T("VK_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT");
    case RGY_VK_THREAD_IDLE_KHR:                                    return _T("VK_THREAD_IDLE_KHR");
    case RGY_VK_THREAD_DONE_KHR:                                    return _T("VK_THREAD_DONE_KHR");
    case RGY_VK_OPERATION_DEFERRED_KHR:                             return _T("VK_OPERATION_DEFERRED_KHR");
    case RGY_VK_OPERATION_NOT_DEFERRED_KHR:                         return _T("VK_OPERATION_NOT_DEFERRED_KHR");
    case RGY_VK_PIPELINE_COMPILE_REQUIRED_EXT:                      return _T("VK_PIPELINE_COMPILE_REQUIRED_EXT");
    case RGY_ERR_VK_OUT_OF_POOL_MEMORY_KHR:                         return _T("VK_OUT_OF_POOL_MEMORY_KHR");
    case RGY_ERR_VK_INVALID_EXTERNAL_HANDLE_KHR:                    return _T("VK_INVALID_EXTERNAL_HANDLE_KHR");
    case RGY_ERR_VK_FRAGMENTATION_EXT:                              return _T("VK_FRAGMENTATION_EXT");
    case RGY_ERR_VK_INVALID_DEVICE_ADDRESS_EXT:                     return _T("VK_INVALID_DEVICE_ADDRESS_EXT");
    case RGY_ERR_VK_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:             return _T("VK_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR");
    case RGY_ERR_VK_PIPELINE_COMPILE_REQUIRED_EXT:                  return _T("VK_PIPELINE_COMPILE_REQUIRED_EXT");

    case RGY_ERR_NVCV_GENERAL:               return _T("An otherwise unspecified error has occurred.");
    case RGY_ERR_NVCV_UNIMPLEMENTED:         return _T("The requested feature is not yet implemented.");
    case RGY_ERR_NVCV_MEMORY:                return _T("There is not enough memory for the requested operation.");
    case RGY_ERR_NVCV_EFFECT:                return _T("An invalid effect handle has been supplied.");
    case RGY_ERR_NVCV_SELECTOR:              return _T("The given parameter selector is not valid in this effect filter.");
    case RGY_ERR_NVCV_BUFFER:                return _T("An image buffer has not been specified.");
    case RGY_ERR_NVCV_PARAMETER:             return _T("An invalid parameter value has been supplied for this effect+selector.");
    case RGY_ERR_NVCV_MISMATCH:              return _T("Some parameters are not appropriately matched.");
    case RGY_ERR_NVCV_PIXELFORMAT:           return _T("The specified pixel format is not accommodated.");
    case RGY_ERR_NVCV_MODEL:                 return _T("Error while loading the TRT model.");
    case RGY_ERR_NVCV_LIBRARY:               return _T("Error loading the dynamic library.");
    case RGY_ERR_NVCV_INITIALIZATION:        return _T("The effect has not been properly initialized.");
    case RGY_ERR_NVCV_FILE:                  return _T("The file could not be found.");
    case RGY_ERR_NVCV_FEATURENOTFOUND:       return _T("The requested feature was not found");
    case RGY_ERR_NVCV_MISSINGINPUT:          return _T("A required parameter was not set");
    case RGY_ERR_NVCV_RESOLUTION:            return _T("The specified image resolution is not supported.");
    case RGY_ERR_NVCV_UNSUPPORTEDGPU:        return _T("The GPU is not supported");
    case RGY_ERR_NVCV_WRONGGPU:              return _T("The current GPU is not the one selected.");
    case RGY_ERR_NVCV_UNSUPPORTEDDRIVER:     return _T("The currently installed graphics driver is not supported");
    case RGY_ERR_NVCV_MODELDEPENDENCIES:     return _T("There is no model with dependencies that match this system");
    case RGY_ERR_NVCV_PARSE:                 return _T("There has been a parsing or syntax error while reading a file");
    case RGY_ERR_NVCV_MODELSUBSTITUTION:     return _T("The specified model does not exist and has been substituted.");
    case RGY_ERR_NVCV_READ:                  return _T("An error occurred while reading a file.");
    case RGY_ERR_NVCV_WRITE:                 return _T("An error occurred while writing a file.");
    case RGY_ERR_NVCV_PARAMREADONLY:         return _T("The selected parameter is read-only.");
    case RGY_ERR_NVCV_TRT_ENQUEUE:           return _T("TensorRT enqueue failed.");
    case RGY_ERR_NVCV_TRT_BINDINGS:          return _T("Unexpected TensorRT bindings.");
    case RGY_ERR_NVCV_TRT_CONTEXT:           return _T("An error occurred while creating a TensorRT context.");
    case RGY_ERR_NVCV_TRT_INFER:             return _T("The was a problem creating the inference engine.");
    case RGY_ERR_NVCV_TRT_ENGINE:            return _T("There was a problem deserializing the inference runtime engine.");
    case RGY_ERR_NVCV_NPP:                   return _T("An error has occurred in the NPP library.");
    case RGY_ERR_NVCV_CONFIG:                return _T("No suitable model exists for the specified parameter configuration.");
    case RGY_ERR_NVCV_TOOSMALL:              return _T("A supplied parameter or buffer is not large enough.");
    case RGY_ERR_NVCV_TOOBIG:                return _T("A supplied parameter is too big.");
    case RGY_ERR_NVCV_WRONGSIZE:             return _T("A supplied parameter is not the expected size.");
    case RGY_ERR_NVCV_OBJECTNOTFOUND:        return _T("The specified object was not found.");
    case RGY_ERR_NVCV_SINGULAR:              return _T("A mathematical singularity has been encountered.");
    case RGY_ERR_NVCV_NOTHINGRENDERED:       return _T("Nothing was rendered in the specified region.");
    case RGY_ERR_NVCV_CONVERGENCE:           return _T("An iteration did not converge satisfactorily.");
    case RGY_ERR_NVCV_OPENGL:                return _T("An OpenGL error has occurred.");
    case RGY_ERR_NVCV_DIRECT3D:              return _T("A Direct3D error has occurred.");
    case RGY_ERR_NVCV_CUDA_BASE:             return _T("CUDA errors are offset from this value.");
    case RGY_ERR_NVCV_CUDA_VALUE:            return _T("A CUDA parameter is not within the acceptable range.");
    case RGY_ERR_NVCV_CUDA_MEMORY:           return _T("There is not enough CUDA memory for the requested operation.");
    case RGY_ERR_NVCV_CUDA_PITCH:            return _T("A CUDA pitch is not within the acceptable range.");
    case RGY_ERR_NVCV_CUDA_INIT:             return _T("The CUDA driver and runtime could not be initialized.");
    case RGY_ERR_NVCV_CUDA_LAUNCH:           return _T("The CUDA kernel launch has failed.");
    case RGY_ERR_NVCV_CUDA_KERNEL:           return _T("No suitable kernel image is available for the device.");
    case RGY_ERR_NVCV_CUDA_DRIVER:           return _T("The installed NVIDIA CUDA driver is older than the CUDA runtime library.");
    case RGY_ERR_NVCV_CUDA_UNSUPPORTED:      return _T("The CUDA operation is not supported on the current system or device.");
    case RGY_ERR_NVCV_CUDA_ILLEGAL_ADDRESS:  return _T("CUDA tried to load or store on an invalid memory address.");
    case RGY_ERR_NVCV_CUDA:                  return _T("An otherwise unspecified CUDA error has been reported.");

#define CASE_ERR_MPP(x) case RGY_ERR_ ## x: return _T(#x);
    CASE_ERR_MPP(MPP_ERR_UNKNOW);
    CASE_ERR_MPP(MPP_ERR_NULL_PTR);
    CASE_ERR_MPP(MPP_ERR_MALLOC);
    CASE_ERR_MPP(MPP_ERR_OPEN_FILE);
    CASE_ERR_MPP(MPP_ERR_VALUE);
    CASE_ERR_MPP(MPP_ERR_READ_BIT);
    CASE_ERR_MPP(MPP_ERR_TIMEOUT);
    CASE_ERR_MPP(MPP_ERR_PERM);
    CASE_ERR_MPP(MPP_ERR_BASE);
    CASE_ERR_MPP(MPP_ERR_LIST_STREAM);
    CASE_ERR_MPP(MPP_ERR_INIT);
    CASE_ERR_MPP(MPP_ERR_VPU_CODEC_INIT);
    CASE_ERR_MPP(MPP_ERR_STREAM);
    CASE_ERR_MPP(MPP_ERR_FATAL_THREAD);
    CASE_ERR_MPP(MPP_ERR_NOMEM);
    CASE_ERR_MPP(MPP_ERR_PROTOL);
    CASE_ERR_MPP(MPP_FAIL_SPLIT_FRAME);
    CASE_ERR_MPP(MPP_ERR_VPUHW);
    CASE_ERR_MPP(MPP_EOS_STREAM_REACHED);
    CASE_ERR_MPP(MPP_ERR_BUFFER_FULL);
    CASE_ERR_MPP(MPP_ERR_DISPLAY_FULL);
#undef CASE_ERR_MPP

    default:                                      return _T("unknown error.");
    }
}

