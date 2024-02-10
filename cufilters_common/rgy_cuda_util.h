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
// --------------------------------------------------------------------------------------------

#pragma once
#ifndef __RGY_CUDA_UTIL_H__
#define __RGY_CUDA_UTIL_H__

#pragma warning (push)
#pragma warning (disable: 4819)
#include <cuda_runtime.h>
#include <npp.h>
#include <cuda.h>
#pragma warning (pop)
#include "rgy_tchar.h"
#include "rgy_util.h"
#include "rgy_err.h"
#include "convert_csp.h"

struct cudaevent_deleter {
    void operator()(cudaEvent_t *pEvent) const {
        cudaEventDestroy(*pEvent);
        delete pEvent;
    }
};

struct cudastream_deleter {
    void operator()(cudaStream_t *pStream) const {
        cudaStreamDestroy(*pStream);
        delete pStream;
    }
};

struct cudahost_deleter {
    void operator()(void *ptr) const {
        cudaFreeHost(ptr);
    }
};

struct cudadevice_deleter {
    void operator()(void *ptr) const {
        cudaFree(ptr);
    }
};

static inline int divCeil(int value, int radix) {
    return (value + radix - 1) / radix;
}

static inline cudaMemcpyKind getCudaMemcpyKind(bool inputDevice, bool outputDevice) {
    if (inputDevice) {
        return (outputDevice) ? cudaMemcpyDeviceToDevice : cudaMemcpyDeviceToHost;
    } else {
        return (outputDevice) ? cudaMemcpyHostToDevice : cudaMemcpyHostToHost;
    }
}

static const TCHAR *getCudaMemcpyKindStr(cudaMemcpyKind kind) {
    switch (kind) {
    case cudaMemcpyDeviceToDevice:
        return _T("copyDtoD");
    case cudaMemcpyDeviceToHost:
        return _T("copyDtoH");
    case cudaMemcpyHostToDevice:
        return _T("copyHtoD");
    case cudaMemcpyHostToHost:
        return _T("copyHtoH");
    default:
        return _T("copyUnknown");
    }
}

static const TCHAR *getCudaMemcpyKindStr(bool inputDevice, bool outputDevice) {
    return getCudaMemcpyKindStr(getCudaMemcpyKind(inputDevice, outputDevice));
}

static RGY_ERR copyPlane(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemcpy2D(dst->ptr, dst->pitch, src->ptr, src->pitch, width_byte, dst->height, getCudaMemcpyKind(src->deivce_mem, dst->deivce_mem)));
}

static RGY_ERR copyPlaneAsync(RGYFrameInfo *dst, const RGYFrameInfo *src, cudaStream_t stream) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemcpy2DAsync(dst->ptr, dst->pitch, src->ptr, src->pitch, width_byte, dst->height, getCudaMemcpyKind(src->deivce_mem, dst->deivce_mem), stream));
}

static RGY_ERR copyPlaneField(RGYFrameInfo *dst, const RGYFrameInfo *src, const bool dstTopField, const bool srcTopField) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemcpy2D(
        dst->ptr + ((dstTopField) ? 0 : dst->pitch),
        dst->pitch << 1,
        src->ptr + ((srcTopField) ? 0 : src->pitch),
        src->pitch << 1,
        width_byte,
        dst->height >> 1,
        getCudaMemcpyKind(src->deivce_mem, dst->deivce_mem)));
}

static RGY_ERR copyPlaneFieldAsync(RGYFrameInfo *dst, const RGYFrameInfo *src, const bool dstTopField, const bool srcTopField, cudaStream_t stream) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemcpy2DAsync(
        dst->ptr + ((dstTopField) ? 0 : dst->pitch),
        dst->pitch << 1,
        src->ptr + ((srcTopField) ? 0 : src->pitch),
        src->pitch << 1,
        width_byte,
        dst->height >> 1,
        getCudaMemcpyKind(src->deivce_mem, dst->deivce_mem), stream));
}

static RGY_ERR setPlane(RGYFrameInfo *dst, int value) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemset2D(
        dst->ptr,
        dst->pitch,
        value,
        width_byte,
        dst->height));
}

static RGY_ERR setPlaneAsync(RGYFrameInfo *dst, int value, cudaStream_t stream) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemset2DAsync(
        dst->ptr,
        dst->pitch,
        value,
        width_byte,
        dst->height, stream));
}

static RGY_ERR setPlaneField(RGYFrameInfo *dst, int value, bool topField) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemset2D(
        dst->ptr + ((topField) ? 0 : dst->pitch),
        dst->pitch << 1,
        value,
        width_byte,
        dst->height >> 1));
}

static RGY_ERR setPlaneFieldAsync(RGYFrameInfo *dst, int value, bool topField, cudaStream_t stream) {
    const int width_byte = dst->width * (RGY_CSP_BIT_DEPTH[dst->csp] > 8 ? 2 : 1);
    return err_to_rgy(cudaMemset2DAsync(
        dst->ptr + ((topField) ? 0 : dst->pitch),
        dst->pitch << 1,
        value,
        width_byte,
        dst->height >> 1, stream));
}

static RGY_ERR checkCopyFrame(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    auto dstInfoEx = getFrameInfoExtra(dst);
    const auto srcInfoEx = getFrameInfoExtra(src);
    if (dst->pitch == 0
        || srcInfoEx.width_byte > dst->pitch
        || srcInfoEx.height_total > dstInfoEx.height_total) {
        if (dst->ptr) {
            cudaFree(dst->ptr);
            dst->ptr = nullptr;
        }
        dst->pitch = 0;
    }
    if (dst->ptr == nullptr) {
        dstInfoEx = getFrameInfoExtra(dst);
        if (!dstInfoEx.width_byte) {
            return RGY_ERR_UNSUPPORTED;
        }
        if (dst->deivce_mem) {
            size_t memPitch = 0;
            auto ret = cudaMallocPitch(&dst->ptr, &memPitch, dstInfoEx.width_byte, dstInfoEx.height_total);
            if (ret != cudaSuccess) {
                return err_to_rgy(ret);
            }
            dst->pitch = (int)memPitch;
        } else {
            dst->pitch = ALIGN(dstInfoEx.width_byte, 64);
            dstInfoEx = getFrameInfoExtra(dst);
            auto ret = cudaMallocHost(&dst->ptr, dstInfoEx.frame_size);
            if (ret != cudaSuccess) {
                return err_to_rgy(ret);
            }
        }
    }
    return RGY_ERR_NONE;
}

static RGY_ERR copyFrame(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    for (int i = 0; i < RGY_CSP_PLANES[dst->csp]; i++) {
        const auto srcPlane = getPlane(src, (RGY_PLANE)i);
        auto dstPlane = getPlane(dst, (RGY_PLANE)i);
        auto ret = copyPlane(&dstPlane, &srcPlane);
        if (ret != RGY_ERR_NONE) {
            return ret;
        }
    }
    return RGY_ERR_NONE;
}

static RGY_ERR copyFrameAsync(RGYFrameInfo *dst, const RGYFrameInfo *src, cudaStream_t stream) {
    for (int i = 0; i < RGY_CSP_PLANES[dst->csp]; i++) {
        const auto srcPlane = getPlane(src, (RGY_PLANE)i);
        auto dstPlane = getPlane(dst, (RGY_PLANE)i);
        auto ret = copyPlaneAsync(&dstPlane, &srcPlane, stream);
        if (ret != RGY_ERR_NONE) {
            return ret;
        }
    }
    return RGY_ERR_NONE;
}

static RGY_ERR copyFrameField(RGYFrameInfo *dst, const RGYFrameInfo *src, const bool dstTopField, const bool srcTopField) {
    for (int i = 0; i < RGY_CSP_PLANES[dst->csp]; i++) {
        const auto srcPlane = getPlane(src, (RGY_PLANE)i);
        auto dstPlane = getPlane(dst, (RGY_PLANE)i);
        auto ret = copyPlaneField(&dstPlane, &srcPlane, dstTopField, srcTopField);
        if (ret != RGY_ERR_NONE) {
            return ret;
        }
    }
    return RGY_ERR_NONE;
}

static RGY_ERR copyFrameFieldAsync(RGYFrameInfo *dst, const RGYFrameInfo *src, const bool dstTopField, const bool srcTopField, cudaStream_t stream) {
    for (int i = 0; i < RGY_CSP_PLANES[dst->csp]; i++) {
        const auto srcPlane = getPlane(src, (RGY_PLANE)i);
        auto dstPlane = getPlane(dst, (RGY_PLANE)i);
        auto ret = copyPlaneFieldAsync(&dstPlane, &srcPlane, dstTopField, srcTopField, stream);
        if (ret != RGY_ERR_NONE) {
            return ret;
        }
    }
    return RGY_ERR_NONE;
}

static RGY_ERR copyFrameData(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    {   auto ret = checkCopyFrame(dst, src);
        if (ret != RGY_ERR_NONE) {
            return ret;
        }
    }
    auto ret = copyFrame(dst, src);
    if (ret == RGY_ERR_NONE) {
        copyFrameProp(dst, src);
    }
    return RGY_ERR_NONE;
}

static RGY_ERR copyFrameDataAsync(RGYFrameInfo *dst, const RGYFrameInfo *src, cudaStream_t stream) {
    {   auto ret = checkCopyFrame(dst, src);
        if (ret != RGY_ERR_NONE) {
            return ret;
        }
    }
    auto ret = copyFrameAsync(dst, src, stream);
    if (ret == RGY_ERR_NONE) {
        copyFrameProp(dst, src);
    }
    return RGY_ERR_NONE;
}

struct CUFrameBuf {
public:
    RGYFrameInfo frame;
    cudaEvent_t event;
    CUFrameBuf()
        : frame(), event() {
        cudaEventCreate(&event);
    };
    CUFrameBuf(uint8_t *ptr, int pitch, int width, int height, RGY_CSP csp = RGY_CSP_NV12)
        : frame(), event() {
        frame.ptr = ptr;
        frame.pitch = pitch;
        frame.width = width;
        frame.height = height;
        frame.csp = csp;
        frame.deivce_mem = true;
        cudaEventCreate(&event);
    };
    CUFrameBuf(int width, int height, RGY_CSP csp = RGY_CSP_NV12)
        : frame(), event() {
        frame.ptr = nullptr;
        frame.pitch = 0;
        frame.width = width;
        frame.height = height;
        frame.csp = csp;
        frame.deivce_mem = true;
        cudaEventCreate(&event);
    };
    CUFrameBuf(const RGYFrameInfo& _info)
        : frame(_info), event() {
        cudaEventCreate(&event);
    };
    RGY_ERR copyFrame(const RGYFrameInfo *src) {
        return copyFrameData(&frame, src);
    }
    RGY_ERR copyFrameAsync(const RGYFrameInfo *src, cudaStream_t stream) {
        return copyFrameDataAsync(&frame, src, stream);
    }
protected:
    CUFrameBuf(const CUFrameBuf &) = delete;
    void operator =(const CUFrameBuf &) = delete;
public:
    RGY_ERR alloc() {
        if (frame.ptr) {
            cudaFree(frame.ptr);
        }
        size_t memPitch = 0;
        auto ret = RGY_ERR_NONE;
        const auto infoEx = getFrameInfoExtra(&frame);
        if (infoEx.width_byte) {
            ret = err_to_rgy(cudaMallocPitch(&frame.ptr, &memPitch, infoEx.width_byte, infoEx.height_total));
        } else {
            ret = RGY_ERR_UNSUPPORTED;
        }
        frame.pitch = (int)memPitch;
        return ret;
    }
    RGY_ERR alloc(int width, int height, RGY_CSP csp = RGY_CSP_NV12) {
        if (frame.ptr) {
            cudaFree(frame.ptr);
        }
        frame.ptr = nullptr;
        frame.pitch = 0;
        frame.width = width;
        frame.height = height;
        frame.csp = csp;
        frame.deivce_mem = true;
        return alloc();
    }
    RGY_ERR allocHost() {
        if (frame.ptr) {
            cudaFree(frame.ptr);
        }
        auto ret = RGY_ERR_NONE;
        const auto infoEx = getFrameInfoExtra(&frame);
        frame.pitch = ALIGN(infoEx.width_byte, 64);
        if (frame.pitch) {
            ret = err_to_rgy(cudaMallocHost(&frame.ptr, frame.pitch * infoEx.height_total));
        } else {
            ret = RGY_ERR_UNSUPPORTED;
        }
        frame.deivce_mem = false;
        return ret;
    }
    RGY_ERR allocHost(int width, int height, RGY_CSP csp = RGY_CSP_NV12) {
        if (frame.ptr) {
            cudaFree(frame.ptr);
        }
        const auto infoEx = getFrameInfoExtra(&frame);
        frame.ptr = nullptr;
        frame.pitch = 0;
        frame.width = width;
        frame.height = height;
        frame.csp = csp;
        frame.deivce_mem = false;
        return allocHost();
    }
    void clear() {
        if (frame.ptr) {
            cudaFree(frame.ptr);
            frame.ptr = nullptr;
        }
    }
    ~CUFrameBuf() {
        clear();
        if (event) {
            cudaEventDestroy(event);
            event = nullptr;
        }
    }
};

struct CUFrameBufPair {
public:
    RGYFrameInfo frameDev;
    RGYFrameInfo frameHost;
    cudaEvent_t event;
    CUFrameBufPair()
        : frameDev(), frameHost(), event() {
        cudaEventCreate(&event);
    };
    CUFrameBufPair(int width, int height, RGY_CSP csp = RGY_CSP_NV12)
        : frameDev(), frameHost(), event() {
        frameDev.ptr = nullptr;
        frameDev.pitch = 0;
        frameDev.width = width;
        frameDev.height = height;
        frameDev.csp = csp;
        frameDev.deivce_mem = true;

        frameHost = frameDev;
        frameHost.deivce_mem = false;

        cudaEventCreate(&event);
    };
protected:
    CUFrameBufPair(const CUFrameBufPair &) = delete;
    void operator =(const CUFrameBufPair &) = delete;
public:
    RGY_ERR allocHost() {
        if (frameHost.ptr) {
            cudaFree(frameHost.ptr);
        }
        auto ret = RGY_ERR_NONE;
        const auto infoEx = getFrameInfoExtra(&frameHost);
        frameHost.pitch = ALIGN(infoEx.width_byte, 256);
        if (infoEx.width_byte) {
            ret = err_to_rgy(cudaMallocHost(&frameHost.ptr, frameHost.pitch * infoEx.height_total));
        } else {
            ret = RGY_ERR_UNSUPPORTED;
        }
        return ret;
    }
    RGY_ERR allocDev() {
        if (frameDev.ptr) {
            cudaFree(frameDev.ptr);
        }
        size_t memPitch = 0;
        auto ret = RGY_ERR_NONE;
        const auto infoEx = getFrameInfoExtra(&frameDev);
        if (infoEx.width_byte) {
            ret = err_to_rgy(cudaMallocPitch(&frameDev.ptr, &memPitch, infoEx.width_byte, infoEx.height_total));
        } else {
            ret = RGY_ERR_UNSUPPORTED;
        }
        frameDev.pitch = (int)memPitch;
        return ret;
    }
    RGY_ERR alloc() {
        clearHost();
        clearDev();
        auto err = allocDev();
        if (err != RGY_ERR_NONE) return err;
        return allocHost();
    }
    RGY_ERR alloc(int width, int height, RGY_CSP csp = RGY_CSP_NV12) {
        clearHost();
        clearDev();

        frameDev.ptr = nullptr;
        frameDev.pitch = 0;
        frameDev.width = width;
        frameDev.height = height;
        frameDev.csp = csp;
        frameDev.deivce_mem = true;

        frameHost = frameDev;
        frameHost.deivce_mem = false;

        return alloc();
    }
    RGY_ERR copyDtoHAsync(cudaStream_t stream = 0) {
        const auto infoEx = getFrameInfoExtra(&frameDev);
        return err_to_rgy(cudaMemcpy2DAsync(frameHost.ptr, frameHost.pitch, frameDev.ptr, frameDev.pitch, infoEx.width_byte, infoEx.height_total, cudaMemcpyDeviceToHost, stream));
    }
    RGY_ERR copyDtoH() {
        const auto infoEx = getFrameInfoExtra(&frameDev);
        return err_to_rgy(cudaMemcpy2D(frameHost.ptr, frameHost.pitch, frameDev.ptr, frameDev.pitch, infoEx.width_byte, infoEx.height_total, cudaMemcpyDeviceToHost));
    }
    RGY_ERR copyHtoDAsync(cudaStream_t stream = 0) {
        const auto infoEx = getFrameInfoExtra(&frameDev);
        return err_to_rgy(cudaMemcpy2DAsync(frameDev.ptr, frameDev.pitch, frameHost.ptr, frameHost.pitch, infoEx.width_byte, infoEx.height_total, cudaMemcpyHostToDevice, stream));
    }
    RGY_ERR copyHtoD() {
        const auto infoEx = getFrameInfoExtra(&frameDev);
        return err_to_rgy(cudaMemcpy2D(frameDev.ptr, frameDev.pitch, frameHost.ptr, frameHost.pitch, infoEx.width_byte, infoEx.height_total, cudaMemcpyHostToDevice));
    }

    void clearHost() {
        if (frameDev.ptr) {
            cudaFree(frameDev.ptr);
            frameDev.ptr = nullptr;
        }
    }
    void clearDev() {
        if (frameDev.ptr) {
            cudaFree(frameDev.ptr);
            frameDev.ptr = nullptr;
        }
    }
    void clear() {
        clearDev();
        clearHost();
    }
    ~CUFrameBufPair() {
        clearDev();
        clearHost();
        if (event) {
            cudaEventDestroy(event);
            event = nullptr;
        }
    }
};

struct CUMemBuf {
    void *ptr;
    size_t nSize;

    CUMemBuf() : ptr(nullptr), nSize(0) {

    };
    CUMemBuf(void *_ptr, size_t _nSize) : ptr(_ptr), nSize(_nSize) {

    };
    CUMemBuf(size_t _nSize) : ptr(nullptr), nSize(_nSize) {

    }
    RGY_ERR alloc() {
        if (ptr) {
            cudaFree(ptr);
        }
        auto ret = RGY_ERR_NONE;
        if (nSize > 0) {
            ret = err_to_rgy(cudaMalloc(&ptr, nSize));
        } else {
            ret = RGY_ERR_UNSUPPORTED;
        }
        return ret;
    }
    void clear() {
        if (ptr) {
            cudaFree(ptr);
            ptr = nullptr;
        }
        nSize = 0;
    }
    ~CUMemBuf() {
        clear();
    }
};

struct CUMemBufPair {
    void *ptrDevice;
    void *ptrHost;
    size_t nSize;

    CUMemBufPair() : ptrDevice(nullptr), ptrHost(nullptr), nSize(0) {

    };
    CUMemBufPair(size_t _nSize) : ptrDevice(nullptr), ptrHost(nullptr), nSize(_nSize) {

    }
    RGY_ERR alloc() {
        if (ptrDevice) {
            cudaFree(ptrDevice);
        }
        auto ret = RGY_ERR_NONE;
        if (nSize > 0) {
            ret = err_to_rgy(cudaMalloc(&ptrDevice, nSize));
            if (ret == RGY_ERR_NONE) {
                ret = err_to_rgy(cudaMallocHost(&ptrHost, nSize));
            }
        } else {
            ret = RGY_ERR_UNSUPPORTED;
        }
        return ret;
    }
    RGY_ERR alloc(size_t _nSize) {
        nSize = _nSize;
        return alloc();
    }
    RGY_ERR copyDtoHAsync(cudaStream_t stream = 0) {
        return err_to_rgy(cudaMemcpyAsync(ptrHost, ptrDevice, nSize, cudaMemcpyDeviceToHost, stream));
    }
    RGY_ERR copyDtoH() {
        return err_to_rgy(cudaMemcpy(ptrHost, ptrDevice, nSize, cudaMemcpyDeviceToHost));
    }
    RGY_ERR copyHtoDAsync(cudaStream_t stream = 0) {
        return err_to_rgy(cudaMemcpyAsync(ptrDevice, ptrHost, nSize, cudaMemcpyHostToDevice, stream));
    }
    RGY_ERR copyHtoD() {
        return err_to_rgy(cudaMemcpy(ptrDevice, ptrHost, nSize, cudaMemcpyHostToDevice));
    }
    void clear() {
        if (ptrDevice) {
            cudaFree(ptrDevice);
            ptrDevice = nullptr;
        }
        if (ptrHost) {
            cudaFreeHost(ptrHost);
            ptrDevice = nullptr;
        }
        nSize = 0;
    }
    ~CUMemBufPair() {
        clear();
    }
};

#endif //__RGY_CUDA_UTIL_H__
