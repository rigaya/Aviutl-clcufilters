// -----------------------------------------------------------------------------------------
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

#ifndef _RGY_FRAME_INFO_H_
#define _RGY_FRAME_INFO_H_

#include "convert_csp.h"

class RGYFrameData;

struct RGYFrameInfo {
    uint8_t *ptr[RGY_MAX_PLANES];
    RGY_CSP csp;
    int width, height, pitch[RGY_MAX_PLANES];
    int bitdepth;
    int64_t timestamp;
    int64_t duration;
    RGY_MEM_TYPE mem_type;
    RGY_PICSTRUCT picstruct;
    RGY_FRAME_FLAGS flags;
    int inputFrameId;
    std::vector<std::shared_ptr<RGYFrameData>> dataList;

    RGYFrameInfo() :
        ptr(),
        csp(RGY_CSP_NA),
        width(0),
        height(0),
        pitch(),
        bitdepth(0),
        timestamp(0),
        duration(0),
        mem_type(RGY_MEM_TYPE_CPU),
        picstruct(RGY_PICSTRUCT_UNKNOWN),
        flags(RGY_FRAME_FLAG_NONE),
        inputFrameId(-1),
        dataList() {
        memset(ptr, 0, sizeof(ptr));
        memset(pitch, 0, sizeof(pitch));
    };

    RGYFrameInfo(const int width_, const int height_, const RGY_CSP csp_, const int bitdepth_,
        const RGY_PICSTRUCT picstruct_ = RGY_PICSTRUCT_UNKNOWN, const RGY_MEM_TYPE memtype_ = RGY_MEM_TYPE_CPU) :
        ptr(),
        csp(csp_),
        width(width_),
        height(height_),
        pitch(),
        bitdepth(bitdepth_),
        timestamp(0),
        duration(0),
        mem_type(memtype_),
        picstruct(picstruct_),
        flags(RGY_FRAME_FLAG_NONE),
        inputFrameId(-1),
        dataList() {
        memset(ptr, 0, sizeof(ptr));
        memset(pitch, 0, sizeof(pitch));
    };

    std::basic_string<TCHAR> print() const {
        TCHAR buf[1024];
#if ENCODER_NVENC
        _stprintf_s(buf, _T("%dx%d %s %dbit (%d) %s %s f0x%x"),
            width, height, RGY_CSP_NAMES[csp], bitdepth, pitch,
            picstrcut_to_str(picstruct), deivce_mem ? _T("deivce") : _T("host"), (uint32_t)flags);
#else
        _stprintf_s(buf, _T("%dx%d %s %dbit (%d, %d, %d, %d) %s %s f0x%x"),
            width, height, RGY_CSP_NAMES[csp], bitdepth, pitch[0], pitch[1], pitch[2], pitch[3],
            picstrcut_to_str(picstruct), get_memtype_str(mem_type), (uint32_t)flags);
#endif
        return std::basic_string<TCHAR>(buf);
    };
};

static bool interlaced(const RGYFrameInfo& frameInfo) {
    return (frameInfo.picstruct & RGY_PICSTRUCT_INTERLACED) != 0;
}

static void copyFrameProp(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    dst->width = src->width;
    dst->height = src->height;
    dst->csp = src->csp;
    dst->picstruct = src->picstruct;
    dst->timestamp = src->timestamp;
    dst->duration = src->duration;
    dst->flags = src->flags;
    dst->inputFrameId = src->inputFrameId;
}

static RGYFrameInfo getPlane(const RGYFrameInfo *frameInfo, RGY_PLANE plane) {
    RGYFrameInfo planeInfo = *frameInfo;
    if (frameInfo->csp == RGY_CSP_YUY2
        || frameInfo->csp == RGY_CSP_Y210 || frameInfo->csp == RGY_CSP_Y216
        || frameInfo->csp == RGY_CSP_Y410 || frameInfo->csp == RGY_CSP_Y416
        || RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_RGB_PACKED
        || RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_MONOCHROME
        || RGY_CSP_PLANES[frameInfo->csp] == 1) {
        return planeInfo; //何もしない
    }
    if (frameInfo->csp == RGY_CSP_GBR || frameInfo->csp == RGY_CSP_GBRA) {
        switch (plane) {
        case RGY_PLANE_R: plane = RGY_PLANE_G; break;
        case RGY_PLANE_G: plane = RGY_PLANE_R; break;
        default:
            break;
        }
    }
    if (   frameInfo->csp == RGY_CSP_BGR_16 || frameInfo->csp == RGY_CSP_BGRA_16
        || frameInfo->csp == RGY_CSP_BGR_F32 || frameInfo->csp == RGY_CSP_BGRA_F32) {
        switch (plane) {
        case RGY_PLANE_R: plane = RGY_PLANE_B; break;
        case RGY_PLANE_B: plane = RGY_PLANE_R; break;
        default:
            break;
        }
    }
    if (plane == RGY_PLANE_Y) {
        for (int i = 1; i < RGY_MAX_PLANES; i++) {
            planeInfo.ptr[i] = nullptr;
            planeInfo.pitch[i] = 0;
        }
        return planeInfo;
    }
    auto const ptr = planeInfo.ptr[plane];
    const auto pitch = planeInfo.pitch[plane];
    for (int i = 0; i < RGY_MAX_PLANES; i++) {
        planeInfo.ptr[i] = nullptr;
        planeInfo.pitch[i] = 0;
    }
    planeInfo.ptr[0] = ptr;
    planeInfo.pitch[0] = pitch;

    if (frameInfo->csp == RGY_CSP_NV12 || frameInfo->csp == RGY_CSP_P010) {
        planeInfo.height >>= 1;
    } else if (frameInfo->csp == RGY_CSP_NV16 || frameInfo->csp == RGY_CSP_P210) {
        ;
    } else if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_YUV420) {
        planeInfo.width >>= 1;
        planeInfo.height >>= 1;
    } else if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_YUV422) {
        planeInfo.width >>= 1;
    }
    return planeInfo;
}

static sInputCrop getPlane(const sInputCrop *crop, const RGY_CSP csp, const RGY_PLANE plane) {
    sInputCrop planeCrop = *crop;
    if (plane == RGY_PLANE_Y
        || csp == RGY_CSP_YUY2
        || csp == RGY_CSP_Y210 || csp == RGY_CSP_Y216
        || csp == RGY_CSP_Y410 || csp == RGY_CSP_Y416
        || RGY_CSP_CHROMA_FORMAT[csp] == RGY_CHROMAFMT_RGB
        || RGY_CSP_CHROMA_FORMAT[csp] == RGY_CHROMAFMT_RGB_PACKED
        || RGY_CSP_CHROMA_FORMAT[csp] == RGY_CHROMAFMT_YUV444
        || RGY_CSP_CHROMA_FORMAT[csp] == RGY_CHROMAFMT_MONOCHROME
        || RGY_CSP_PLANES[csp] == 1) {
        return planeCrop;
    }
    if (csp == RGY_CSP_NV12 || csp == RGY_CSP_P010) {
        planeCrop.e.up >>= 1;
        planeCrop.e.bottom >>= 1;
    } else if (csp == RGY_CSP_NV16 || csp == RGY_CSP_P210) {
        ;
    } else if (RGY_CSP_CHROMA_FORMAT[csp] == RGY_CHROMAFMT_YUV420) {
        planeCrop.e.up >>= 1;
        planeCrop.e.bottom >>= 1;
        planeCrop.e.left >>= 1;
        planeCrop.e.right >>= 1;
    } else if (RGY_CSP_CHROMA_FORMAT[csp] == RGY_CHROMAFMT_YUV422) {
        planeCrop.e.left >>= 1;
        planeCrop.e.right >>= 1;
    }
    return planeCrop;
}

static bool cmpFrameInfoCspResolution(const RGYFrameInfo *pA, const RGYFrameInfo *pB) {
    return pA->csp != pB->csp
        || pA->width != pB->width
        || pA->height != pB->height
        || pA->mem_type != pB->mem_type;
}

#endif //_RGY_FRAME_INFO_H_
