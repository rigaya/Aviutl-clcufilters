﻿// -----------------------------------------------------------------------------------------
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

#include "NVEncUtil.h"
#include "NVEncParam.h"
#include "rgy_frame.h"
#include "rgy_aspect_ratio.h"

static const auto RGY_CODEC_TO_NVENC = make_array<std::pair<RGY_CODEC, cudaVideoCodec>>(
    std::make_pair(RGY_CODEC_H264,  cudaVideoCodec_H264),
    std::make_pair(RGY_CODEC_HEVC,  cudaVideoCodec_HEVC),
    std::make_pair(RGY_CODEC_MPEG1, cudaVideoCodec_MPEG1),
    std::make_pair(RGY_CODEC_MPEG2, cudaVideoCodec_MPEG2),
    std::make_pair(RGY_CODEC_MPEG4, cudaVideoCodec_MPEG4),
    std::make_pair(RGY_CODEC_VP8,   cudaVideoCodec_VP8),
    std::make_pair(RGY_CODEC_VP9,   cudaVideoCodec_VP9),
    std::make_pair(RGY_CODEC_VC1,   cudaVideoCodec_VC1),
    std::make_pair(RGY_CODEC_AV1,   cudaVideoCodec_AV1)
    );

MAP_PAIR_0_1(codec, rgy, RGY_CODEC, dec, cudaVideoCodec, RGY_CODEC_TO_NVENC, RGY_CODEC_UNKNOWN, cudaVideoCodec_NumCodecs);

const GUID GUID_EMPTY = { 0 };

static const auto RGY_CODEC_TO_GUID = make_array<std::pair<RGY_CODEC, GUID>>(
    std::make_pair(RGY_CODEC_H264, NV_ENC_CODEC_H264_GUID),
    std::make_pair(RGY_CODEC_HEVC, NV_ENC_CODEC_HEVC_GUID),
    std::make_pair(RGY_CODEC_AV1,  NV_ENC_CODEC_AV1_GUID)
    );

MAP_PAIR_0_1(codec_guid, rgy, RGY_CODEC, enc, GUID, RGY_CODEC_TO_GUID, RGY_CODEC_UNKNOWN, GUID_EMPTY);


static const auto RGY_CODEC_PROFILE_TO_GUID = make_array<std::pair<RGY_CODEC_DATA, GUID>>(
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_H264, 77),  NV_ENC_H264_PROFILE_BASELINE_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_H264, 88),  NV_ENC_H264_PROFILE_MAIN_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_H264, 100), NV_ENC_H264_PROFILE_HIGH_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_H264, 144), NV_ENC_H264_PROFILE_HIGH_444_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_HEVC, 1),   NV_ENC_HEVC_PROFILE_MAIN_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_HEVC, 2),   NV_ENC_HEVC_PROFILE_MAIN10_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_HEVC, 4),   NV_ENC_HEVC_PROFILE_FREXT_GUID),
    std::make_pair(RGY_CODEC_DATA(RGY_CODEC_AV1,  0),   NV_ENC_AV1_PROFILE_MAIN_GUID)
    //std::make_pair(RGY_CODEC_DATA(RGY_CODEC_AV1,  1),   NV_ENC_AV1_PROFILE_HIGH_GUID)
    );

MAP_PAIR_0_1(codec_guid_profile, rgy, RGY_CODEC_DATA, enc, GUID, RGY_CODEC_PROFILE_TO_GUID, RGY_CODEC_DATA(), GUID_EMPTY);

static const auto RGY_CHROMAFMT_TO_NVENC = make_array<std::pair<RGY_CHROMAFMT, cudaVideoChromaFormat>>(
    std::make_pair(RGY_CHROMAFMT_MONOCHROME, cudaVideoChromaFormat_Monochrome),
    std::make_pair(RGY_CHROMAFMT_YUV420,     cudaVideoChromaFormat_420),
    std::make_pair(RGY_CHROMAFMT_YUV422,     cudaVideoChromaFormat_422),
    std::make_pair(RGY_CHROMAFMT_YUV444,     cudaVideoChromaFormat_444)
    );

MAP_PAIR_0_1(chromafmt, rgy, RGY_CHROMAFMT, enc, cudaVideoChromaFormat, RGY_CHROMAFMT_TO_NVENC, RGY_CHROMAFMT_UNKNOWN, cudaVideoChromaFormat_Monochrome);

static const auto RGY_CSP_TO_NVENC = make_array<std::pair<RGY_CSP, NV_ENC_BUFFER_FORMAT>>(
    std::make_pair(RGY_CSP_NA,        NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_NV12,      NV_ENC_BUFFER_FORMAT_NV12),
    std::make_pair(RGY_CSP_YV12,      NV_ENC_BUFFER_FORMAT_YV12),
    std::make_pair(RGY_CSP_YUY2,      NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV422,    NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV444,    NV_ENC_BUFFER_FORMAT_YUV444),
    std::make_pair(RGY_CSP_YV12_09,   NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YV12_10,   NV_ENC_BUFFER_FORMAT_YUV420_10BIT),
    std::make_pair(RGY_CSP_YV12_12,   NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YV12_14,   NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YV12_16,   NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_P010,      NV_ENC_BUFFER_FORMAT_YUV420_10BIT),
    std::make_pair(RGY_CSP_NV12A,     NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_P010A,     NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV422_09, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV422_10, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV422_12, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV422_14, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV422_16, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_P210,      NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV444_09, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV444_10, NV_ENC_BUFFER_FORMAT_YUV444_10BIT),
    std::make_pair(RGY_CSP_YUV444_12, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV444_14, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YUV444_16, NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_BGR24R,    NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_BGR32R,    NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_RGB24,     NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_RGB32,     NV_ENC_BUFFER_FORMAT_ARGB),
    std::make_pair(RGY_CSP_BGR24,     NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_BGR32,     NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_RGB,       NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_RGBA,      NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_GBR,       NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_GBRA,      NV_ENC_BUFFER_FORMAT_UNDEFINED),
    std::make_pair(RGY_CSP_YC48,      NV_ENC_BUFFER_FORMAT_UNDEFINED)
    );

MAP_PAIR_0_1(csp, rgy, RGY_CSP, enc, NV_ENC_BUFFER_FORMAT, RGY_CSP_TO_NVENC, RGY_CSP_NA, NV_ENC_BUFFER_FORMAT_UNDEFINED);

static const auto RGY_CSP_TO_SURFACEFMT = make_array<std::pair<RGY_CSP, cudaVideoSurfaceFormat>>(
    std::make_pair(RGY_CSP_NV12,      cudaVideoSurfaceFormat_NV12),
    std::make_pair(RGY_CSP_P010,      cudaVideoSurfaceFormat_P016),
    std::make_pair(RGY_CSP_YUV444,    cudaVideoSurfaceFormat_YUV444),
    std::make_pair(RGY_CSP_YUV444_16, cudaVideoSurfaceFormat_YUV444_16Bit),
    std::make_pair(RGY_CSP_YUV444_12, cudaVideoSurfaceFormat_YUV444_16Bit),
    std::make_pair(RGY_CSP_YUV444_10, cudaVideoSurfaceFormat_YUV444_16Bit)
    );

MAP_PAIR_0_1(csp, rgy, RGY_CSP, surfacefmt, cudaVideoSurfaceFormat, RGY_CSP_TO_SURFACEFMT, RGY_CSP_NA, cudaVideoSurfaceFormat_NV12);

NV_ENC_PIC_STRUCT picstruct_rgy_to_enc(RGY_PICSTRUCT picstruct) {
    if (picstruct & RGY_PICSTRUCT_TFF) return NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM;
    if (picstruct & RGY_PICSTRUCT_BFF) return NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP;
    return NV_ENC_PIC_STRUCT_FRAME;
}

RGY_PICSTRUCT picstruct_enc_to_rgy(NV_ENC_PIC_STRUCT picstruct) {
    if (picstruct == NV_ENC_PIC_STRUCT_FIELD_TOP_BOTTOM) return RGY_PICSTRUCT_FRAME_TFF;
    if (picstruct == NV_ENC_PIC_STRUCT_FIELD_BOTTOM_TOP) return RGY_PICSTRUCT_FRAME_BFF;
    return RGY_PICSTRUCT_FRAME;
}

RGY_CSP getEncCsp(NV_ENC_BUFFER_FORMAT enc_buffer_format, const bool alphaChannel, const bool yuv444_as_rgb) {
    switch (enc_buffer_format) {
    case NV_ENC_BUFFER_FORMAT_NV12:
        return (alphaChannel) ? RGY_CSP_NV12A : RGY_CSP_NV12;
    case NV_ENC_BUFFER_FORMAT_YV12:
    case NV_ENC_BUFFER_FORMAT_IYUV:
        return (alphaChannel) ? RGY_CSP_YUVA420 : RGY_CSP_YV12;
    case NV_ENC_BUFFER_FORMAT_YUV444:
        if (alphaChannel) {
            return RGY_CSP_YUVA444;
        }
        if (yuv444_as_rgb) {
            return RGY_CSP_GBR;
        }
        return RGY_CSP_YUV444;
    case NV_ENC_BUFFER_FORMAT_YUV420_10BIT:
        return (alphaChannel) ? RGY_CSP_P010A : RGY_CSP_P010;
    case NV_ENC_BUFFER_FORMAT_YUV444_10BIT:
        if (alphaChannel) {
            return RGY_CSP_YUVA444_16;
        }
        if (yuv444_as_rgb) {
            return RGY_CSP_GBR_16;
        }
        return RGY_CSP_YUV444_16;
    case NV_ENC_BUFFER_FORMAT_ARGB:
        return RGY_CSP_RGB32;
    case NV_ENC_BUFFER_FORMAT_UNDEFINED:
    default:
        return RGY_CSP_NA;
    }
}

void RGYBitstream::addFrameData(RGYFrameData *frameData) {
    if (frameData != nullptr) {
        frameDataNum++;
        frameDataList = (RGYFrameData **)realloc(frameDataList, frameDataNum * sizeof(frameDataList[0]));
        frameDataList[frameDataNum - 1] = frameData;
    }
}

void RGYBitstream::clearFrameDataList() {
    frameDataNum = 0;
    if (frameDataList) {
        for (int i = 0; i < frameDataNum; i++) {
            if (frameDataList[i]) {
                delete frameDataList[i];
            }
        }
        free(frameDataList);
        frameDataList = nullptr;
    }
}
std::vector<RGYFrameData *> RGYBitstream::getFrameDataList() {
    return make_vector(frameDataList, frameDataNum);
}

VideoInfo videooutputinforaw(
    const GUID& encCodecGUID,
    NV_ENC_BUFFER_FORMAT buffer_fmt,
    int nEncWidth,
    int nEncHeight,
    NV_ENC_PIC_STRUCT nPicStruct,
    std::pair<int, int> sar,
    std::pair<int, int> outFps) {

    VideoInfo info;
    info.codec = RGY_CODEC_RAW;
    info.dstWidth = nEncWidth;
    info.dstHeight = nEncHeight;
    info.fpsN = outFps.first;
    info.fpsD = outFps.second;
    info.sar[0] = sar.first;
    info.sar[1] = sar.second;
    adjust_sar(&info.sar[0], &info.sar[1], nEncWidth, nEncHeight);
    info.picstruct = picstruct_enc_to_rgy(nPicStruct);
    info.csp = csp_enc_to_rgy(buffer_fmt);
    return info;
}

VideoInfo videooutputinfo(
    const GUID& encCodecGUID,
    NV_ENC_BUFFER_FORMAT buffer_fmt,
    int nEncWidth,
    int nEncHeight,
    const NV_ENC_CONFIG *pEncConfig,
    NV_ENC_PIC_STRUCT nPicStruct,
    std::pair<int, int> sar,
    rgy_rational<int> outFps) {

    VideoInfo info;
    info.dstWidth = nEncWidth;
    info.dstHeight = nEncHeight;
    info.fpsN = outFps.n();
    info.fpsD = outFps.d();
    info.sar[0] = sar.first;
    info.sar[1] = sar.second;
    adjust_sar(&info.sar[0], &info.sar[1], nEncWidth, nEncHeight);
    info.picstruct = picstruct_enc_to_rgy(nPicStruct);
    info.csp = csp_enc_to_rgy(buffer_fmt);

    if (pEncConfig) {
        info.codec = codec_guid_enc_to_rgy(encCodecGUID);
        info.codecLevel = (info.codec == RGY_CODEC_H264) ? pEncConfig->encodeCodecConfig.h264Config.level : pEncConfig->encodeCodecConfig.hevcConfig.level;
        info.codecProfile = codec_guid_profile_enc_to_rgy(pEncConfig->profileGUID).codecProfile;
        info.videoDelay = (info.codec == RGY_CODEC_AV1) ? 0 : ((pEncConfig->frameIntervalP - 2) > 0) + (((pEncConfig->frameIntervalP - 2) >= 2));
        info.vui.colorprim = (CspColorprim)get_colorprim(pEncConfig->encodeCodecConfig, info.codec);
        info.vui.matrix = (CspMatrix)get_colormatrix(pEncConfig->encodeCodecConfig, info.codec);
        info.vui.transfer = (CspTransfer)get_transfer(pEncConfig->encodeCodecConfig, info.codec);
        info.vui.colorrange = get_colorrange(pEncConfig->encodeCodecConfig, info.codec) ? RGY_COLORRANGE_FULL : RGY_COLORRANGE_UNSPECIFIED;
        info.vui.format = (int)get_videoFormat(pEncConfig->encodeCodecConfig, info.codec);
        if (info.codec == RGY_CODEC_H264 || info.codec == RGY_CODEC_HEVC) {
            info.vui.chromaloc = (get_chromaSampleLocationFlag(pEncConfig->encodeCodecConfig, info.codec)) ? (CspChromaloc)(get_chromaSampleLocationTop(pEncConfig->encodeCodecConfig, info.codec) + 1) : RGY_CHROMALOC_UNSPECIFIED;
        } else if (info.codec == RGY_CODEC_AV1) {
            switch (pEncConfig->encodeCodecConfig.av1Config.chromaSamplePosition) {
            case 1:  info.vui.chromaloc = RGY_CHROMALOC_LEFT; break;
            case 2:  info.vui.chromaloc = RGY_CHROMALOC_TOPLEFT; break;
            default: info.vui.chromaloc = RGY_CHROMALOC_UNSPECIFIED; break;
            }
        } else {
            info.vui.chromaloc = RGY_CHROMALOC_UNSPECIFIED;
        }
        info.vui.setDescriptPreset();
    } else {
        info.codec = RGY_CODEC_RAW;
    }
    return info;
}

#if !ENABLE_AVSW_READER
#define TTMATH_NOASM
#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4834)
#include "ttmath/ttmath.h"
#pragma warning(pop)

int64_t rational_rescale(int64_t v, rgy_rational<int> from, rgy_rational<int> to) {
    auto mul = rgy_rational<int64_t>((int64_t)from.n() * (int64_t)to.d(), (int64_t)from.d() * (int64_t)to.n());

#if _M_IX86
#define RESCALE_INT_SIZE 4
#else
#define RESCALE_INT_SIZE 2
#endif
    ttmath::Int<RESCALE_INT_SIZE> tmp1 = v;
    tmp1 *= mul.n();
    ttmath::Int<RESCALE_INT_SIZE> tmp2 = mul.d();

    tmp1 = (tmp1 + tmp2 - 1) / tmp2;
    int64_t ret;
    tmp1.ToInt(ret);
    return ret;
}

#endif