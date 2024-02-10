﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
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

#include "NVEncFilterParam.h"

VppCustom::VppCustom() :
    enable(false),
    filter_name(),
    kernel_name(FILTER_DEFAULT_CUSTOM_KERNEL_NAME),
    kernel_path(),
    kernel(),
    dev_params(nullptr),
    compile_options(),
    kernel_interface(VPP_CUSTOM_INTERFACE_PER_PLANE),
    interlace(VPP_CUSTOM_INTERLACE_UNSUPPORTED),
    threadPerBlockX(FILTER_DEFAULT_CUSTOM_THREAD_PER_BLOCK_X),
    threadPerBlockY(FILTER_DEFAULT_CUSTOM_THREAD_PER_BLOCK_Y),
    pixelPerThreadX(FILTER_DEFAULT_CUSTOM_PIXEL_PER_THREAD_X),
    pixelPerThreadY(FILTER_DEFAULT_CUSTOM_PIXEL_PER_THREAD_Y),
    dstWidth(0),
    dstHeight(0),
    params() {

}

bool VppCustom::operator==(const VppCustom &x) const {
    return enable == x.enable
        && filter_name == x.filter_name
        && kernel_name == x.kernel_name
        && kernel_path == x.kernel_path
        && kernel == x.kernel
        && dev_params == x.dev_params
        && compile_options == x.compile_options
        && kernel_interface == x.kernel_interface
        && interlace == x.interlace
        && threadPerBlockX == x.threadPerBlockX
        && threadPerBlockY == x.threadPerBlockY
        && pixelPerThreadX == x.pixelPerThreadX
        && pixelPerThreadY == x.pixelPerThreadY
        && dstWidth == x.dstWidth
        && dstHeight == x.dstHeight
        && params == x.params;
}
bool VppCustom::operator!=(const VppCustom &x) const {
    return !(*this == x);
}

tstring VppCustom::print() const {
    return strsprintf(_T("%s: %s, interface %s, interlace %s\n")
        _T("                    thread/block (%d,%d), pixel/thread (%d,%d)\n"),
        filter_name.c_str(), kernel_path.c_str(),
        get_cx_desc(list_vpp_custom_interface, kernel_interface),
        get_cx_desc(list_vpp_custom_interlace, interlace),
        threadPerBlockX, threadPerBlockY,
        pixelPerThreadX, pixelPerThreadY);
}

VppNvvfxDenoise::VppNvvfxDenoise() :
    enable(false),
    strength(FILTER_DEFAULT_NVVFX_DENOISE_STRENGTH) {

}

bool VppNvvfxDenoise::operator==(const VppNvvfxDenoise &x) const {
    return enable == x.enable
        && strength == x.strength;
}
bool VppNvvfxDenoise::operator!=(const VppNvvfxDenoise &x) const {
    return !(*this == x);
}

tstring VppNvvfxDenoise::print() const {
    return strsprintf(_T("nvvfx-denoise: strength %.0f"),
        strength);
}

VppNvvfxArtifactReduction::VppNvvfxArtifactReduction() :
    enable(false),
    mode(FILTER_DEFAULT_NVVFX_ARTIFACT_REDUCTION_MODE) {

}

bool VppNvvfxArtifactReduction::operator==(const VppNvvfxArtifactReduction &x) const {
    return enable == x.enable
        && mode == x.mode;
}
bool VppNvvfxArtifactReduction::operator!=(const VppNvvfxArtifactReduction &x) const {
    return !(*this == x);
}

tstring VppNvvfxArtifactReduction::print() const {
    return strsprintf(_T("nvvfx-artifact-reduction: mode %d (%s)"),
        mode, get_cx_desc(list_vpp_nvvfx_mode, mode));
}

VppNvvfxSuperRes::VppNvvfxSuperRes() :
    enable(false),
    mode(FILTER_DEFAULT_NVVFX_SUPER_RES_MODE),
    strength(FILTER_DEFAULT_NVVFX_SUPER_RES_STRENGTH) {

}

bool VppNvvfxSuperRes::operator==(const VppNvvfxSuperRes &x) const {
    return enable == x.enable
        && mode == x.mode
        && strength == x.strength;
}
bool VppNvvfxSuperRes::operator!=(const VppNvvfxSuperRes &x) const {
    return !(*this == x);
}

tstring VppNvvfxSuperRes::print() const {
    return strsprintf(_T("nvvfx-superres: mode: %d (%s), strength %.2f"),
        mode, get_cx_desc(list_vpp_nvvfx_mode, mode), strength);
}

VppNvvfxUpScaler::VppNvvfxUpScaler() :
    enable(false),
    strength(FILTER_DEFAULT_NVVFX_UPSCALER_STRENGTH) {

}

bool VppNvvfxUpScaler::operator==(const VppNvvfxUpScaler &x) const {
    return enable == x.enable
        && strength == x.strength;
}
bool VppNvvfxUpScaler::operator!=(const VppNvvfxUpScaler &x) const {
    return !(*this == x);
}

tstring VppNvvfxUpScaler::print() const {
    return strsprintf(_T("nvvfx-upscaler: strength %.2f"),
        strength);
}


VppParam::VppParam() :
#if ENCODER_NVENC
    deinterlace(cudaVideoDeinterlaceMode_Weave),
#endif //#if ENCODER_NVENC
    gaussMaskSize((NppiMaskSize)0),
    nvvfxDenoise(),
    nvvfxArtifactReduction(),
    nvvfxSuperRes(),
    nvvfxUpScaler(),
    nvvfxModelDir() {
}
