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

#ifndef __CLCUFILTERS_CHAIN_PRM_H__
#define __CLCUFILTERS_CHAIN_PRM_H__

#include <vector>
#include <cstdint>
#include "rgy_osdep.h"
#include "rgy_tchar.h"
#include "rgy_log.h"
#include "rgy_prm.h"

static const int SIZE_PIXEL_YC = 6;

static const int FILTER_NAME_MAX_LENGTH = 1024;

#if !CLFILTERS_EN
static const char *FILTER_NAME_COLORSPACE     = _T("色空間変換");
static const char *FILTER_NAME_NNEDI          = _T("nnedi");
static const char *FILTER_NAME_DENOISE_KNN    = _T("ノイズ除去 (knn)");
static const char *FILTER_NAME_DENOISE_PMD    = _T("ノイズ除去 (pmd)");
static const char *FILTER_NAME_DENOISE_SMOOTH = _T("ノイズ除去 (smooth)");
static const char *FILTER_NAME_RESIZE         = _T("リサイズ");
static const char *FILTER_NAME_UNSHARP        = _T("unsharp");
static const char *FILTER_NAME_EDGELEVEL      = _T("エッジレベル調整");
static const char *FILTER_NAME_WARPSHARP      = _T("warpsharp");
static const char *FILTER_NAME_TWEAK          = _T("色調補正");
static const char *FILTER_NAME_DEBAND         = _T("バンディング低減");
#else
static const char *FILTER_NAME_COLORSPACE     = _T("colorspace");
static const char *FILTER_NAME_NNEDI          = _T("nnedi");
static const char *FILTER_NAME_DENOISE_KNN    = _T("knn");
static const char *FILTER_NAME_DENOISE_PMD    = _T("pmd");
static const char *FILTER_NAME_DENOISE_SMOOTH = _T("smooth");
static const char *FILTER_NAME_RESIZE         = _T("resize");
static const char *FILTER_NAME_UNSHARP        = _T("unsharp");
static const char *FILTER_NAME_EDGELEVEL      = _T("edgelevel");
static const char *FILTER_NAME_WARPSHARP      = _T("warpsharp");
static const char *FILTER_NAME_TWEAK          = _T("tweak");
static const char *FILTER_NAME_DEBAND         = _T("deband");
#endif

#define FILTER_NAME(x) std::make_pair(FILTER_NAME_ ## x, VppType::CL_ ## x)

static const auto filterList = make_array<std::pair<const TCHAR*, VppType>>(
    FILTER_NAME(COLORSPACE),
    FILTER_NAME(NNEDI),
    FILTER_NAME(DENOISE_KNN),
    FILTER_NAME(DENOISE_PMD),
    FILTER_NAME(DENOISE_SMOOTH),
    FILTER_NAME(RESIZE),
    FILTER_NAME(UNSHARP),
    FILTER_NAME(EDGELEVEL),
    FILTER_NAME(WARPSHARP),
    FILTER_NAME(TWEAK),
    FILTER_NAME(DEBAND)
);

#undef FILTER_NAME

struct AviutlAufExeParams {
    RGYParamLogLevel log_level;
    tstring logfile;
    bool clinfo;
    bool checkDevice;
    uint32_t ppid;
    int max_w;
    int max_h;
    int sizeSharedPrm; // sizeof(clfitersSharedPrms)
    int sizeSharedMesData; // sizeof(clfitersSharedMesData)
    int sizePIXELYC; //sizeof(PIXEL_YC)
    HANDLE eventMesStart;
    HANDLE eventMesEnd;

    AviutlAufExeParams();
    ~AviutlAufExeParams();
};

static const int16_t CLCU_PLATFORM_CUDA = -1;

union CL_PLATFORM_DEVICE {
    struct {
        int16_t platform;
        int16_t device;
    } s;
    int i;
};

// 保存モード(is_saving=true)の時に、どのくらい先まで処理をしておくべきか?
static const int frameInOffset = 2;
static const int frameProcOffset = 1;
static const int frameOutOffset = 0;

struct clFilterChainParam {
    HMODULE hModule;
    RGYParamVpp vpp;
    RGYParamLogLevel log_level;
    bool log_to_file;
    int outWidth;
    int outHeight;

    clFilterChainParam();
    bool operator==(const clFilterChainParam &x) const;
    bool operator!=(const clFilterChainParam &x) const;
    std::vector<VppType> getFilterChain(const bool resizeRequired) const;
    tstring genCmd() const;
    void setPrmFromCmd(const tstring& cmd);
};

#endif //__CLCUFILTERS_CHAIN_PRM_H__
