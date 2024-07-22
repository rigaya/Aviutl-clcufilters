// -----------------------------------------------------------------------------------------
// clfilters by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2022 rigaya
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

#include "rgy_osdep.h"
#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include <algorithm>
#include <vector>
#include <cstdint>

#define DEFINE_GLOBAL
#include "filter.h"
#include "clcufilters_version.h"
#include "clcufilters_shared.h"
#include "clcufilters_auf.h"
#include "clcufilters.h"

void init_device_list();
void init_dialog(HWND hwnd, FILTER *fp);
void update_cx(FILTER *fp);

static_assert(sizeof(PIXEL_YC) == SIZE_PIXEL_YC);

#define ENABLE_FIELD (0)
#define ENABLE_HDR2SDR_DESAT (0)

enum {
    ID_LB_RESIZE_RES = ID_TX_RESIZE_RES_ADD+1,
    ID_CX_RESIZE_RES,
    ID_BT_RESIZE_RES_ADD,
    ID_BT_RESIZE_RES_DEL,
    ID_CX_RESIZE_ALGO,
    ID_LB_RESIZE_NGX_VSR_QUALITY,
    ID_CX_RESIZE_NGX_VSR_QUALITY,

    ID_LB_OPENCL_DEVICE,
    ID_CX_OPENCL_DEVICE,
    ID_BT_OPENCL_INFO,

    ID_LB_LOG_LEVEL,
    ID_CX_LOG_LEVEL,

    ID_LB_FILTER_ORDER,
    ID_LS_FILTER_ORDER,
    ID_BT_FILTER_ORDER_UP,
    ID_BT_FILTER_ORDER_DOWN,

    ID_LB_COLORSPACE_COLORMATRIX_FROM_TO,
    ID_CX_COLORSPACE_COLORMATRIX_FROM,
    ID_CX_COLORSPACE_COLORMATRIX_TO,
    ID_LB_COLORSPACE_COLORPRIM_FROM_TO,
    ID_CX_COLORSPACE_COLORPRIM_FROM,
    ID_CX_COLORSPACE_COLORPRIM_TO,
    ID_LB_COLORSPACE_TRANSFER_FROM_TO,
    ID_CX_COLORSPACE_TRANSFER_FROM,
    ID_CX_COLORSPACE_TRANSFER_TO,
    ID_LB_COLORSPACE_COLORRANGE_FROM_TO,
    ID_CX_COLORSPACE_COLORRANGE_FROM,
    ID_CX_COLORSPACE_COLORRANGE_TO,
    ID_LB_COLORSPACE_HDR2SDR,
    ID_CX_COLORSPACE_HDR2SDR,

    ID_LB_NNEDI_FIELD,
    ID_CX_NNEDI_FIELD,
    ID_LB_NNEDI_NSIZE,
    ID_CX_NNEDI_NSIZE,
    ID_LB_NNEDI_NNS,
    ID_CX_NNEDI_NNS,
    ID_LB_NNEDI_QUALITY,
    ID_CX_NNEDI_QUALITY,
    ID_LB_NNEDI_PRESCREEN,
    ID_CX_NNEDI_PRESCREEN,
    ID_LB_NNEDI_ERRORTYPE,
    ID_CX_NNEDI_ERRORTYPE,

    ID_LB_NVVFX_DENOISE_STRENGTH,
    ID_CX_NVVFX_DENOISE_STRENGTH,

    ID_LB_NVVFX_ARTIFACT_REDUCTION_MODE,
    ID_CX_NVVFX_ARTIFACT_REDUCTION_MODE,

    ID_LB_NVVFX_SUPRERES_MODE,
    ID_CX_NVVFX_SUPRERES_MODE,

    ID_LB_DENOISE_DCT_STEP,
    ID_CX_DENOISE_DCT_STEP,
    ID_LB_DENOISE_DCT_BLOCK_SIZE,
    ID_CX_DENOISE_DCT_BLOCK_SIZE,

    ID_LB_DENOISE_NLMEANS_PATCH,
    ID_CX_DENOISE_NLMEANS_PATCH,
    ID_LB_DENOISE_NLMEANS_SEARCH,
    ID_CX_DENOISE_NLMEANS_SEARCH,

    ID_LB_SMOOTH_QUALITY,
    ID_CX_SMOOTH_QUALITY,

    ID_LB_KNN_RADIUS,
    ID_CX_KNN_RADIUS,

    ID_LB_UNSHARP_RADIUS,
    ID_CX_UNSHARP_RADIUS,

    ID_LB_WARPSHARP_BLUR,
    ID_CX_WARPSHARP_BLUR,

    ID_LB_DEBAND_SAMPLE,
    ID_CX_DEBAND_SAMPLE,

    ID_TB_NGX_TRUEHDR_CONTRAST,
    ID_TB_NGX_TRUEHDR_SATURATION = ID_TB_NGX_TRUEHDR_CONTRAST + 5,
    ID_TB_NGX_TRUEHDR_MIDDLEGRAY = ID_TB_NGX_TRUEHDR_SATURATION + 5,
    ID_TB_NGX_TRUEHDR_MAXLUMINANCE = ID_TB_NGX_TRUEHDR_MIDDLEGRAY + 5,
};

#pragma pack(1)

struct CLFILTER_TRACKBAR {
    int id;
    HWND label;
    HWND trackbar;
    HWND bt_left;
    HWND bt_right;
    HWND bt_text;
};

struct CLFILTER_TRACKBAR_DATA {
    CLFILTER_TRACKBAR *tb;
    const char *labelText;
    int label_id;
    int val_min;
    int val_max;
    int val_default;
    int *ex_data_pos;
};

struct CLFILTER_EXDATA {
    CL_PLATFORM_DEVICE cl_dev_id;
    RGYLogLevel log_level;

    int resize_idx;
    int resize_algo;

    VideoVUIInfo csp_from, csp_to;
    HDR2SDRToneMap hdr2sdr;

    VppNnediField nnedi_field;
    int nnedi_nns;
    VppNnediNSize nnedi_nsize;
    VppNnediQuality nnedi_quality;
    VppNnediPreScreen nnedi_prescreen;
    VppNnediErrorType nnedi_errortype;

    int knn_radius;
    int pmd_apply_count;
    int smooth_quality;
    int unsharp_radius;
    int warpsharp_blur;
    int deband_sample;

    VppType filterOrder[64];

    int denoise_dct_step;
    int denoise_dct_block_size;

    int nvvfx_denoise_strength;
    int nvvfx_artifact_reduction_mode;
    int nvvfx_superres_mode;
    int nvvfx_superres_strength;

    int nlmeans_patch;
    int nlmeans_search;

    char resize_ngx_vsr_quality;
    char reserved3[3];

    int ngx_truehdr_contrast;
    int ngx_truehdr_saturation;
    int ngx_truehdr_middlegray;
    int ngx_truehdr_maxluminance;

    char reserved[592];
};
# pragma pack()
static const int extra_track_data_offset = 32;
static const size_t exdatasize = sizeof(CLFILTER_EXDATA);
static_assert(exdatasize == 1024);

static CLFILTER_EXDATA cl_exdata;

static std::unique_ptr<clcuFiltersAufDevices> g_clfiltersAufDevices;
static std::unique_ptr<clcuFiltersAuf> g_clfiltersAuf;
static int g_cuda_device_nvvfx_support = -1;
static int g_resize_nvvfx_superres = -1;
static int g_resize_ngx_vsr_quality = -1;
static std::vector<CLFILTER_TRACKBAR_DATA> g_trackBars;
static HBRUSH g_hbrBackground = NULL;

//---------------------------------------------------------------------
//        ラベル
//---------------------------------------------------------------------
#if !CLFILTERS_EN
static const char *LB_WND_OPENCL_UNAVAIL = "フィルタは無効です: OpenCLを使用できません。";
static const char *LB_WND_OPENCL_AVAIL = "OpenCL 有効";
static const char *LB_CX_OPENCL_DEVICE = "デバイス選択";
static const char *LB_CX_LOG_LEVEL = "ログ出力";
static const char *LB_CX_FILTER_ORDER = "フィルタ順序";
static const char *LB_CX_RESIZE_SIZE = "サイズ";
static const char *LB_BT_RESIZE_ADD = "追加";
static const char *LB_BT_RESIZE_DELETE = "削除";
static const char *TX_RESIZE_SIZE = "サイズ";
static const char *TX_RESIZE_ADD = "追加";
static const char *TX_RESIZE_DELETE = "削除";
static const char *LB_CX_RESIZE_NGX_VSR_QUALITY = "品質";
static const char *LB_CX_NNEDI_FIELD = "field";
static const char *LB_CX_NNEDI_NNS = "nns";
static const char *LB_CX_NNEDI_NSIZE = "nsize";
static const char *LB_CX_NNEDI_QUALITY = "品質";
static const char *LB_CX_NNEDI_PRESCREEN = "前処理";
static const char *LB_CX_NNEDI_ERRORTYPE = "errortype";
static const char *LB_CX_NVVFX_DENOISE_STRENGTH = "強度";
static const char *LB_CX_NVVFX_ARTIFACT_REDUCTION_MODE = "モード";
static const char *LB_CX_NVVFX_SUPRERES_MODE = "モード";
static const char *LB_CX_DENOISE_DCT_STEP = "品質";
static const char *LB_CX_DENOISE_DCT_BLOCK_SIZE = "ブロックサイズ";
static const char *LB_CX_DENOISE_NLMEANS_PATCH = "パッチサイズ";
static const char *LB_CX_DENOISE_NLMEANS_SEARCH = "探索範囲";
static const char *LB_CX_SMOOTH_QUALITY = "品質";
static const char *LB_CX_KNN_RADIUS = "適用半径";
static const char *LB_CX_UNSHARP_RADIUS = "範囲";
static const char *LB_CX_WARPSHARP_BLUR = "ブラー";
static const char *LB_CX_DEBAND_SAMPLE = "sample";
static const char *LB_CX_TRUEHDR_CONTRAST = "コントラスト";
static const char *LB_CX_TRUEHDR_SATURATION = "彩度";
static const char *LB_CX_TRUEHDR_MIDDLEGRAY = "目標輝度";
static const char *LB_CX_TRUEHDR_MAXLUMINANCE = "最大輝度";
#else
static const char *LB_WND_OPENCL_UNAVAIL = "Filter disabled, OpenCL could not be used.";
static const char *LB_WND_OPENCL_AVAIL = "OpenCL Enabled";
static const char *LB_CX_OPENCL_DEVICE = "Device";
static const char *LB_CX_LOG_LEVEL = "Log";
static const char *LB_CX_FILTER_ORDER = "Filter Order";
static const char *LB_CX_RESIZE_SIZE = "Size";
static const char *LB_BT_RESIZE_ADD = "Add";
static const char *LB_BT_RESIZE_DELETE = "Delete";
static const char *TX_RESIZE_SIZE = "Size";
static const char *TX_RESIZE_ADD = "Add";
static const char *TX_RESIZE_DELETE = "Delete";
static const char *LB_CX_RESIZE_NGX_VSR_QUALITY = "Quality";
static const char *LB_CX_NNEDI_FIELD = "field";
static const char *LB_CX_NNEDI_NNS = "nns";
static const char *LB_CX_NNEDI_NSIZE = "nsize";
static const char *LB_CX_NNEDI_QUALITY = "quality";
static const char *LB_CX_NNEDI_PRESCREEN = "prescreen";
static const char *LB_CX_NNEDI_ERRORTYPE = "errortype";
static const char *LB_CX_NVVFX_DENOISE_STRENGTH = "strength";
static const char *LB_CX_NVVFX_ARTIFACT_REDUCTION_MODE = "mode";
static const char *LB_CX_NVVFX_SUPRERES_MODE = "mode";
static const char *LB_CX_DENOISE_DCT_STEP = "step";
static const char *LB_CX_DENOISE_DCT_BLOCK_SIZE = "blocksize";
static const char *LB_CX_DENOISE_NLMEANS_PATCH = "patch size";
static const char *LB_CX_DENOISE_NLMEANS_SEARCH = "search size";
static const char *LB_CX_SMOOTH_QUALITY = "quality";
static const char *LB_CX_KNN_RADIUS = "radius";
static const char *LB_CX_UNSHARP_RADIUS = "radius";
static const char *LB_CX_WARPSHARP_BLUR = "blur";
static const char *LB_CX_DEBAND_SAMPLE = "sample";
static const char *LB_CX_TRUEHDR_CONTRAST = "contrast";
static const char *LB_CX_TRUEHDR_SATURATION = "saturation";
static const char *LB_CX_TRUEHDR_MIDDLEGRAY = "middlegray";
static const char *LB_CX_TRUEHDR_MAXLUMINANCE = "maxluminance";
#endif

//---------------------------------------------------------------------
//        フィルタ構造体定義
//---------------------------------------------------------------------
//  トラックバーの名前
const TCHAR *track_name_ja[] = {
    "強度", //リサイズ(nvvfx-superres)
    "入力ピーク輝度", "目標輝度", //colorspace
#if ENABLE_HDR2SDR_DESAT
    "脱飽和ｵﾌｾｯﾄ", "脱飽和強度", "脱飽和指数", //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    "sigma", //denoise-dct
    "QP", // smooth
    "強さ", "ブレンド度合い", "ブレンド閾値", //knn
    "分散", "強度", //nlmeans
    "適用回数", "強さ", "閾値", //pmd
    "強さ", "閾値", //unsharp
    "特性", "閾値", "黒", "白", //エッジレベル調整
    "閾値", "深度", //warpsharp
    "輝度", "コントラスト", "ガンマ", "彩度", "色相", //tweak
    "range", "Y", "C", "ditherY", "ditherC" //バンディング低減
};
const TCHAR *track_name_en[] = {
    "strength", //リサイズ(nvvfx-superres)
    "srcpeak", "ldr_nits", //colorspace
#if ENABLE_HDR2SDR_DESAT
    "desat_base", "desat_strength", "desat_exp", //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    "sigma", //denoise-dct
    "QP", // smooth
    "strength", "lerp", "th_lerp", //knn
    "sigma", "h", //nlmeans
    "apply cnt", "strength", "threshold", //pmd
    "weight", "threshold", //unsharp
    "strength", "threshold", "black", "white", //エッジレベル調整
    "threshold", "depth", //warpsharp
    "bright", "contrast", "gamma", "saturation", "hue", //tweak
    "range", "Y", "C", "ditherY", "ditherC" //バンディング低減
};
static_assert(_countof(track_name_ja) == _countof(track_name_en), "TRACK_N check");

#if CLFILTERS_EN
const TCHAR **track_name = track_name_en;
#else
const TCHAR **track_name = track_name_ja;
#endif

enum {
    CLFILTER_TRACK_RESIZE_FIRST = 0,
    CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH = CLFILTER_TRACK_RESIZE_FIRST,
    CLFILTER_TRACK_RESIZE_MAX,

    CLFILTER_TRACK_COLORSPACE_FIRST = CLFILTER_TRACK_RESIZE_MAX,
    CLFILTER_TRACK_COLORSPACE_SOURCE_PEAK = CLFILTER_TRACK_COLORSPACE_FIRST,
    CLFILTER_TRACK_COLORSPACE_LDR_NITS,
#if ENABLE_HDR2SDR_DESAT
    CLFILTER_TRACK_COLORSPACE_DESAT_BASE,
    CLFILTER_TRACK_COLORSPACE_DESAT_STRENGTH,
    CLFILTER_TRACK_COLORSPACE_DESAT_EXP,
#endif //#if ENABLE_HDR2SDR_DESAT
    CLFILTER_TRACK_COLORSPACE_MAX,

    CLFILTER_TRACK_NVVFX_DENOISE_FIRST = CLFILTER_TRACK_COLORSPACE_MAX,
    CLFILTER_TRACK_NVVFX_DENOISE_MAX = CLFILTER_TRACK_NVVFX_DENOISE_FIRST,

    CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_FIRST = CLFILTER_TRACK_NVVFX_DENOISE_MAX,
    CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_MAX = CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_FIRST,

    CLFILTER_TRACK_DENOISE_DCT_FIRST = CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_MAX,
    CLFILTER_TRACK_DENOISE_DCT_SIGMA = CLFILTER_TRACK_DENOISE_DCT_FIRST,
    CLFILTER_TRACK_DENOISE_DCT_MAX,

    CLFILTER_TRACK_SMOOTH_FIRST = CLFILTER_TRACK_DENOISE_DCT_MAX,
    CLFILTER_TRACK_SMOOTH_QP = CLFILTER_TRACK_SMOOTH_FIRST,
    CLFILTER_TRACK_SMOOTH_MAX,

    CLFILTER_TRACK_KNN_FIRST = CLFILTER_TRACK_SMOOTH_MAX,
    CLFILTER_TRACK_KNN_STRENGTH = CLFILTER_TRACK_KNN_FIRST,
    CLFILTER_TRACK_KNN_LERP,
    CLFILTER_TRACK_KNN_TH_LERP,
    CLFILTER_TRACK_KNN_MAX,

    CLFILTER_TRACK_NLMEANS_FIRST = CLFILTER_TRACK_KNN_MAX,
    CLFILTER_TRACK_NLMEANS_SIGMA = CLFILTER_TRACK_NLMEANS_FIRST,
    CLFILTER_TRACK_NLMEANS_H,
    CLFILTER_TRACK_NLMEANS_MAX,

    CLFILTER_TRACK_PMD_FIRST = CLFILTER_TRACK_NLMEANS_MAX,
    CLFILTER_TRACK_PMD_APPLY_COUNT = CLFILTER_TRACK_PMD_FIRST,
    CLFILTER_TRACK_PMD_STRENGTH,
    CLFILTER_TRACK_PMD_THRESHOLD,
    CLFILTER_TRACK_PMD_MAX,

    CLFILTER_TRACK_UNSHARP_FIRST = CLFILTER_TRACK_PMD_MAX,
    CLFILTER_TRACK_UNSHARP_WEIGHT = CLFILTER_TRACK_UNSHARP_FIRST,
    CLFILTER_TRACK_UNSHARP_THRESHOLD,
    CLFILTER_TRACK_UNSHARP_MAX,

    CLFILTER_TRACK_EDGELEVEL_FIRST = CLFILTER_TRACK_UNSHARP_MAX,
    CLFILTER_TRACK_EDGELEVEL_STRENGTH = CLFILTER_TRACK_EDGELEVEL_FIRST,
    CLFILTER_TRACK_EDGELEVEL_THRESHOLD,
    CLFILTER_TRACK_EDGELEVEL_BLACK,
    CLFILTER_TRACK_EDGELEVEL_WHITE,
    CLFILTER_TRACK_EDGELEVEL_MAX,

    CLFILTER_TRACK_WARPSHARP_FIRST = CLFILTER_TRACK_EDGELEVEL_MAX,
    CLFILTER_TRACK_WARPSHARP_THRESHOLD = CLFILTER_TRACK_WARPSHARP_FIRST,
    CLFILTER_TRACK_WARPSHARP_DEPTH,
    CLFILTER_TRACK_WARPSHARP_MAX,

    CLFILTER_TRACK_TWEAK_FIRST = CLFILTER_TRACK_WARPSHARP_MAX,
    CLFILTER_TRACK_TWEAK_BRIGHTNESS = CLFILTER_TRACK_TWEAK_FIRST,
    CLFILTER_TRACK_TWEAK_CONTRAST,
    CLFILTER_TRACK_TWEAK_GAMMA,
    CLFILTER_TRACK_TWEAK_SATURATION,
    CLFILTER_TRACK_TWEAK_HUE,
    CLFILTER_TRACK_TWEAK_MAX,

    CLFILTER_TRACK_DEBAND_FIRST = CLFILTER_TRACK_TWEAK_MAX,
    CLFILTER_TRACK_DEBAND_RANGE = CLFILTER_TRACK_DEBAND_FIRST,
    CLFILTER_TRACK_DEBAND_Y,
    CLFILTER_TRACK_DEBAND_C,
    CLFILTER_TRACK_DEBAND_DITHER_Y,
    CLFILTER_TRACK_DEBAND_DITHER_C,
    CLFILTER_TRACK_DEBAND_MAX,

    CLFILTER_TRACK_NNEDI_FIRST = CLFILTER_TRACK_DEBAND_MAX,
    CLFILTER_TRACK_NNEDI_MAX = CLFILTER_TRACK_NNEDI_FIRST,

    CLFILTER_TRACK_NGX_TRUEHDR_FIRST = CLFILTER_TRACK_NNEDI_MAX,
    CLFILTER_TRACK_NGX_TRUEHDR_MAX = CLFILTER_TRACK_NGX_TRUEHDR_FIRST,

    CLFILTER_TRACK_MAX = CLFILTER_TRACK_NNEDI_MAX,
};

//  トラックバーの初期値
int track_default[] = {
    50, //リサイズ(nvvfx-superres)
    1000, 100, //colorspace
#if ENABLE_HDR2SDR_DESAT
    18, 75, 15, //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    40, //denoise-dct
    12, //smooth
    8, 20, 80, //knn
    5, 50, //nlmeans
    2, 100, 100, //pmd
    5, 10, //unsharp
    5, 20, 0, 0, //エッジレベル調整
    128, 16, //warpsharp
    0, 100, 100, 100, 0, //tweak
    15, 15, 15, 15, 15 //バンディング低減
};
//  トラックバーの下限値
int track_s[] = {
    0, //リサイズ(nvvfx-superres)
    1, 1, //colorspace
#if ENABLE_HDR2SDR_DESAT
    0, 0, 1, //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    0, //denoise-dct
    1, //smooth
    0,0,0, //knn
    0,1,//nlmeans
    1,0,0, //pmd
    0,0, //unsharp
    -31,0,0,0, //エッジレベル調整
    0, -128, //warpsharp
    -100,-200,1,0,-180, //tweak
    0,0,0,0,0 //バンディング低減
};
//  トラックバーの上限値
int track_e[] = {
    100, //リサイズ(nvvfx-superres)
    5000, 1000, //colorspace
#if ENABLE_HDR2SDR_DESAT
    100, 100, 30, //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    500, //denoise-dct
    63, //smooth
    100, 100, 100, //knn
    1000,1000, //nlmeans
    10, 100, 255, //pmd
    100, 255, //unsharp
    31, 255, 31, 31, //エッジレベル調整
    255, 128, //warpsharp
    100,200,200,200,180, //tweak
    127,31,31,31,31//バンディング低減
};

//  トラックバーの数
#define    TRACK_N    (_countof(track_name_ja))
//static_assert(TRACK_N <= 32, "TRACK_N check");
static_assert(TRACK_N == CLFILTER_TRACK_MAX, "TRACK_N check");
static_assert(TRACK_N == _countof(track_default), "track_default check");
static_assert(TRACK_N == _countof(track_s), "track_s check");
static_assert(TRACK_N == _countof(track_e), "track_e check");

//  チェックボックスの名前
const TCHAR *check_name_ja[] = {
#if ENABLE_FIELD
    "フィールド処理",
#endif //#if ENABLE_FIELD
    "ファイルに出力", // log to file
    "リサイズ",
    "色空間変換", "matrix", "colorprim", "transfer", "range",
    "nnedi",
    "ノイズ除去 (nvvfx-denoise)",
    "ノイズ除去 (nvvfx-artifact-reduction)",
    "ノイズ除去 (denoise-dct)",
    "ノイズ除去 (smooth)",
    "ノイズ除去 (knn)",
    "ノイズ除去 (nlmeans)",
    "ノイズ除去 (pmd)",
    "unsharp",
    "エッジレベル調整",
    "warpsharp", "マスクサイズ off:13x13, on:5x5", "色差マスク",
    "色調補正",
    "バンディング低減", "ブラー処理を先に", "毎フレーム乱数を生成",
    "TrueHDR"
};
const TCHAR *check_name_en[] = {
#if ENABLE_FIELD
    "process field",
#endif //#if ENABLE_FIELD
    "log to file", // log to file
    "resize",
    "colorspace", "matrix", "colorprim", "transfer", "range",
    "nnedi",
    "nvvfx-denoise",
    "nvvfx-artifact-reduction",
    "denoise-dct",
    "smooth",
    "knn",
    "nlmeans",
    "pmd",
    "unsharp",
    "edgelevel",
    "warpsharp", "type [off:13x13, on:5x5]", "chroma",
    "tweak",
    "deband", "blurfirst", "rand_each_frame",
    "TrueHDR"
};
static_assert(_countof(check_name_ja) == _countof(check_name_en), "CHECK_N check");

#if CLFILTERS_EN
const TCHAR **check_name = check_name_en;
#else
const TCHAR **check_name = check_name_ja;
#endif

enum {
#if ENABLE_FIELD
    CLFILTER_CHECK_FIELD,
#endif //#if ENABLE_FIELD
    CLFILTER_CHECK_LOG_TO_FILE,
    CLFILTER_CHECK_LOG_MAX,

    CLFILTER_CHECK_RESIZE_ENABLE = CLFILTER_CHECK_LOG_MAX,
    CLFILTER_CHECK_RESIZE_MAX,

    CLFILTER_CHECK_COLORSPACE_ENABLE = CLFILTER_CHECK_RESIZE_MAX,
    CLFILTER_CHECK_COLORSPACE_MATRIX_ENABLE,
    CLFILTER_CHECK_COLORSPACE_COLORPRIM_ENABLE,
    CLFILTER_CHECK_COLORSPACE_TRANSFER_ENABLE,
    CLFILTER_CHECK_COLORSPACE_RANGE_ENABLE,
    CLFILTER_CHECK_COLORSPACE_MAX,

    CLFILTER_CHECK_NNEDI_ENABLE = CLFILTER_CHECK_COLORSPACE_MAX,
    CLFILTER_CHECK_NNEDI_MAX,

    CLFILTER_CHECK_NVVFX_DENOISE_ENABLE = CLFILTER_CHECK_NNEDI_MAX,
    CLFILTER_CHECK_NVVFX_DENOISE_MAX,

    CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE = CLFILTER_CHECK_NVVFX_DENOISE_MAX,
    CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_MAX,

    CLFILTER_CHECK_DENOISE_DCT_ENABLE = CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_MAX,
    CLFILTER_CHECK_DENOISE_DCT_MAX,

    CLFILTER_CHECK_SMOOTH_ENABLE = CLFILTER_CHECK_DENOISE_DCT_MAX,
    CLFILTER_CHECK_SMOOTH_MAX,

    CLFILTER_CHECK_KNN_ENABLE = CLFILTER_CHECK_SMOOTH_MAX,
    CLFILTER_CHECK_KNN_MAX,

    CLFILTER_CHECK_NLMEANS_ENABLE = CLFILTER_CHECK_KNN_MAX,
    CLFILTER_CHECK_NLMEANS_MAX,

    CLFILTER_CHECK_PMD_ENABLE = CLFILTER_CHECK_NLMEANS_MAX,
    CLFILTER_CHECK_PMD_MAX,

    CLFILTER_CHECK_UNSHARP_ENABLE = CLFILTER_CHECK_PMD_MAX,
    CLFILTER_CHECK_UNSHARP_MAX,

    CLFILTER_CHECK_EDGELEVEL_ENABLE = CLFILTER_CHECK_UNSHARP_MAX,
    CLFILTER_CHECK_EDGELEVEL_MAX,

    CLFILTER_CHECK_WARPSHARP_ENABLE = CLFILTER_CHECK_EDGELEVEL_MAX,
    CLFILTER_CHECK_WARPSHARP_SMALL_BLUR,
    CLFILTER_CHECK_WARPSHARP_CHROMA_MASK,
    CLFILTER_CHECK_WARPSHARP_MAX,

    CLFILTER_CHECK_TWEAK_ENABLE = CLFILTER_CHECK_WARPSHARP_MAX,
    CLFILTER_CHECK_TWEAK_MAX,

    CLFILTER_CHECK_DEBAND_ENABLE = CLFILTER_CHECK_TWEAK_MAX,
    CLFILTER_CHECK_DEBAND_BLUR_FIRST,
    CLFILTER_CHECK_DEBAND_RAND_EACH_FRAME,
    CLFILTER_CHECK_DEBAND_MAX,

    CLFILTER_CHECK_TRUEHDR_ENABLE = CLFILTER_CHECK_DEBAND_MAX,
    CLFILTER_CHECK_TRUEHDR_MAX,

    CLFILTER_CHECK_MAX = CLFILTER_CHECK_TRUEHDR_MAX,
};

//  チェックボックスの初期値 (値は0か1)
int check_default[] = {
#if ENABLE_FIELD
    0, // field
#endif //#if ENABLE_FIELD
    0, // log to file
    0, // resize
    0, 0, 0, 0, 0, // colorspace
    0, //nnedi
    0, // nvvfx-denoise
    0, // nvvfx-artifact-reduction
    0, // denoise-dct
    0, // smooth
    0, // knn
    0, // nlmeans
    0, // pmd
    0, // unsharp
    0, // edgelevel
    0, 0, 0, // warpsharp
    0, // tweak
    0, 0, 0, // deband
    0 // TrueHDR
};
//  チェックボックスの数
#define    CHECK_N    (_countof(check_name_ja))
static_assert(CHECK_N <= 32, "CHECK_N check");
static_assert(CHECK_N == CLFILTER_CHECK_MAX, "CHECK_N check");
static_assert(CHECK_N == _countof(check_default), "track_default check");

FILTER_DLL filter = {
    FILTER_FLAG_EX_INFORMATION | FILTER_FLAG_EX_DATA,
                                //    フィルタのフラグ
                                //    FILTER_FLAG_ALWAYS_ACTIVE        : フィルタを常にアクティブにします
                                //    FILTER_FLAG_CONFIG_POPUP        : 設定をポップアップメニューにします
                                //    FILTER_FLAG_CONFIG_CHECK        : 設定をチェックボックスメニューにします
                                //    FILTER_FLAG_CONFIG_RADIO        : 設定をラジオボタンメニューにします
                                //    FILTER_FLAG_EX_DATA                : 拡張データを保存出来るようにします。
                                //    FILTER_FLAG_PRIORITY_HIGHEST    : フィルタのプライオリティを常に最上位にします
                                //    FILTER_FLAG_PRIORITY_LOWEST        : フィルタのプライオリティを常に最下位にします
                                //    FILTER_FLAG_WINDOW_THICKFRAME    : サイズ変更可能なウィンドウを作ります
                                //    FILTER_FLAG_WINDOW_SIZE            : 設定ウィンドウのサイズを指定出来るようにします
                                //    FILTER_FLAG_DISP_FILTER            : 表示フィルタにします
                                //    FILTER_FLAG_EX_INFORMATION        : フィルタの拡張情報を設定できるようにします
                                //    FILTER_FLAG_NO_CONFIG            : 設定ウィンドウを表示しないようにします
                                //    FILTER_FLAG_AUDIO_FILTER        : オーディオフィルタにします
                                //    FILTER_FLAG_RADIO_BUTTON        : チェックボックスをラジオボタンにします
                                //    FILTER_FLAG_WINDOW_HSCROLL        : 水平スクロールバーを持つウィンドウを作ります
                                //    FILTER_FLAG_WINDOW_VSCROLL        : 垂直スクロールバーを持つウィンドウを作ります
                                //    FILTER_FLAG_IMPORT                : インポートメニューを作ります
                                //    FILTER_FLAG_EXPORT                : エクスポートメニューを作ります
    0,0,                        //    設定ウインドウのサイズ (FILTER_FLAG_WINDOW_SIZEが立っている時に有効)
    (char *)AUF_FULL_NAME,      //    フィルタの名前
    TRACK_N,                    //    トラックバーの数 (0なら名前初期値等もNULLでよい)
    (char **)track_name,        //    トラックバーの名前郡へのポインタ
    track_default,              //    トラックバーの初期値郡へのポインタ
    track_s,track_e,            //    トラックバーの数値の下限上限 (NULLなら全て0～256)
    CHECK_N,                    //    チェックボックスの数 (0なら名前初期値等もNULLでよい)
    (char **)check_name,        //    チェックボックスの名前郡へのポインタ
    check_default,              //    チェックボックスの初期値郡へのポインタ
    func_proc,                  //    フィルタ処理関数へのポインタ (NULLなら呼ばれません)
    func_init,                  //    開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_exit,                  //    終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_update,                //    設定が変更されたときに呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_WndProc,               //    設定ウィンドウにウィンドウメッセージが来た時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    NULL,NULL,                  //    システムで使いますので使用しないでください
    &cl_exdata,                 //  拡張データ領域へのポインタ (FILTER_FLAG_EX_DATAが立っている時に有効)
    sizeof(cl_exdata),          //  拡張データサイズ (FILTER_FLAG_EX_DATAが立っている時に有効)
    (char *)AUF_VERSION_NAME,
                                //  フィルタ情報へのポインタ (FILTER_FLAG_EX_INFORMATIONが立っている時に有効)
    NULL,                       //    セーブが開始される直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    NULL,                       //    セーブが終了した直前に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};

//---------------------------------------------------------------------
//        フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void )
{
    init_device_list();
    return &filter;
}

BOOL func_init( FILTER *fp ) {
    return TRUE;
}

BOOL func_exit( FILTER *fp ) {
    g_clfiltersAuf.reset();
    return TRUE;
}

BOOL func_update(FILTER* fp, int status) {
    update_cx(fp);
    return TRUE;
}

static void cl_exdata_set_default() {
    cl_exdata.cl_dev_id.i = 0;
    cl_exdata.log_level = RGY_LOG_QUIET;

    cl_exdata.resize_idx = 0;
    cl_exdata.resize_algo = RGY_VPP_RESIZE_SPLINE36;

    cl_exdata.resize_ngx_vsr_quality = FILTER_DEFAULT_NGX_VSR_QUALITY;

    cl_exdata.csp_from = VideoVUIInfo();
    cl_exdata.csp_to = VideoVUIInfo();
    cl_exdata.hdr2sdr = HDR2SDR_DISABLED;

    VppNnedi nnedi;
    cl_exdata.nnedi_field = VPP_NNEDI_FIELD_USE_TOP;
    cl_exdata.nnedi_nns = nnedi.nns;
    cl_exdata.nnedi_nsize = nnedi.nsize;
    cl_exdata.nnedi_quality = nnedi.quality;
    cl_exdata.nnedi_prescreen = nnedi.pre_screen;
    cl_exdata.nnedi_errortype = nnedi.errortype;

    VppNvvfxDenoise nvvfxdenoise;
    cl_exdata.nvvfx_denoise_strength = (int)(nvvfxdenoise.strength + 0.5);

    VppNvvfxArtifactReduction nvvfxArtifactReduction;
    cl_exdata.nvvfx_artifact_reduction_mode = nvvfxArtifactReduction.mode;

    VppNvvfxSuperRes nvvfxSuperRes;
    cl_exdata.nvvfx_superres_mode = nvvfxSuperRes.mode;
    cl_exdata.nvvfx_superres_strength = (int)(nvvfxSuperRes.strength * 100.0f + 0.5f);

    VppDenoiseDct dct;
    cl_exdata.denoise_dct_step = dct.step;
    cl_exdata.denoise_dct_block_size = dct.block_size;

    VppSmooth smooth;
    cl_exdata.smooth_quality = smooth.quality;

    VppKnn knn;
    cl_exdata.knn_radius = knn.radius;

    VppNLMeans nlmeans;
    cl_exdata.nlmeans_patch = nlmeans.patchSize;
    cl_exdata.nlmeans_search = nlmeans.searchSize;

    VppPmd pmd;
    cl_exdata.pmd_apply_count = pmd.applyCount;

    VppUnsharp unsharp;
    cl_exdata.unsharp_radius = unsharp.radius;

    VppWarpsharp warpsharp;
    cl_exdata.warpsharp_blur = warpsharp.blur;

    VppDeband deband;
    cl_exdata.deband_sample = deband.sample;

    VppNGXTrueHDR ngxTrueHDR;
    cl_exdata.ngx_truehdr_contrast = ngxTrueHDR.contrast;
    cl_exdata.ngx_truehdr_saturation = ngxTrueHDR.saturation;
    cl_exdata.ngx_truehdr_middlegray = ngxTrueHDR.middleGray;
    cl_exdata.ngx_truehdr_maxluminance = ngxTrueHDR.maxLuminance;
}

//---------------------------------------------------------------------
//        設定画面処理
//---------------------------------------------------------------------
static HWND lb_device;
static std::vector<HWND> child_hwnd;
static HWND lb_proc_mode;
static std::vector<std::pair<int, int>> resize_res;
static HWND lb_opencl_device;
static HWND cx_opencl_device;
static HWND bt_opencl_info;
static HWND lb_log_level;
static HWND cx_log_level;
static HWND cx_resize_res;
static HWND bt_resize_res_add;
static HWND bt_resize_res_del;
static HWND cx_resize_algo;

static HWND lb_resize_ngx_vsr_quality;
static HWND cx_resize_ngx_vsr_quality;

static HWND lb_filter_order;
static HWND ls_filter_order;
static HWND bt_filter_order_up;
static HWND bt_filter_order_down;

static HWND lb_colorspace_colormatrix_from_to;
static HWND cx_colorspace_colormatrix_from;
static HWND cx_colorspace_colormatrix_to;
static HWND lb_colorspace_colorprim_from_to;
static HWND cx_colorspace_colorprim_from;
static HWND cx_colorspace_colorprim_to;
static HWND lb_colorspace_transfer_from_to;
static HWND cx_colorspace_transfer_from;
static HWND cx_colorspace_transfer_to;
static HWND lb_colorspace_colorrange_from_to;
static HWND cx_colorspace_colorrange_from;
static HWND cx_colorspace_colorrange_to;
static HWND lb_colorspace_hdr2sdr;
static HWND cx_colorspace_hdr2sdr;

static HWND lb_nnedi_field;
static HWND cx_nnedi_field;
static HWND lb_nnedi_nsize;
static HWND cx_nnedi_nsize;
static HWND lb_nnedi_nns;
static HWND cx_nnedi_nns;
static HWND lb_nnedi_quality;
static HWND cx_nnedi_quality;
static HWND lb_nnedi_prescreen;
static HWND cx_nnedi_prescreen;
static HWND lb_nnedi_errortype;
static HWND cx_nnedi_errortype;

static HWND lb_nvvfx_denoise_strength;
static HWND cx_nvvfx_denoise_strength;
static HWND lb_nvvfx_artifact_reduction_mode;
static HWND cx_nvvfx_artifact_reduction_mode;
static HWND lb_nvvfx_superres_mode;
static HWND cx_nvvfx_superres_mode;

static HWND lb_knn_radius;
static HWND cx_knn_radius;
static HWND lb_nlmeans_patch;
static HWND cx_nlmeans_patch;
static HWND lb_nlmeans_search;
static HWND cx_nlmeans_search;
static HWND lb_denoise_dct_step;
static HWND cx_denoise_dct_step;
static HWND lb_denoise_dct_block_size;
static HWND cx_denoise_dct_block_size;
static HWND lb_smooth_quality;
static HWND cx_smooth_quality;
static HWND lb_unsharp_radius;
static HWND cx_unsharp_radius;
static HWND lb_warpsharp_blur;
static HWND cx_warpsharp_blur;

static HWND lb_deband_sample;
static HWND cx_deband_sample;

static CLFILTER_TRACKBAR tb_ngx_truehdr_contrast;
static CLFILTER_TRACKBAR tb_ngx_truehdr_saturation;
static CLFILTER_TRACKBAR tb_ngx_truehdr_middlegray;
static CLFILTER_TRACKBAR tb_ngx_truehdr_maxluminance;


static void set_cl_exdata(const HWND hwnd, const int value) {
    if (hwnd == cx_opencl_device) {
        cl_exdata.cl_dev_id.i = value;
    } else if (hwnd == cx_log_level) {
        cl_exdata.log_level = (RGYLogLevel)value;
    } else if (hwnd == cx_resize_res) {
        cl_exdata.resize_idx = value;
    } else if (hwnd == cx_resize_algo) {
        cl_exdata.resize_algo = value;
    } else if (hwnd == cx_resize_ngx_vsr_quality) {
        cl_exdata.resize_ngx_vsr_quality = (char)value;
    } else if (hwnd == cx_colorspace_colormatrix_from) {
        cl_exdata.csp_from.matrix = (CspMatrix)value;
    } else if (hwnd == cx_colorspace_colormatrix_to) {
        cl_exdata.csp_to.matrix = (CspMatrix)value;
    } else if (hwnd == cx_colorspace_colorprim_from) {
        cl_exdata.csp_from.colorprim = (CspColorprim)value;
    } else if (hwnd == cx_colorspace_colorprim_to) {
        cl_exdata.csp_to.colorprim = (CspColorprim)value;
    } else if (hwnd == cx_colorspace_transfer_from) {
        cl_exdata.csp_from.transfer = (CspTransfer)value;
    } else if (hwnd == cx_colorspace_transfer_to) {
        cl_exdata.csp_to.transfer = (CspTransfer)value;
    } else if (hwnd == cx_colorspace_colorrange_from) {
        cl_exdata.csp_from.colorrange = (CspColorRange)value;
    } else if (hwnd == cx_colorspace_colorrange_to) {
        cl_exdata.csp_to.colorrange = (CspColorRange)value;
    } else if (hwnd == cx_colorspace_hdr2sdr) {
        cl_exdata.hdr2sdr = (HDR2SDRToneMap)value;
    } else if (hwnd == cx_nnedi_field) {
        cl_exdata.nnedi_field = (VppNnediField)value;
    } else if (hwnd == cx_nnedi_nns) {
        cl_exdata.nnedi_nns = value;
    } else if (hwnd == cx_nnedi_nsize) {
        cl_exdata.nnedi_nsize = (VppNnediNSize)value;
    } else if (hwnd == cx_nnedi_quality) {
        cl_exdata.nnedi_quality = (VppNnediQuality)value;
    } else if (hwnd == cx_nnedi_prescreen) {
        cl_exdata.nnedi_prescreen = (VppNnediPreScreen)value;
    } else if (hwnd == cx_nnedi_errortype) {
        cl_exdata.nnedi_errortype = (VppNnediErrorType)value;
    } else if (hwnd == cx_nvvfx_denoise_strength) {
        cl_exdata.nvvfx_denoise_strength = value;
    } else if (hwnd == cx_nvvfx_artifact_reduction_mode) {
        cl_exdata.nvvfx_artifact_reduction_mode = value;
    } else if (hwnd == cx_nvvfx_superres_mode) {
        cl_exdata.nvvfx_superres_mode = value;
    } else if (hwnd == cx_knn_radius) {
        cl_exdata.knn_radius = value;
    } else if (hwnd == cx_nlmeans_patch) {
        cl_exdata.nlmeans_patch = value;
    } else if (hwnd == cx_nlmeans_search) {
        cl_exdata.nlmeans_search = value;
    } else if (hwnd == cx_denoise_dct_step) {
        cl_exdata.denoise_dct_step = value;
    } else if (hwnd == cx_denoise_dct_block_size) {
        cl_exdata.denoise_dct_block_size = value;
    } else if (hwnd == cx_smooth_quality) {
        cl_exdata.smooth_quality = value;
    } else if (hwnd == cx_unsharp_radius) {
        cl_exdata.unsharp_radius = value;
    } else if (hwnd == cx_warpsharp_blur) {
        cl_exdata.warpsharp_blur = value;
    } else if (hwnd == cx_deband_sample) {
        cl_exdata.deband_sample = value;
    }
}

CLFILTER_TRACKBAR create_trackbar_ex(HWND hwndParent, HINSTANCE hInstance, const char *labelText,
    const int x, const int y, const int trackbar_label_id, const int val_min, const int val_max, const int val_default) {
    CLFILTER_TRACKBAR trackbar;
    trackbar.id = trackbar_label_id;
    
    HFONT b_font = CreateFont(14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, "Meiryo UI");
    
    // ラベルの作成
    trackbar.label = CreateWindow(
        "STATIC", labelText, SS_SIMPLE|WS_CHILD|WS_VISIBLE,
        x, y, 50, 20, hwndParent, (HMENU)trackbar_label_id, hInstance, NULL);
    SendMessage(trackbar.label, WM_SETFONT, (WPARAM)b_font, 0);

    // トラックバーの作成
    trackbar.trackbar = CreateWindowEx(
        0, TRACKBAR_CLASS, NULL, WS_CHILD | WS_VISIBLE | TBS_HORZ,
        x + 52, y, 190, 25, hwndParent, (HMENU)(trackbar_label_id+1), hInstance, NULL);
    SendMessage(trackbar.trackbar, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(trackbar.trackbar, TBM_SETRANGE, TRUE, MAKELONG(val_min, val_max));
    SendMessage(trackbar.trackbar, TBM_SETPOS, TRUE, val_default);
    SendMessage(trackbar.trackbar, TBM_SETPAGESIZE, val_default, 1);
    SendMessage(trackbar.trackbar, TBM_SETTICFREQ, std::max((val_max - val_min) / 2, 0), 0);

    // 左ボタンの作成
    auto handleAviutl = GetModuleHandle(nullptr);
    auto hIconLeft = LoadImage(handleAviutl, "ICON_LEFT", IMAGE_ICON, 0, 0, 0);
    trackbar.bt_left = CreateWindowExW(
        0, L"BUTTON", hIconLeft ? L"" : L"◄", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,
        x + 242, y, 16, 16, hwndParent, (HMENU)(trackbar_label_id+2), hInstance, NULL);
    SendMessage(trackbar.bt_left, WM_SETFONT, (WPARAM)b_font, 0);
    if (hIconLeft) {
        SendMessage(trackbar.bt_left, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconLeft);
    }

    // 右ボタンの作成
    auto hIconRight = LoadImage(handleAviutl, "ICON_RIGHT", IMAGE_ICON, 0, 0, 0);
    trackbar.bt_right = CreateWindowExW(
        0, L"BUTTON", hIconRight ? L"" : L"►", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,
        x + 258, y, 16, 16, hwndParent, (HMENU)(trackbar_label_id+3), hInstance, NULL);
    SendMessage(trackbar.bt_right, WM_SETFONT, (WPARAM)b_font, 0);
    if (hIconRight) {
        SendMessage(trackbar.bt_right, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconRight);
    }

    // テキストボックスの作成
    trackbar.bt_text = CreateWindow(
        "EDIT", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_LEFT,
        x + 274, y, 30, 20, hwndParent, (HMENU)(trackbar_label_id+4), hInstance, NULL);
    SendMessage(trackbar.bt_text, WM_SETFONT, (WPARAM)b_font, 0);

    //テキストボックスにデフォルト値を設定
    SetWindowText(trackbar.bt_text, strsprintf("%d", val_default).c_str());
    return trackbar;
}

void create_trackbars_ex(HWND hwndParent, HINSTANCE hInstance, const int x, int& y_pos, int track_bar_delta_y, const CLFILTER_TRACKBAR_DATA *trackbars) {
    for (; trackbars->tb != nullptr; trackbars++, y_pos += track_bar_delta_y) {
        (*trackbars->tb) = create_trackbar_ex(hwndParent, hInstance, trackbars->labelText, x, y_pos, trackbars->label_id, trackbars->val_min, trackbars->val_max, trackbars->val_default);
        g_trackBars.push_back((*trackbars));
    }
}

const CLFILTER_TRACKBAR_DATA *get_trackbar_data(const int control_id) {
    for (const auto& tb : g_trackBars) {
        if (tb.label_id <= control_id && control_id < tb.label_id + 5) {
            return &tb;
        }
    }
    return nullptr;
}

const CLFILTER_TRACKBAR_DATA *get_trackbar_data_from_handle(const HWND handle) {
    for (const auto& tb : g_trackBars) {
        if (tb.tb) {
            if (   tb.tb->label    == handle
                || tb.tb->trackbar == handle
                || tb.tb->bt_left  == handle
                || tb.tb->bt_right == handle
                || tb.tb->bt_text  == handle) {
                return &tb;
            }
        }
    }
    return nullptr;
}

static void set_filter_order() {
    static_assert((size_t)(VppType::CL_MAX) <= _countof(cl_exdata.filterOrder));
    memset(cl_exdata.filterOrder, 0, sizeof(cl_exdata.filterOrder));

    const int n = SendMessage(ls_filter_order, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < std::min<int>(n, _countof(cl_exdata.filterOrder)); i++) {
        const auto id = (VppType)SendMessage(ls_filter_order, LB_GETITEMDATA, i, 0);
        cl_exdata.filterOrder[i] = id;
    }
}

static void change_cx_param(HWND hwnd) {
    LRESULT ret;

    // 選択番号取得
    ret = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    ret = SendMessage(hwnd, CB_GETITEMDATA, ret, 0);

    if (ret != CB_ERR) {
        set_cl_exdata(hwnd, ret);
    }
}

static void lstbox_move_up(HWND hdlg) {
    // 選択位置取得
    int n = SendMessage(hdlg, LB_GETCURSEL, 0, 0);
    if (n == 0 || n == LB_ERR) {// 一番上のときor選択されていない
        SendMessage(hdlg, LB_SETCURSEL, n, 0);
        return;
    }

    // データ・文字列取得
    char str[FILTER_NAME_MAX_LENGTH] = { 0 };
    void *data = (void *)SendMessage(hdlg, LB_GETITEMDATA, n, 0);
    SendMessage(hdlg, LB_GETTEXT, n, (LPARAM)str);

    // 削除
    SendMessage(hdlg, LB_DELETESTRING, n, 0);

    // 挿入
    n--;	// 一つ上
    SendMessage(hdlg, LB_INSERTSTRING, n, (LPARAM)str);
    SendMessage(hdlg, LB_SETITEMDATA, n, (LPARAM)data);

    SendMessage(hdlg, LB_SETCURSEL, n, 0);
}

static void lstbox_move_down(HWND hdlg) {
    // 選択位置取得
    int n = SendMessage(hdlg, LB_GETCURSEL, 0, 0);
    int count = SendMessage(hdlg, LB_GETCOUNT, 0, 0);
    if (n == count - 1 || n == LB_ERR) { // 一番下or選択されていない
        SendMessage(hdlg, LB_SETCURSEL, n, 0);
        return;
    }

    // データ・文字列取得
    char str[FILTER_NAME_MAX_LENGTH] = { 0 };
    void *data = (void *)SendMessage(hdlg, LB_GETITEMDATA, n, 0);
    SendMessage(hdlg, LB_GETTEXT, n, (LPARAM)str);

    // 削除
    SendMessage(hdlg, LB_DELETESTRING, n, 0);

    // 挿入
    n++;	// 一つ下
    SendMessage(hdlg, LB_INSERTSTRING, n, (LPARAM)str);
    SendMessage(hdlg, LB_SETITEMDATA, n, (LPARAM)data);

    SendMessage(hdlg, LB_SETCURSEL, n, 0);
}

static int select_combo_item(HWND hwnd, int data) {
    const int current_sel = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    int current_data = SendMessage(hwnd, CB_GETITEMDATA, current_sel, 0);
    if (current_data == data) {
        return current_sel;
    }
    // コンボボックスアイテム数
    int num = SendMessage(hwnd, CB_GETCOUNT, 0, 0);

    int sel_idx = 0;
    for (int i = 0; i < num; i++) {
        if (data == SendMessage(hwnd, CB_GETITEMDATA, i, 0)) {
            sel_idx = i;
            break;
        }
    }
    SendMessage(hwnd, CB_SETCURSEL, sel_idx, 0); // カーソルセット
    current_data = SendMessage(hwnd, CB_GETITEMDATA, sel_idx, 0);
    set_cl_exdata(hwnd, current_data);
    return sel_idx;
}

static int set_combo_item(HWND hwnd, const char *string, int data) {
    // コンボボックスアイテム数
    int num = SendMessage(hwnd, CB_GETCOUNT, 0, 0);

    // 最後尾に追加
    SendMessage(hwnd, CB_INSERTSTRING, num, (LPARAM)string);
    SendMessage(hwnd, CB_SETITEMDATA, num, (LPARAM)data);

    return num;
}

static void save_combo_item_ini(FILTER *fp) {
    resize_res.clear();

    char key[256] = { 0 };
    char ini_str[256] = { 0 };
    int num = SendMessage(cx_resize_res, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < num; i++) {
        sprintf_s(key, "clfilter_resize_%d", i+1);
        memset(ini_str, 0, sizeof(ini_str));
        SendMessage(cx_resize_res, CB_GETLBTEXT, i, (LPARAM)ini_str);
        if (!fp->exfunc->ini_save_str(fp, key, ini_str)) {
            break;
        }
        int w = 0, h = 0;
        if (2 != sscanf_s(ini_str, "%dx%d", &w, &h)) {
            break;
        }
        resize_res.push_back(std::make_pair(w, h));
    }
    sprintf_s(key, "clfilter_resize_%d", num+1);
    char buf[4];
    strcpy_s(buf, "");
    fp->exfunc->ini_save_str(fp, key, buf);
}

static void del_combo_item(FILTER *fp, HWND hwnd, void *string) {
    const int current_sel = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    const int current_data = SendMessage(hwnd, CB_GETITEMDATA, current_sel, 0);

    int num = SendMessage(hwnd, CB_FINDSTRING, (WPARAM)-1, (WPARAM)string);
    if (num >= 0) {
        SendMessage(hwnd, CB_DELETESTRING, num, 0);
        select_combo_item(hwnd, current_data);
        save_combo_item_ini(fp);
    }
}

static void del_combo_item_current(FILTER *fp, HWND hwnd) {
    const int current_sel = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    const int current_data = SendMessage(hwnd, CB_GETITEMDATA, current_sel, 0);

    if (current_sel >= 0) {
        SendMessage(hwnd, CB_DELETESTRING, current_sel, 0);
        select_combo_item(hwnd, current_data);
        save_combo_item_ini(fp);
    }
}

static BOOL out_opencl_info(FILTER *fp) {
    char filename[4096] = { 0 };
    if (fp->exfunc->dlg_get_save_name(filename, (LPSTR)".txt", (LPSTR)"clinfo.txt")) {
        auto proc = createRGYPipeProcess();
        proc->init(PIPE_MODE_DISABLE, PIPE_MODE_ENABLE, PIPE_MODE_DISABLE);
        const std::vector<tstring> args =  { getClfiltersExePath(), _T("--clinfo") };
        if (proc->run(args, nullptr, 0, true, true) == 0) {
            auto str = proc->getOutput();
            if (str.length() > 0) {
                if (FILE *fpcl = nullptr; _tfopen_s(&fpcl, char_to_tstring(filename).c_str(), _T("w")) == 0 && fpcl) {
                    auto fpclinfo = std::unique_ptr<FILE, fp_deleter>(fpcl, fp_deleter());
                    fprintf(fpclinfo.get(), "%s", str.c_str());
                }
            }
        }
        proc->close();
    }
    return FALSE;
}

static void update_cx_resize_res_items(FILTER *fp) {
    //クリア
    SendMessage(cx_resize_res, CB_RESETCONTENT, 0, 0);
    resize_res.clear();

    char key[256] = { 0 };
    char ini_def[4] = { 0 };
    char ini_load[256] = { 0 };
    for (int i = 0;; i++) {
        sprintf_s(key, "clfilter_resize_%d", i+1);
        memset(ini_def, 0, sizeof(ini_def));
        if (!fp->exfunc->ini_load_str(fp, key, ini_load, ini_def)) {
            break;
        }
        int w = 0, h = 0;
        if (2 != sscanf_s(ini_load, "%dx%d", &w, &h)) {
            break;
        }
        set_combo_item(cx_resize_res, ini_load, i);
        resize_res.push_back(std::make_pair(w, h));
    }
    select_combo_item(cx_resize_res, cl_exdata.resize_idx);
}

void add_cx_resize_res_items(FILTER *fp) {
    EnableWindow(bt_resize_res_add, FALSE); // 追加ボタン無効化
    EnableWindow(bt_resize_res_del, FALSE); // 削除ボタン無効化

    char str[1024] = { 0 };
    int ret = DialogBoxParam(fp->dll_hinst, "ADD_RES_DLG", GetWindow(fp->hwnd, GW_OWNER), add_res_dlg, (LPARAM)str);
    if (ret > 0) {
        int num = SendMessage(cx_resize_res, CB_GETCOUNT, 0, 0);
        int w = ret >> 16;
        int h = ret & 0xffff;
        sprintf_s(str, "%dx%d", w, h);
        set_combo_item(cx_resize_res, str, num);
        save_combo_item_ini(fp);
        select_combo_item(cx_resize_res, num);
    }

    EnableWindow(bt_resize_res_add, TRUE); // 追加ボタン有効化
    EnableWindow(bt_resize_res_del, TRUE); // 削除ボタン有効化
}

static void set_resize_algo_items(const bool isCUDADevice) {
    int ret = 0;
    // 選択番号取得
    ret = SendMessage(cx_resize_algo, CB_GETCURSEL, 0, 0);
    ret = SendMessage(cx_resize_algo, CB_GETITEMDATA, ret, 0);

    SendMessage(cx_resize_algo, CB_RESETCONTENT, 0, 0);
    set_combo_item(cx_resize_algo, "spline16", RGY_VPP_RESIZE_SPLINE16);
    set_combo_item(cx_resize_algo, "spline36", RGY_VPP_RESIZE_SPLINE36);
    set_combo_item(cx_resize_algo, "spline64", RGY_VPP_RESIZE_SPLINE64);
    set_combo_item(cx_resize_algo, "lanczos2", RGY_VPP_RESIZE_LANCZOS2);
    set_combo_item(cx_resize_algo, "lanczos3", RGY_VPP_RESIZE_LANCZOS3);
    set_combo_item(cx_resize_algo, "lanczos4", RGY_VPP_RESIZE_LANCZOS4);
    set_combo_item(cx_resize_algo, "bilinear", RGY_VPP_RESIZE_BILINEAR);
    set_combo_item(cx_resize_algo, "bicubic",  RGY_VPP_RESIZE_BICUBIC);
    if (isCUDADevice) {
        set_combo_item(cx_resize_algo, "nvvfx-superres", RGY_VPP_RESIZE_NVVFX_SUPER_RES);
        set_combo_item(cx_resize_algo, "ngx-vsr", RGY_VPP_RESIZE_NGX_VSR);
    }
    if (ret != CB_ERR) {
        select_combo_item(cx_resize_algo, ret);
    }
}

void set_track_bar_enable(int track_bar, bool enable) {
    for (int j = 0; j < 5; j++) {
        EnableWindow(child_hwnd[track_bar * 5 + j + 1], enable);
    }
}
void set_track_bar_ex_enable(const CLFILTER_TRACKBAR& tb, bool enable) {
    EnableWindow(tb.label, enable);
    EnableWindow(tb.trackbar, enable);
    EnableWindow(tb.bt_left, enable);
    EnableWindow(tb.bt_right, enable);
    EnableWindow(tb.bt_text, enable);
}
void set_check_box_enable(int check_box, bool enable) {
    const int checkbox_idx = 1 + 5 * CLFILTER_TRACK_MAX;
    EnableWindow(child_hwnd[checkbox_idx + check_box], enable);
}
void set_track_bar_show_hide(int track_bar, bool show) {
    for (int j = 0; j < 5; j++) {
        ShowWindow(child_hwnd[track_bar * 5 + j + 1], show ? SW_SHOW : SW_HIDE);
    }
}
void set_check_box_show_hide(int check_box, bool show) {
    const int checkbox_idx = 1 + 5 * CLFILTER_TRACK_MAX;
    ShowWindow(child_hwnd[checkbox_idx + check_box], show ? SW_SHOW : SW_HIDE);
}

void update_cx(FILTER *fp) {
    if (cl_exdata.nnedi_nns == 0) {
        cl_exdata_set_default();
    }
    select_combo_item(cx_opencl_device,               cl_exdata.cl_dev_id.i);
    select_combo_item(cx_log_level,                   cl_exdata.log_level);
    select_combo_item(cx_resize_res,                  cl_exdata.resize_idx);
    select_combo_item(cx_resize_algo,                 cl_exdata.resize_algo);
    select_combo_item(cx_resize_ngx_vsr_quality,      cl_exdata.resize_ngx_vsr_quality);
    select_combo_item(cx_colorspace_colormatrix_from, cl_exdata.csp_from.matrix);
    select_combo_item(cx_colorspace_colormatrix_to,   cl_exdata.csp_to.matrix);
    select_combo_item(cx_colorspace_colorprim_from,   cl_exdata.csp_from.colorprim);
    select_combo_item(cx_colorspace_colorprim_to,     cl_exdata.csp_to.colorprim);
    select_combo_item(cx_colorspace_transfer_from,    cl_exdata.csp_from.transfer);
    select_combo_item(cx_colorspace_transfer_to,      cl_exdata.csp_to.transfer);
    select_combo_item(cx_colorspace_colorrange_from,  cl_exdata.csp_from.colorrange);
    select_combo_item(cx_colorspace_colorrange_to,    cl_exdata.csp_to.colorrange);
    select_combo_item(cx_colorspace_hdr2sdr,          cl_exdata.hdr2sdr);
    select_combo_item(cx_nnedi_field,                 cl_exdata.nnedi_field);
    select_combo_item(cx_nnedi_nns,                   cl_exdata.nnedi_nns);
    select_combo_item(cx_nnedi_nsize,                 cl_exdata.nnedi_nsize);
    select_combo_item(cx_nnedi_quality,               cl_exdata.nnedi_quality);
    select_combo_item(cx_nnedi_prescreen,             cl_exdata.nnedi_prescreen);
    select_combo_item(cx_nnedi_errortype,             cl_exdata.nnedi_errortype);
    select_combo_item(cx_nvvfx_denoise_strength,      cl_exdata.nvvfx_denoise_strength);
    select_combo_item(cx_nvvfx_artifact_reduction_mode, cl_exdata.nvvfx_artifact_reduction_mode);
    select_combo_item(cx_nvvfx_superres_mode,         cl_exdata.nvvfx_superres_mode);
    select_combo_item(cx_knn_radius,                  cl_exdata.knn_radius);
    select_combo_item(cx_nlmeans_patch,               cl_exdata.nlmeans_patch);
    select_combo_item(cx_nlmeans_search,              cl_exdata.nlmeans_search);
    select_combo_item(cx_denoise_dct_step,            cl_exdata.denoise_dct_step);
    select_combo_item(cx_denoise_dct_block_size,      cl_exdata.denoise_dct_block_size);
    select_combo_item(cx_smooth_quality,              cl_exdata.smooth_quality);
    select_combo_item(cx_unsharp_radius,              cl_exdata.unsharp_radius);
    select_combo_item(cx_warpsharp_blur,              cl_exdata.warpsharp_blur);
    select_combo_item(cx_deband_sample,               cl_exdata.deband_sample);
}

static void init_filter_order_list(FILTER *fp) {
    const int n = SendMessage(ls_filter_order, LB_GETCOUNT, 0, 0);
    if (n > 0) return; // もう初期化済みなら初期化しない

    SendMessage(ls_filter_order, LB_RESETCONTENT, 0, 0);

    // フィルタリストが何件あるかを確認
    size_t current_count = 0;
    for (size_t i = 0; i < std::min(filterList.size(), _countof(cl_exdata.filterOrder)); i++, current_count++) {
        if (cl_exdata.filterOrder[i] == VppType::VPP_NONE) {
            current_count = i;
            break;
        }
    }
    bool setOK = false;
    if (current_count == filterList.size()) {
        // cl_exdata.filterOrder の中に登録されているものが適切な値かチェック
        setOK = true;
        for (size_t ifilter = 0; ifilter < filterList.size(); ifilter++) {
            const auto filterFound = std::find_if(filterList.begin(), filterList.end(), [filter_type = cl_exdata.filterOrder[ifilter]](const std::pair<const TCHAR*, VppType>& obj) {
                return obj.second == filter_type;
                });
            if (filterFound == filterList.end()) {
                setOK = false;
                break;
            }
        }
    }
    if (setOK) {
        // 適切な値の場合はそのまま
        for (size_t ifilter = 0; ifilter < filterList.size(); ifilter++) {
            const auto filterFound = std::find_if(filterList.begin(), filterList.end(), [filter_type = cl_exdata.filterOrder[ifilter]](const std::pair<const TCHAR*, VppType>& obj) {
                return obj.second == filter_type;
                });
            SendMessage(ls_filter_order, LB_INSERTSTRING, ifilter, (LPARAM)filterFound->first);
            SendMessage(ls_filter_order, LB_SETITEMDATA, ifilter, (LPARAM)filterFound->second);
        }
    } else {
        // 完全初期化
        for (size_t ifilter = 0; ifilter < filterList.size(); ifilter++) {
            SendMessage(ls_filter_order, LB_INSERTSTRING, ifilter, (LPARAM)filterList[ifilter].first);
            SendMessage(ls_filter_order, LB_SETITEMDATA, ifilter, (LPARAM)filterList[ifilter].second);
            cl_exdata.filterOrder[ifilter] = filterList[ifilter].second;
        }
    }

}

// CUDA関連のオプションの有効/無効の切り替え
static void update_cuda_enable(FILTER *fp) {
    const auto dev = g_clfiltersAufDevices->findDevice(cl_exdata.cl_dev_id.s.platform, cl_exdata.cl_dev_id.s.device);
    const int cudaNvvfxSupport = dev && dev->pd.s.platform == CLCU_PLATFORM_CUDA && dev->cudaVer.first >= 7 ? 1 : 0; // nvvfxはCC7.0(Turing)以上が必要
    if (g_cuda_device_nvvfx_support != cudaNvvfxSupport) {
        // CUDAデバイスかどうかが変更された
        EnableWindow(bt_opencl_info, cudaNvvfxSupport ? FALSE : TRUE);
        EnableWindow(cx_nvvfx_denoise_strength, cudaNvvfxSupport);
        EnableWindow(lb_nvvfx_denoise_strength, cudaNvvfxSupport);
        EnableWindow(cx_nvvfx_artifact_reduction_mode, cudaNvvfxSupport);
        EnableWindow(lb_nvvfx_artifact_reduction_mode, cudaNvvfxSupport);
        set_check_box_enable(CLFILTER_CHECK_NVVFX_DENOISE_ENABLE, cudaNvvfxSupport);
        set_check_box_enable(CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE, cudaNvvfxSupport);
        set_resize_algo_items(cudaNvvfxSupport);
        const int checkbox_idx = 1 + 5 * CLFILTER_TRACK_MAX;
        if (!cudaNvvfxSupport) {
            fp->check[CLFILTER_CHECK_NVVFX_DENOISE_ENABLE] = FALSE;
            SendMessage(child_hwnd[checkbox_idx + CLFILTER_CHECK_NVVFX_DENOISE_ENABLE], BM_SETCHECK, BST_UNCHECKED, 0);

            fp->check[CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE] = FALSE;
            SendMessage(child_hwnd[checkbox_idx + CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE], BM_SETCHECK, BST_UNCHECKED, 0);
        }

        set_check_box_enable(CLFILTER_CHECK_TRUEHDR_ENABLE, cudaNvvfxSupport);
        set_track_bar_ex_enable(tb_ngx_truehdr_contrast, cudaNvvfxSupport);
        set_track_bar_ex_enable(tb_ngx_truehdr_saturation, cudaNvvfxSupport);
        set_track_bar_ex_enable(tb_ngx_truehdr_middlegray, cudaNvvfxSupport);
        set_track_bar_ex_enable(tb_ngx_truehdr_maxluminance, cudaNvvfxSupport);
        if (!cudaNvvfxSupport) {
            fp->check[CLFILTER_CHECK_TRUEHDR_ENABLE] = FALSE;
            SendMessage(child_hwnd[checkbox_idx + CLFILTER_CHECK_TRUEHDR_ENABLE], BM_SETCHECK, BST_UNCHECKED, 0);
        }

        g_cuda_device_nvvfx_support = cudaNvvfxSupport;
    }
}

// nvvfx supreres関連の表示/非表示切り替え
static void update_nvvfx_superres(FILTER *fp) {
    const int resize_nvvfx_superres = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo == RGY_VPP_RESIZE_NVVFX_SUPER_RES ? 1 : 0;
    const int resize_ngx_vsr_quality = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo == RGY_VPP_RESIZE_NGX_VSR ? 1 : 0;
    if (g_resize_nvvfx_superres != resize_nvvfx_superres) {
        set_track_bar_show_hide(CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH, resize_nvvfx_superres);
        ShowWindow(lb_nvvfx_superres_mode, resize_nvvfx_superres ? SW_SHOW : SW_HIDE);
        ShowWindow(cx_nvvfx_superres_mode, resize_nvvfx_superres ? SW_SHOW : SW_HIDE);
        g_resize_nvvfx_superres = resize_nvvfx_superres;
    }
    if (g_resize_ngx_vsr_quality != resize_ngx_vsr_quality) {
        ShowWindow(lb_resize_ngx_vsr_quality, resize_ngx_vsr_quality ? SW_SHOW : SW_HIDE);
        ShowWindow(cx_resize_ngx_vsr_quality, resize_ngx_vsr_quality ? SW_SHOW : SW_HIDE);
        g_resize_ngx_vsr_quality = resize_ngx_vsr_quality;
    }
}

BOOL proc_trackbar_ex(const CLFILTER_TRACKBAR_DATA *trackbar, const int ctrlID, WPARAM wparam, LPARAM lparam) {
    if (ctrlID - trackbar->label_id == 2) { // 左ボタンの処理
        // トラックバーの現在の値を取得
        int pos = SendMessage(trackbar->tb->trackbar, TBM_GETPOS, 0, 0);
        // 値を1減らす
        pos = std::max(pos - 1, trackbar->val_min);
        // トラックバーの値を設定
        SendMessage(trackbar->tb->trackbar, TBM_SETPOS, TRUE, pos);
        // テキストボックスに値を設定
        SetWindowText(trackbar->tb->bt_text, strsprintf("%d", pos).c_str());
        *(trackbar->ex_data_pos) = pos;
        return TRUE; //TRUEを返すと画像処理が更新される
    } else if (ctrlID - trackbar->label_id == 3) { // 右ボタンの処理
        // トラックバーの現在の値を取得
        int pos = SendMessage(trackbar->tb->trackbar, TBM_GETPOS, 0, 0);
        // 値を1増やす
        pos = std::min(pos + 1, trackbar->val_max);
        // トラックバーの値を設定
        SendMessage(trackbar->tb->trackbar, TBM_SETPOS, TRUE, pos);
        // テキストボックスに値を設定
        SetWindowText(trackbar->tb->bt_text, strsprintf("%d", pos).c_str());
        *(trackbar->ex_data_pos) = pos;
        return TRUE; //TRUEを返すと画像処理が更新される
    }  else if (ctrlID - trackbar->label_id == 4) { // テキストボックスの処理
        if (HIWORD(wparam) == EN_CHANGE) {
            // 現在のカーソル位置を取得
            DWORD start, end;
            SendMessage(trackbar->tb->bt_text, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
            // テキストボックスの内容が変更されたとき
            char buffer[256] = { 0 };
            int value = 0;
            GetWindowText(trackbar->tb->bt_text, buffer, sizeof(buffer));
            if (sscanf_s(buffer, "%d", &value) == 1) {
                const int pos = clamp(value, trackbar->val_min, trackbar->val_max);
                SendMessage(trackbar->tb->trackbar, TBM_SETPOS, TRUE, pos);
                *(trackbar->ex_data_pos) = pos;
            }
            // カーソル位置を復元
            SendMessage(trackbar->tb->bt_text, EM_SETSEL, start, end);
            return TRUE; //TRUEを返すと画像処理が更新される
        } else if (HIWORD(wparam) == EN_KILLFOCUS) {
            // 現在のカーソル位置を取得
            DWORD start, end;
            SendMessage(trackbar->tb->bt_text, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
            char buffer[256] = { 0 };
            int org_value = 0;
            if (strlen(buffer) == 0) {
                // 空白の場合、元の値に戻す
                const int pos = SendMessage(trackbar->tb->trackbar, TBM_GETPOS, 0, 0);
                SetWindowText(trackbar->tb->bt_text, strsprintf("%d", pos).c_str());
            } else if (sscanf_s(buffer, "%d", &org_value) == 1) {
                const int pos = clamp(org_value, trackbar->val_min, trackbar->val_max);
                SendMessage(trackbar->tb->trackbar, TBM_SETPOS, TRUE, pos);
                if (pos != org_value) {
                    SetWindowText(trackbar->tb->bt_text, strsprintf("%d", pos).c_str());
                }
                *(trackbar->ex_data_pos) = pos;
            }
            // カーソル位置を復元
            SendMessage(trackbar->tb->bt_text, EM_SETSEL, start, end);
            return TRUE; //TRUEを返すと画像処理が更新される
        }
    }
    return FALSE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void*, FILTER *fp) {
    switch (message) {
    case WM_FILTER_FILE_OPEN:
    case WM_FILTER_FILE_CLOSE:
        break;
    case WM_FILTER_INIT:
        cl_exdata_set_default();
        init_dialog(hwnd, fp);
        g_hbrBackground = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
        return TRUE;
    case WM_FILTER_UPDATE: // フィルタ更新
    case WM_FILTER_SAVE_END: // セーブ終了
        update_cx(fp);
        init_filter_order_list(fp);
        update_cuda_enable(fp);
        update_nvvfx_superres(fp);
        break;
    case WM_NOTIFY: {
        if (auto trackbar = get_trackbar_data(wparam); trackbar != nullptr) {
            // トラックバーをドラッグしている最中の処理
            if (((const NMHDR *)lparam)->code == NM_CUSTOMDRAW) {
                // トラックバーの現在の値を取得
                int pos = SendMessage(trackbar->tb->trackbar, TBM_GETPOS, 0, 0);
                // テキストボックスに値を設定 (テキストボックスが最小値-最大値の範囲内の場合のみ更新する)
                char buffer[256] = { 0 };
                GetWindowText(trackbar->tb->bt_text, buffer, sizeof(buffer));
                int value = 0;
                if (sscanf_s(buffer, "%d", &value) == 1
                    && trackbar->val_min <= value && value <= trackbar->val_max) {
                    SetWindowText(trackbar->tb->bt_text, strsprintf("%d", pos).c_str());
                }
            }
        }
        break;
    }
    case WM_HSCROLL: {
        if (auto trackbar = get_trackbar_data_from_handle((HWND)lparam); trackbar != nullptr) {
            // トラックバーをドラッグし終わったときの処理
            HWND hwndTrackbar = (HWND)lparam;
            // トラックバーの現在の値を取得
            int pos = SendMessage(hwndTrackbar, TBM_GETPOS, 0, 0);
            // テキストボックスに値を設定 (テキストボックスが最小値-最大値の範囲内の場合のみ更新する)
            char buffer[256] = { 0 };
            GetWindowText(trackbar->tb->bt_text, buffer, sizeof(buffer));
            int value = 0;
            if (sscanf_s(buffer, "%d", &value) == 1
                && trackbar->val_min <= value && value <= trackbar->val_max) {
                SetWindowText(trackbar->tb->bt_text, strsprintf("%d", pos).c_str());
            }
            *(trackbar->ex_data_pos) = pos;
        }
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case ID_CX_OPENCL_DEVICE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_opencl_device);
                EnableWindow(bt_opencl_info, (cl_exdata.cl_dev_id.s.platform == CLCU_PLATFORM_CUDA) ? FALSE : TRUE);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_LOG_LEVEL: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_log_level);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_RESIZE_RES: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_resize_res);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_RESIZE_ALGO: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_resize_algo);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_RESIZE_NGX_VSR_QUALITY: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_resize_ngx_vsr_quality);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_BT_RESIZE_RES_ADD:
            add_cx_resize_res_items(fp);
            break;
        case ID_BT_RESIZE_RES_DEL:
            del_combo_item_current(fp, cx_resize_res);
            break;
        case ID_BT_OPENCL_INFO:
            out_opencl_info(fp);
            break;
        case ID_LS_FILTER_ORDER:
            set_filter_order();
            break;
        case ID_BT_FILTER_ORDER_UP:
            lstbox_move_up(ls_filter_order);
            set_filter_order();
            return TRUE; //TRUEを返すと画像処理が更新される
        case ID_BT_FILTER_ORDER_DOWN:
            lstbox_move_down(ls_filter_order);
            return TRUE; //TRUEを返すと画像処理が更新される
            break;
        case ID_CX_COLORSPACE_COLORMATRIX_FROM: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_colormatrix_from);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_COLORMATRIX_TO: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_colormatrix_to);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_COLORPRIM_FROM: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_colorprim_from);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_COLORPRIM_TO: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_colorprim_to);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_TRANSFER_FROM: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_transfer_from);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_TRANSFER_TO: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_transfer_to);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_COLORRANGE_FROM: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_colorrange_from);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_COLORRANGE_TO: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_colorrange_to);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_COLORSPACE_HDR2SDR: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_colorspace_hdr2sdr);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NNEDI_FIELD: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nnedi_field);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NNEDI_NSIZE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nnedi_nsize);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NNEDI_NNS: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nnedi_nns);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NNEDI_QUALITY: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nnedi_quality);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NNEDI_PRESCREEN: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nnedi_prescreen);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NNEDI_ERRORTYPE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nnedi_errortype);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NVVFX_DENOISE_STRENGTH: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nvvfx_denoise_strength);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NVVFX_ARTIFACT_REDUCTION_MODE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nvvfx_artifact_reduction_mode);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_NVVFX_SUPRERES_MODE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nvvfx_superres_mode);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_DENOISE_DCT_STEP: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_denoise_dct_step);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_DENOISE_DCT_BLOCK_SIZE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_denoise_dct_block_size);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_SMOOTH_QUALITY: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_smooth_quality);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_KNN_RADIUS: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_knn_radius);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_DENOISE_NLMEANS_PATCH: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nlmeans_patch);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_DENOISE_NLMEANS_SEARCH: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_nlmeans_search);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_UNSHARP_RADIUS: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_unsharp_radius);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_WARPSHARP_BLUR: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_warpsharp_blur);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        case ID_CX_DEBAND_SAMPLE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_deband_sample);
                return TRUE; //TRUEを返すと画像処理が更新される
            default:
                break;
            }
            break;
        default: {
            const auto ctrlID = LOWORD(wparam);
            // 追加したトラックバーに関する処理
            if (const auto trackbar = get_trackbar_data(ctrlID); trackbar != nullptr) {
                if (proc_trackbar_ex(trackbar, ctrlID, wparam, lparam)) {
                    return TRUE;
                }
            }
            break;
        }
        }
        break;
    // テキストボックスの背景色を変更したいが、WM_CTLCOLOREDITが送られてこないため断念
    // case WM_CTLCOLOREDIT:
    //     if (auto trackbar = get_trackbar_data_from_handle((HWND)lparam); trackbar != nullptr) {
    //         if (trackbar->tb->bt_text == (HWND)lparam) { // テキストボックスの処理
    //             SetBkColor((HDC)wparam, GetSysColor(COLOR_WINDOW)); // 背景色をメインウィンドウの背景色に設定
    //             return (LRESULT)g_hbrBackground;
    //         }
    //     }
    //     break;
    case WM_FILTER_EXIT:
        break;
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_MOUSEWHEEL:
        SendMessage(GetWindow(hwnd, GW_OWNER), message, wparam, lparam);
        break;
    //case WM_SETFOCUS:
    //    if (const auto trackbar = get_trackbar_data_from_handle((HWND)wparam); trackbar != nullptr) {
    //        if (trackbar->tb->bt_text == (HWND)wparam) {
    //            SendMessage(trackbar->tb->bt_text, EM_SETSEL, 0, -1);
    //        }
    //    }
    //    break;
    default:
        return FALSE;
    }

    return FALSE;
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    child_hwnd.push_back(hwnd);
    return TRUE;
}

// COMBOBOX の追加位置
enum AddCXMode {
    ADD_CX_FIRST,       // 有効無効のボックスのすぐあと
    ADD_CX_AFTER_TRACK, // trackbarのあと
    ADD_CX_AFTER_CHECK, // checkboxのあと
};

static const int CX_HEIGHT = 24;
static const int AVIUTL_1_10_OFFSET = 5;

void move_track_bar(int& y_pos, int col, int col_width, int track_min, int track_max, int track_bar_delta_y, const RECT& dialog_rc) {
    for (int i = track_min; i < track_max; i++, y_pos += track_bar_delta_y) {
        for (int j = 0; j < 5; j++) {
            RECT rc;
            GetWindowRect(child_hwnd[i * 5 + j + 1], &rc);
            SetWindowPos(child_hwnd[i * 5 + j + 1], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }
}

void move_group(int& y_pos, int col, int col_width, int check_min, int check_max, int track_min, int track_max, const int track_bar_delta_y,
                const AddCXMode add_cx_mode, const int add_cx_num, int& cx_y_pos, const int checkbox_idx, const RECT& dialog_rc) {
    RECT rc;
    // 有効無効のボックス
    GetWindowRect(child_hwnd[checkbox_idx + check_min], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + check_min], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    y_pos += track_bar_delta_y;

    if (add_cx_num > 0 && add_cx_mode == ADD_CX_FIRST) {
        cx_y_pos = y_pos + 2;                // すこし窮屈なので +2pix
        y_pos += CX_HEIGHT * add_cx_num + 4; // すこし窮屈なので +4pix
    }

    // trackbar
    move_track_bar(y_pos, col, col_width, track_min, track_max, track_bar_delta_y, dialog_rc);

    if (add_cx_num > 0 && add_cx_mode == ADD_CX_AFTER_TRACK) {
        cx_y_pos = y_pos + 2;                // すこし窮屈なので +2pix
        y_pos += CX_HEIGHT * add_cx_num + 4; // すこし窮屈なので +4pix
    }

    // checkbox
    for (int i = check_min+1; i < check_max; i++, y_pos += track_bar_delta_y) {
        GetWindowRect(child_hwnd[checkbox_idx+i], &rc);
        SetWindowPos(child_hwnd[checkbox_idx+i], HWND_TOP, rc.left - dialog_rc.left + 10 + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }

    if (add_cx_num > 0 && add_cx_mode == ADD_CX_AFTER_CHECK) {
        cx_y_pos = y_pos + 2;                // すこし窮屈なので +2pix
        y_pos += CX_HEIGHT * add_cx_num + 4; // すこし窮屈なので +4pix
    }

    y_pos += track_bar_delta_y;
}

void set_combobox_items(HWND hwnd_cx, const CX_DESC *cx_items, int limit = INT_MAX) {
    for (int i = 0; cx_items[i].desc && i < limit; i++) {
        set_combo_item(hwnd_cx, (char *)cx_items[i].desc, cx_items[i].value);
    }
}

void add_combobox(HWND& hwnd_cx, int id_cx, HWND& hwnd_lb, int id_lb, const char *lb_str, int col, int col_width, int& y_pos, HFONT b_font, HWND hwnd, HINSTANCE hinst, const CX_DESC *cx_items, int cx_item_limit = INT_MAX) {
    hwnd_lb = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, 10 + col * col_width + AVIUTL_1_10_OFFSET, y_pos, 58, 24, hwnd, (HMENU)id_lb, hinst, NULL);
    SendMessage(hwnd_lb, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(hwnd_lb, WM_SETTEXT, 0, (LPARAM)lb_str);
    hwnd_cx = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 68 + col * col_width + AVIUTL_1_10_OFFSET, y_pos, 145, 100, hwnd, (HMENU)id_cx, hinst, NULL);
    SendMessage(hwnd_cx, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx, cx_items, cx_item_limit);
    y_pos += CX_HEIGHT;
}

void add_combobox_from_to(
    HWND& hwnd_cx_from, int id_cx_from, HWND& hwnd_cx_to, int id_cx_to,
    HWND& hwnd_lb_from_to, int id_lb_from_to,
    const char *lb_str, int col, int col_width, int& y_pos, HFONT b_font, HWND hwnd, HINSTANCE hinst, const CX_DESC *cx_items, int cx_item_limit = INT_MAX) {
    // from
    hwnd_cx_from = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 98 + col * col_width + AVIUTL_1_10_OFFSET, y_pos, 90, 100, hwnd, (HMENU)id_cx_from, hinst, NULL);
    SendMessage(hwnd_cx_from, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx_from, cx_items, cx_item_limit);
    // label (from->to)
    hwnd_lb_from_to = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, 192 + col * col_width + AVIUTL_1_10_OFFSET, y_pos, 10, 24, hwnd, (HMENU)id_lb_from_to, hinst, NULL);
    SendMessage(hwnd_lb_from_to, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(hwnd_lb_from_to, WM_SETTEXT, 0, (LPARAM)"→");
    // to
    hwnd_cx_to = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 208 + col * col_width + AVIUTL_1_10_OFFSET, y_pos, 90, 100, hwnd, (HMENU)id_cx_to, hinst, NULL);
    SendMessage(hwnd_cx_to, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx_to, cx_items, cx_item_limit);
    y_pos += CX_HEIGHT;
}

void move_colorspace(int& y_pos, int col, int col_width, int check_min, int check_max, int track_min, int track_max, const int track_bar_delta_y, const int checkbox_idx, const RECT& dialog_rc, HFONT b_font, HWND hwnd, HINSTANCE hinst) {
    RECT rc;
    GetWindowRect(child_hwnd[checkbox_idx + check_min], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + check_min], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    y_pos += track_bar_delta_y;
    int cx_y_pos = y_pos;
    for (int i = check_min + 1; i < check_max; i++, y_pos += track_bar_delta_y) {
        GetWindowRect(child_hwnd[checkbox_idx + i], &rc);
        SetWindowPos(child_hwnd[checkbox_idx + i], HWND_TOP, rc.left - dialog_rc.left + 10 + col * col_width, y_pos, 70, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOZORDER);
    }

    add_combobox_from_to(cx_colorspace_colormatrix_from, ID_CX_COLORSPACE_COLORMATRIX_FROM, cx_colorspace_colormatrix_to, ID_CX_COLORSPACE_COLORMATRIX_TO,
                         lb_colorspace_colormatrix_from_to, ID_LB_COLORSPACE_COLORMATRIX_FROM_TO,
                         "matrix", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_colormatrix+3);
    add_combobox_from_to(cx_colorspace_colorprim_from, ID_CX_COLORSPACE_COLORPRIM_FROM, cx_colorspace_colorprim_to, ID_CX_COLORSPACE_COLORPRIM_TO,
                         lb_colorspace_colorprim_from_to, ID_LB_COLORSPACE_COLORPRIM_FROM_TO,
                         "prim", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_colorprim+4);
    add_combobox_from_to(cx_colorspace_transfer_from, ID_CX_COLORSPACE_TRANSFER_FROM, cx_colorspace_transfer_to, ID_CX_COLORSPACE_TRANSFER_TO,
                         lb_colorspace_transfer_from_to, ID_LB_COLORSPACE_TRANSFER_FROM_TO,
                         "transfer", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_transfer+4);
    add_combobox_from_to(cx_colorspace_colorrange_from, ID_CX_COLORSPACE_COLORRANGE_FROM, cx_colorspace_colorrange_to, ID_CX_COLORSPACE_COLORRANGE_TO,
                         lb_colorspace_colorrange_from_to, ID_LB_COLORSPACE_COLORRANGE_FROM_TO,
                         "range", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_colorrange+2, 2);
    add_combobox(cx_colorspace_hdr2sdr, ID_CX_COLORSPACE_HDR2SDR, lb_colorspace_hdr2sdr, ID_LB_COLORSPACE_HDR2SDR, "hdr2sdr", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_hdr2sdr);

    y_pos = std::max(y_pos, cx_y_pos);

    for (int i = track_min; i < track_max; i++, y_pos += track_bar_delta_y) {
        for (int j = 0; j < 5; j++) {
            GetWindowRect(child_hwnd[i * 5 + j + 1], &rc);
            SetWindowPos(child_hwnd[i * 5 + j + 1], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    y_pos += track_bar_delta_y;
}

static void init_clfilter_exe(const FILTER *fp) {
    SYS_INFO sys_info = { 0 };
    fp->exfunc->get_sys_info(nullptr, &sys_info);
    g_clfiltersAuf = std::make_unique<clcuFiltersAuf>();
    g_clfiltersAuf->runProcess(fp->dll_hinst, sys_info.max_w, sys_info.max_h, platformIsCUDA(cl_exdata.cl_dev_id.s.platform));
}

void init_device_list() {
    if (!g_clfiltersAufDevices) {
        g_clfiltersAufDevices = std::make_unique<clcuFiltersAufDevices>();
        g_clfiltersAufDevices->createList();
    }
}

void init_dialog(HWND hwnd, FILTER *fp) {
    child_hwnd.clear();
    int nCount = 0;
    EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&nCount);

    RECT rc, dialog_rc;
    GetWindowRect(hwnd, &dialog_rc);

    const int columns = 4;
    const int col_width = dialog_rc.right - dialog_rc.left;

    //clfilterのチェックボックス
    GetWindowRect(child_hwnd[0], &rc);
    SetWindowPos(child_hwnd[0], HWND_TOP, rc.left - dialog_rc.left + (columns-1) * col_width - AVIUTL_1_10_OFFSET, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    //最初のtrackbar
    GetWindowRect(child_hwnd[1], &rc);
    //最初のtrackbarの高さ
    const int first_y = rc.top - dialog_rc.top;

    //次のtrackbar
    GetWindowRect(child_hwnd[1+5], &rc);
    //track bar間の高さを取得
    const int track_bar_delta_y = rc.top - dialog_rc.top - first_y;

    HINSTANCE hinst = fp->dll_hinst;
    HFONT b_font = CreateFont(14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, "Meiryo UI");

    //OpenCL platform/device
    const int cb_opencl_platform_y = 8;
    lb_opencl_device = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, 8, cb_opencl_platform_y, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_RES, hinst, NULL);
    SendMessage(lb_opencl_device, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_opencl_device, WM_SETTEXT, 0, (LPARAM)LB_CX_OPENCL_DEVICE);

    cx_opencl_device = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 68, cb_opencl_platform_y, 245, 100, hwnd, (HMENU)ID_CX_OPENCL_DEVICE, hinst, NULL);
    SendMessage(cx_opencl_device, WM_SETFONT, (WPARAM)b_font, 0);

    init_device_list();
    if (g_clfiltersAufDevices) {
        for (const auto& pd : g_clfiltersAufDevices->getPlatforms()) {
            std::string devFullName = pd.pd.s.platform == CLCU_PLATFORM_CUDA ? "CUDA: " : "OpenCL: ";
            devFullName += pd.devName;
            set_combo_item(cx_opencl_device, devFullName.c_str(), pd.pd.i);
        }
    }

    //OpenCL info
    bt_opencl_info = CreateWindow("BUTTON", "clinfo", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_VCENTER, 318, cb_opencl_platform_y, 48, 22, hwnd, (HMENU)ID_BT_OPENCL_INFO, hinst, NULL);
    SendMessage(bt_opencl_info, WM_SETFONT, (WPARAM)b_font, 0);

    //checkboxの移動
    const int checkbox_idx = 1+5*CLFILTER_TRACK_MAX;
#if ENABLE_FIELD
    //フィールド処理
    const int cb_field_y = cb_opencl_platform_y + 24;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_FIELD], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_FIELD], HWND_TOP, rc.left - dialog_rc.left, cb_filed_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
#else
    const int cb_field_y = cb_opencl_platform_y;
#endif //#if ENABLE_FIELD

    const int cb_row_start_y_pos = cb_field_y + 28;

    //リサイズ
    int col = 0;
    const int cb_resize_y = cb_row_start_y_pos;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], HWND_TOP, col * col_width + rc.left - dialog_rc.left, cb_resize_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    lb_proc_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, col * col_width + 8 + AVIUTL_1_10_OFFSET, cb_resize_y+24, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_RES, hinst, NULL);
    SendMessage(lb_proc_mode, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_proc_mode, WM_SETTEXT, 0, (LPARAM)LB_CX_RESIZE_SIZE);

    cx_resize_res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, col * col_width + 68, cb_resize_y+24, 145, 100, hwnd, (HMENU)ID_CX_RESIZE_RES, hinst, NULL);
    SendMessage(cx_resize_res, WM_SETFONT, (WPARAM)b_font, 0);

    bt_resize_res_add = CreateWindow("BUTTON", LB_BT_RESIZE_ADD, WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, col * col_width + 214, cb_resize_y+24, 32, 22, hwnd, (HMENU)ID_BT_RESIZE_RES_ADD, hinst, NULL);
    SendMessage(bt_resize_res_add, WM_SETFONT, (WPARAM)b_font, 0);

    bt_resize_res_del = CreateWindow("BUTTON", LB_BT_RESIZE_DELETE, WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, col * col_width + 246, cb_resize_y+24, 32, 22, hwnd, (HMENU)ID_BT_RESIZE_RES_DEL, hinst, NULL);
    SendMessage(bt_resize_res_del, WM_SETFONT, (WPARAM)b_font, 0);

    cx_resize_algo = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, col * col_width + 68, cb_resize_y+48, 145, 160, hwnd, (HMENU)ID_CX_RESIZE_ALGO, hinst, NULL);
    SendMessage(cx_resize_algo, WM_SETFONT, (WPARAM)b_font, 0);

    update_cx_resize_res_items(fp);
    set_resize_algo_items(cl_exdata.cl_dev_id.s.platform == CLCU_PLATFORM_CUDA);

    int y_pos_max = 0;
    // --- 最初の列 -----------------------------------------
    lb_nvvfx_superres_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, col * col_width + 8 + AVIUTL_1_10_OFFSET, cb_resize_y + 72, 60, 24, hwnd, (HMENU)ID_LB_NVVFX_SUPRERES_MODE, hinst, NULL);
    SendMessage(lb_nvvfx_superres_mode, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_nvvfx_superres_mode, WM_SETTEXT, 0, (LPARAM)LB_CX_NVVFX_SUPRERES_MODE);

    cx_nvvfx_superres_mode = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, col * col_width + 68, cb_resize_y + 72, 145, 100, hwnd, (HMENU)ID_CX_NVVFX_SUPRERES_MODE, hinst, NULL);
    SendMessage(cx_nvvfx_superres_mode, WM_SETFONT, (WPARAM)b_font, 0);
    set_combo_item(cx_nvvfx_superres_mode, "0 - light", 0);
    set_combo_item(cx_nvvfx_superres_mode, "1 - strong", 1);

    lb_resize_ngx_vsr_quality = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, col * col_width + 8 + AVIUTL_1_10_OFFSET, cb_resize_y + 72, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_NGX_VSR_QUALITY, hinst, NULL);
    SendMessage(lb_resize_ngx_vsr_quality, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_resize_ngx_vsr_quality, WM_SETTEXT, 0, (LPARAM)LB_CX_RESIZE_NGX_VSR_QUALITY);

    cx_resize_ngx_vsr_quality = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, col * col_width + 68, cb_resize_y + 72, 145, 160, hwnd, (HMENU)ID_CX_RESIZE_NGX_VSR_QUALITY, hinst, NULL);
    SendMessage(cx_resize_ngx_vsr_quality, WM_SETFONT, (WPARAM)b_font, 0);
    set_combo_item(cx_resize_ngx_vsr_quality, "1 - fast", 1);
    set_combo_item(cx_resize_ngx_vsr_quality, "2",        2);
    set_combo_item(cx_resize_ngx_vsr_quality, "3",        3);
    set_combo_item(cx_resize_ngx_vsr_quality, "4 - slow", 4);

    int y_pos = cb_resize_y + track_bar_delta_y * 4 + 8;
    int cx_y_pos = 0;

    y_pos -= track_bar_delta_y / 4;
    move_track_bar(y_pos, col, col_width, CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH, CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH+1, track_bar_delta_y, dialog_rc);
    y_pos += track_bar_delta_y / 4;

    //colorspace
    move_colorspace(y_pos, col, col_width, CLFILTER_CHECK_COLORSPACE_ENABLE, CLFILTER_CHECK_COLORSPACE_MAX, CLFILTER_TRACK_COLORSPACE_FIRST, CLFILTER_TRACK_COLORSPACE_MAX, track_bar_delta_y, checkbox_idx, dialog_rc, b_font, hwnd, hinst);

    //nnedi
    move_group(y_pos, col, col_width, CLFILTER_CHECK_NNEDI_ENABLE, CLFILTER_CHECK_NNEDI_MAX, CLFILTER_TRACK_NNEDI_FIRST, CLFILTER_TRACK_NNEDI_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);
    y_pos -= track_bar_delta_y / 2;
    add_combobox(cx_nnedi_field,     ID_CX_NNEDI_FIELD,     lb_nnedi_field,     ID_LB_NNEDI_FIELD,     LB_CX_NNEDI_FIELD,     col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_field+2, 2);
    add_combobox(cx_nnedi_nns,       ID_CX_NNEDI_NNS,       lb_nnedi_nns,       ID_LB_NNEDI_NNS,       LB_CX_NNEDI_NNS,       col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_nns);
    add_combobox(cx_nnedi_nsize,     ID_CX_NNEDI_NSIZE,     lb_nnedi_nsize,     ID_LB_NNEDI_NSIZE,     LB_CX_NNEDI_NSIZE,     col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_nsize);
    add_combobox(cx_nnedi_quality,   ID_CX_NNEDI_QUALITY,   lb_nnedi_quality,   ID_LB_NNEDI_QUALITY,   LB_CX_NNEDI_QUALITY,   col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_quality);
    add_combobox(cx_nnedi_prescreen, ID_CX_NNEDI_PRESCREEN, lb_nnedi_prescreen, ID_LB_NNEDI_PRESCREEN, LB_CX_NNEDI_PRESCREEN, col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_pre_screen);
    add_combobox(cx_nnedi_errortype, ID_CX_NNEDI_ERRORTYPE, lb_nnedi_errortype, ID_LB_NNEDI_ERRORTYPE, LB_CX_NNEDI_ERRORTYPE, col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_error_type);
    y_pos += track_bar_delta_y / 2;

    y_pos_max = std::max(y_pos_max, y_pos);
    // --- 次の列 -----------------------------------------
    col = 1;
    y_pos = cb_row_start_y_pos;

    //nvvfx-denoise
    const CX_DESC list_vpp_nvvfx_denoise_strength[] = {
        { _T("0 - light"),  0 },
        { _T("1 - strong"), 1 },
        { NULL, 0 }
    };
    move_group(y_pos, col, col_width, CLFILTER_CHECK_NVVFX_DENOISE_ENABLE, CLFILTER_CHECK_NVVFX_DENOISE_MAX, CLFILTER_TRACK_NVVFX_DENOISE_FIRST, CLFILTER_TRACK_NVVFX_DENOISE_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_nvvfx_denoise_strength, ID_CX_NVVFX_DENOISE_STRENGTH, lb_nvvfx_denoise_strength, ID_LB_NVVFX_DENOISE_STRENGTH, LB_CX_NVVFX_DENOISE_STRENGTH, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_nvvfx_denoise_strength);

    //nvvfx-artifact-reduction
    const CX_DESC list_vpp_nvvfx_artifact_reduction_mode[] = {
        { _T("0 - light"),  0 },
        { _T("1 - strong"), 1 },
        { NULL, 0 }
    };
    move_group(y_pos, col, col_width, CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE, CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_MAX, CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_FIRST, CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_nvvfx_artifact_reduction_mode, ID_CX_NVVFX_ARTIFACT_REDUCTION_MODE, lb_nvvfx_artifact_reduction_mode, ID_LB_NVVFX_ARTIFACT_REDUCTION_MODE, LB_CX_NVVFX_ARTIFACT_REDUCTION_MODE, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_nvvfx_artifact_reduction_mode);

    //denoise-dct
    move_group(y_pos, col, col_width, CLFILTER_CHECK_DENOISE_DCT_ENABLE, CLFILTER_CHECK_DENOISE_DCT_MAX, CLFILTER_TRACK_DENOISE_DCT_FIRST, CLFILTER_TRACK_DENOISE_DCT_MAX, track_bar_delta_y, ADD_CX_FIRST, 2, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_denoise_dct_step,       ID_CX_DENOISE_DCT_STEP,       lb_denoise_dct_step,       ID_LB_DENOISE_DCT_STEP,       LB_CX_DENOISE_DCT_STEP,       col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_denoise_dct_step);
    add_combobox(cx_denoise_dct_block_size, ID_CX_DENOISE_DCT_BLOCK_SIZE, lb_denoise_dct_block_size, ID_LB_DENOISE_DCT_BLOCK_SIZE, LB_CX_DENOISE_DCT_BLOCK_SIZE, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_denoise_dct_block_size);

    //smooth
    move_group(y_pos, col, col_width, CLFILTER_CHECK_SMOOTH_ENABLE, CLFILTER_CHECK_SMOOTH_MAX, CLFILTER_TRACK_SMOOTH_FIRST, CLFILTER_TRACK_SMOOTH_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_smooth_quality, ID_CX_SMOOTH_QUALITY, lb_smooth_quality, ID_LB_SMOOTH_QUALITY, LB_CX_SMOOTH_QUALITY, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_smooth_quality);

    //knn
    move_group(y_pos, col, col_width, CLFILTER_CHECK_KNN_ENABLE, CLFILTER_CHECK_KNN_MAX, CLFILTER_TRACK_KNN_FIRST, CLFILTER_TRACK_KNN_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_knn_radius, ID_CX_KNN_RADIUS, lb_knn_radius, ID_LB_KNN_RADIUS, LB_CX_KNN_RADIUS, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_raduis);

    //nlmeans
    move_group(y_pos, col, col_width, CLFILTER_CHECK_NLMEANS_ENABLE, CLFILTER_CHECK_NLMEANS_MAX, CLFILTER_TRACK_NLMEANS_FIRST, CLFILTER_TRACK_NLMEANS_MAX, track_bar_delta_y, ADD_CX_FIRST, 2, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_nlmeans_patch,  ID_CX_DENOISE_NLMEANS_PATCH,  lb_nlmeans_patch,  ID_LB_DENOISE_NLMEANS_PATCH,  LB_CX_DENOISE_NLMEANS_PATCH,  col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_nlmeans_block_size);
    add_combobox(cx_nlmeans_search, ID_CX_DENOISE_NLMEANS_SEARCH, lb_nlmeans_search, ID_LB_DENOISE_NLMEANS_SEARCH, LB_CX_DENOISE_NLMEANS_SEARCH, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_nlmeans_block_size);

    //pmd
    move_group(y_pos, col, col_width, CLFILTER_CHECK_PMD_ENABLE, CLFILTER_CHECK_PMD_MAX, CLFILTER_TRACK_PMD_FIRST, CLFILTER_TRACK_PMD_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);

    y_pos_max = std::max(y_pos_max, y_pos);
    // --- 次の列 -----------------------------------------
    col = 2;
    y_pos = cb_row_start_y_pos;
    //unsharp
    move_group(y_pos, col, col_width, CLFILTER_CHECK_UNSHARP_ENABLE, CLFILTER_CHECK_UNSHARP_MAX, CLFILTER_TRACK_UNSHARP_FIRST, CLFILTER_TRACK_UNSHARP_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_unsharp_radius, ID_CX_UNSHARP_RADIUS, lb_unsharp_radius, ID_LB_UNSHARP_RADIUS, LB_CX_UNSHARP_RADIUS, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_raduis);

    //エッジレベル調整
    move_group(y_pos, col, col_width, CLFILTER_CHECK_EDGELEVEL_ENABLE, CLFILTER_CHECK_EDGELEVEL_MAX, CLFILTER_TRACK_EDGELEVEL_FIRST, CLFILTER_TRACK_EDGELEVEL_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);

    //warpsharp
    move_group(y_pos, col, col_width, CLFILTER_CHECK_WARPSHARP_ENABLE, CLFILTER_CHECK_WARPSHARP_MAX, CLFILTER_TRACK_WARPSHARP_FIRST, CLFILTER_TRACK_WARPSHARP_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_warpsharp_blur, ID_CX_WARPSHARP_BLUR, lb_warpsharp_blur, ID_LB_WARPSHARP_BLUR, LB_CX_WARPSHARP_BLUR, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_1_to_10);

    //tweak
    move_group(y_pos, col, col_width, CLFILTER_CHECK_TWEAK_ENABLE, CLFILTER_CHECK_TWEAK_MAX, CLFILTER_TRACK_TWEAK_FIRST, CLFILTER_TRACK_TWEAK_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);

    //バンディング
    move_group(y_pos, col, col_width, CLFILTER_CHECK_DEBAND_ENABLE, CLFILTER_CHECK_DEBAND_MAX, CLFILTER_TRACK_DEBAND_FIRST, CLFILTER_TRACK_DEBAND_MAX, track_bar_delta_y, ADD_CX_AFTER_TRACK, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_deband_sample, ID_CX_DEBAND_SAMPLE, lb_deband_sample, ID_LB_DEBAND_SAMPLE, LB_CX_DEBAND_SAMPLE, col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_deband);

    //TrueHDR
    move_group(y_pos, col, col_width, CLFILTER_CHECK_TRUEHDR_ENABLE, CLFILTER_CHECK_TRUEHDR_MAX, CLFILTER_TRACK_NGX_TRUEHDR_FIRST, CLFILTER_TRACK_NGX_TRUEHDR_MAX, track_bar_delta_y, ADD_CX_AFTER_TRACK, 5, cx_y_pos, checkbox_idx, dialog_rc);
    const CLFILTER_TRACKBAR_DATA tb_truehdr[] = {
        { &tb_ngx_truehdr_contrast,     LB_CX_TRUEHDR_CONTRAST,     ID_TB_NGX_TRUEHDR_CONTRAST,       0,  200,  100, &cl_exdata.ngx_truehdr_contrast     },
        { &tb_ngx_truehdr_saturation,   LB_CX_TRUEHDR_SATURATION,   ID_TB_NGX_TRUEHDR_SATURATION,     0,  200,  100, &cl_exdata.ngx_truehdr_saturation   },
        { &tb_ngx_truehdr_middlegray,   LB_CX_TRUEHDR_MIDDLEGRAY,   ID_TB_NGX_TRUEHDR_MIDDLEGRAY,    10,  100,   50, &cl_exdata.ngx_truehdr_middlegray   },
        { &tb_ngx_truehdr_maxluminance, LB_CX_TRUEHDR_MAXLUMINANCE, ID_TB_NGX_TRUEHDR_MAXLUMINANCE, 400, 2000, 1000, &cl_exdata.ngx_truehdr_maxluminance },
        { 0 }
    };
    create_trackbars_ex(hwnd, hinst, col * col_width + 8 + AVIUTL_1_10_OFFSET, cx_y_pos, track_bar_delta_y, tb_truehdr);
    
    y_pos_max = std::max(y_pos_max, y_pos);

    // --- 次の列 -----------------------------------------
    col = 3;
    y_pos = cb_row_start_y_pos;

    //log_level
    const int cb_log_level_x = rc.left - dialog_rc.left + col * col_width;
    const int cb_log_level_y = cb_opencl_platform_y;
    static const int LB_LOG_LEVEL_W = 60;
    lb_log_level = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, cb_log_level_x, cb_log_level_y, LB_LOG_LEVEL_W, 24, hwnd, (HMENU)ID_LB_LOG_LEVEL, hinst, NULL);
    SendMessage(lb_log_level, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_log_level, WM_SETTEXT, 0, (LPARAM)LB_CX_LOG_LEVEL);

    static const int CX_LOG_LEVEL_W = 145;
    cx_log_level = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, cb_log_level_x + LB_LOG_LEVEL_W, cb_log_level_y, CX_LOG_LEVEL_W, 100, hwnd, (HMENU)ID_CX_LOG_LEVEL, hinst, NULL);
    SendMessage(cx_log_level, WM_SETFONT, (WPARAM)b_font, 0);

    for (const auto& log_level : RGY_LOG_LEVEL_STR) {
        set_combo_item(cx_log_level, log_level.second, log_level.first);
    }

    // log to file
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_LOG_TO_FILE], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_LOG_TO_FILE], HWND_TOP, cb_log_level_x + LB_LOG_LEVEL_W + CX_LOG_LEVEL_W, cb_log_level_y + 4, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    //フィルタのリスト
    const int list_filter_oder_x = rc.left - dialog_rc.left + col * col_width;
    const int list_filter_order_y = cb_row_start_y_pos;
    lb_filter_order = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, list_filter_oder_x + AVIUTL_1_10_OFFSET, list_filter_order_y, 60, 24, hwnd, (HMENU)ID_LB_FILTER_ORDER, hinst, NULL);
    SendMessage(lb_filter_order, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_filter_order, WM_SETTEXT, 0, (LPARAM)LB_CX_FILTER_ORDER);

    static const int bt_filter_order_width = 40;
    static const int list_filter_oder_width = col_width - bt_filter_order_width - 48;
    static const int list_filter_oder_height = 400;
    y_pos = list_filter_order_y + list_filter_oder_height;
    ls_filter_order = CreateWindow("LISTBOX", "clinfo", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_BORDER | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, list_filter_oder_x, list_filter_order_y + 24, list_filter_oder_width, list_filter_oder_height, hwnd, (HMENU)ID_LS_FILTER_ORDER, hinst, NULL);
    SendMessage(ls_filter_order, WM_SETFONT, (WPARAM)b_font, 0);
    //ls_filter_orderの中身はexdataが転送されてから、init_filter_order_listで初期化する

    bt_filter_order_up = CreateWindow("BUTTON", "↑", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_VCENTER, list_filter_oder_x + list_filter_oder_width + 8, list_filter_order_y + list_filter_oder_height / 2 - 24, bt_filter_order_width, 22, hwnd, (HMENU)ID_BT_FILTER_ORDER_UP, hinst, NULL);
    SendMessage(bt_filter_order_up, WM_SETFONT, (WPARAM)b_font, 0);

    bt_filter_order_down = CreateWindow("BUTTON", "↓", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_VCENTER, list_filter_oder_x + list_filter_oder_width + 8, list_filter_order_y + list_filter_oder_height / 2, bt_filter_order_width, 22, hwnd, (HMENU)ID_BT_FILTER_ORDER_DOWN, hinst, NULL);
    SendMessage(bt_filter_order_down, WM_SETFONT, (WPARAM)b_font, 0);

    y_pos_max = std::max(y_pos_max, y_pos);
    //---- 列追加終わり ------------------------------------------

    SetWindowPos(hwnd, HWND_TOP, 0, 0, (dialog_rc.right - dialog_rc.left) * columns, y_pos_max + 24, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

    update_cx(fp);
    set_filter_order();
}

//---------------------------------------------------------------------
//        フィルタ処理関数
//---------------------------------------------------------------------
struct mt_frame_copy_data {
    char *src;
    int srcPitch;
    char *dst;
    int dstPitch;
    int width;
    int height;
    int sizeOfPix;
};
void multi_thread_copy(int thread_id, int thread_num, void *param1, void *param2) {
    //    thread_id    : スレッド番号 ( 0 ～ thread_num-1 )
    //    thread_num    : スレッド数 ( 1 ～ )
    //    param1        : 汎用パラメータ
    //    param2        : 汎用パラメータ
    //
    //    この関数内からWin32APIや外部関数(rgb2yc,yc2rgbは除く)を使用しないでください。
    //

    const int max_threads = 4;
    if (thread_id >= max_threads) return;

    mt_frame_copy_data *prm = (mt_frame_copy_data *)param1;
    
    FILTER_PROC_INFO *fpip    = (FILTER_PROC_INFO *)param1;
    const int y_start = (prm->height * thread_id    ) / std::min(thread_num, max_threads);
    const int y_end   = (prm->height * (thread_id+1)) / std::min(thread_num, max_threads);

    for (int h = y_start; h < y_end; h++) {
        memcpy(prm->dst + h * prm->dstPitch, prm->src + h * prm->srcPitch, prm->width * prm->sizeOfPix);
    }
}

static clFilterChainParam func_proc_get_param(const FILTER *fp, const FILTER_PROC_INFO *fpip) {
    clFilterChainParam prm;
    //dllのモジュールハンドル
    prm.hModule = fp->dll_hinst;
    prm.log_level   = cl_exdata.log_level;
    prm.log_to_file = fp->check[CLFILTER_CHECK_LOG_TO_FILE] != 0;
    prm.outWidth    = (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) ? resize_res[cl_exdata.resize_idx].first  : fpip->w;
    prm.outHeight   = (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) ? resize_res[cl_exdata.resize_idx].second : fpip->h;

    const bool isCUDADevice = (cl_exdata.cl_dev_id.s.platform == CLCU_PLATFORM_CUDA);

    //フィルタ順序
    for (int i = 0; i < _countof(cl_exdata.filterOrder); i++) {
        if (cl_exdata.filterOrder[i] == VppType::VPP_NONE) break;
        prm.vpp.filterOrder.push_back(cl_exdata.filterOrder[i]);
    }

    //リサイズ
    prm.vpp.resize_algo        = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo;
    prm.vppnv.ngxVSR.quality   = cl_exdata.resize_ngx_vsr_quality;

    //colorspace
    ColorspaceConv conv;
    conv.from.matrix     = cl_exdata.csp_from.matrix;
    conv.from.colorprim  = cl_exdata.csp_from.colorprim;
    conv.from.transfer   = cl_exdata.csp_from.transfer;
    conv.from.colorrange = cl_exdata.csp_from.colorrange;
    conv.to.matrix       = (fp->check[CLFILTER_CHECK_COLORSPACE_MATRIX_ENABLE])    ? cl_exdata.csp_to.matrix     : cl_exdata.csp_from.matrix;
    conv.to.colorprim    = (fp->check[CLFILTER_CHECK_COLORSPACE_COLORPRIM_ENABLE]) ? cl_exdata.csp_to.colorprim  : cl_exdata.csp_from.colorprim;
    conv.to.transfer     = (fp->check[CLFILTER_CHECK_COLORSPACE_TRANSFER_ENABLE])  ? cl_exdata.csp_to.transfer   : cl_exdata.csp_from.transfer;
    conv.to.colorrange   = (fp->check[CLFILTER_CHECK_COLORSPACE_RANGE_ENABLE])     ? cl_exdata.csp_to.colorrange : cl_exdata.csp_from.colorrange;
    prm.vpp.colorspace.enable = fp->check[CLFILTER_CHECK_COLORSPACE_ENABLE] != 0;
    prm.vpp.colorspace.convs.push_back(conv);
    prm.vpp.colorspace.hdr2sdr.tonemap         = cl_exdata.hdr2sdr;
    prm.vpp.colorspace.hdr2sdr.hdr_source_peak = (double)fp->track[CLFILTER_TRACK_COLORSPACE_SOURCE_PEAK];
    prm.vpp.colorspace.hdr2sdr.ldr_nits        = (double)fp->track[CLFILTER_TRACK_COLORSPACE_LDR_NITS];
#if ENABLE_HDR2SDR_DESAT
    prm.colorspace.hdr2sdr.desat_base     = (float)fp->track[CLFILTER_TRACK_COLORSPACE_DESAT_BASE] * 0.01f;
    prm.colorspace.hdr2sdr.desat_strength = (float)fp->track[CLFILTER_TRACK_COLORSPACE_DESAT_STRENGTH] * 0.01f;
    prm.colorspace.hdr2sdr.desat_exp      = (float)fp->track[CLFILTER_TRACK_COLORSPACE_DESAT_EXP] * 0.1f;
#endif //#if ENABLE_HDR2SDR_DESAT

    //nnedi
    prm.vpp.nnedi.enable        = fp->check[CLFILTER_CHECK_NNEDI_ENABLE] != 0;
    prm.vpp.nnedi.field         = cl_exdata.nnedi_field;
    prm.vpp.nnedi.nsize         = cl_exdata.nnedi_nsize;
    prm.vpp.nnedi.nns           = cl_exdata.nnedi_nns;
    prm.vpp.nnedi.quality       = cl_exdata.nnedi_quality;
    prm.vpp.nnedi.pre_screen    = cl_exdata.nnedi_prescreen;
    prm.vpp.nnedi.errortype     = cl_exdata.nnedi_errortype;
    prm.vpp.nnedi.precision     = VPP_FP_PRECISION_AUTO;

    //nvvfx-superres
    prm.vppnv.nvvfxSuperRes.mode     = cl_exdata.nvvfx_superres_mode;
    prm.vppnv.nvvfxSuperRes.strength = fp->track[CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH] * 0.01f;

    //nvvfx-denoise
    prm.vppnv.nvvfxDenoise.enable   = isCUDADevice && fp->check[CLFILTER_CHECK_NVVFX_DENOISE_ENABLE] != 0;
    prm.vppnv.nvvfxDenoise.strength = (float)cl_exdata.nvvfx_denoise_strength;

    //nvvfx-artifact-reduction
    prm.vppnv.nvvfxArtifactReduction.enable = isCUDADevice && fp->check[CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE] != 0;
    prm.vppnv.nvvfxArtifactReduction.mode   = cl_exdata.nvvfx_artifact_reduction_mode;

    //denoise-dct
    prm.vpp.dct.enable        = fp->check[CLFILTER_CHECK_DENOISE_DCT_ENABLE] != 0;
    prm.vpp.dct.step          = cl_exdata.denoise_dct_step;
    prm.vpp.dct.sigma         = (float)fp->track[CLFILTER_TRACK_DENOISE_DCT_SIGMA] * 0.1f;
    prm.vpp.dct.block_size    = cl_exdata.denoise_dct_block_size;

    //smooth
    prm.vpp.smooth.enable     = fp->check[CLFILTER_CHECK_SMOOTH_ENABLE] != 0;
    prm.vpp.smooth.quality    = cl_exdata.smooth_quality;
    prm.vpp.smooth.qp         = fp->track[CLFILTER_TRACK_SMOOTH_QP];

    //knn
    prm.vpp.knn.enable         = fp->check[CLFILTER_CHECK_KNN_ENABLE] != 0;
    prm.vpp.knn.radius         = cl_exdata.knn_radius;
    prm.vpp.knn.strength       = (float)fp->track[CLFILTER_TRACK_KNN_STRENGTH] * 0.01f;
    prm.vpp.knn.lerpC          = (float)fp->track[CLFILTER_TRACK_KNN_LERP] * 0.01f;
    prm.vpp.knn.lerp_threshold = (float)fp->track[CLFILTER_TRACK_KNN_TH_LERP] * 0.01f;

    //nlmeans
    prm.vpp.nlmeans.enable     = fp->check[CLFILTER_CHECK_NLMEANS_ENABLE] != 0;
    prm.vpp.nlmeans.patchSize  = cl_exdata.nlmeans_patch;
    prm.vpp.nlmeans.searchSize = cl_exdata.nlmeans_search;
    prm.vpp.nlmeans.sigma      = (float)fp->track[CLFILTER_TRACK_NLMEANS_SIGMA] * 0.001f;
    prm.vpp.nlmeans.h          = (float)fp->track[CLFILTER_TRACK_NLMEANS_H] * 0.001f;

    //pmd
    prm.vpp.pmd.enable         = fp->check[CLFILTER_CHECK_PMD_ENABLE] != 0;
    prm.vpp.pmd.applyCount     = cl_exdata.pmd_apply_count;
    prm.vpp.pmd.strength       = (float)fp->track[CLFILTER_TRACK_PMD_STRENGTH];
    prm.vpp.pmd.threshold      = (float)fp->track[CLFILTER_TRACK_PMD_THRESHOLD];

    //unsharp
    prm.vpp.unsharp.enable    = fp->check[CLFILTER_CHECK_UNSHARP_ENABLE] != 0;
    prm.vpp.unsharp.radius    = cl_exdata.unsharp_radius;
    prm.vpp.unsharp.weight    = (float)fp->track[CLFILTER_TRACK_UNSHARP_WEIGHT] * 0.1f;
    prm.vpp.unsharp.threshold = (float)fp->track[CLFILTER_TRACK_UNSHARP_THRESHOLD];

    //edgelevel
    prm.vpp.edgelevel.enable    = fp->check[CLFILTER_CHECK_EDGELEVEL_ENABLE] != 0;
    prm.vpp.edgelevel.strength  = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_STRENGTH];
    prm.vpp.edgelevel.threshold = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_THRESHOLD];
    prm.vpp.edgelevel.black     = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_BLACK];
    prm.vpp.edgelevel.white     = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_WHITE];

    //warpsharp
    prm.vpp.warpsharp.enable    = fp->check[CLFILTER_CHECK_WARPSHARP_ENABLE] != 0;
    prm.vpp.warpsharp.threshold = (float)fp->track[CLFILTER_TRACK_WARPSHARP_THRESHOLD];
    prm.vpp.warpsharp.blur      = cl_exdata.warpsharp_blur;
    prm.vpp.warpsharp.type      = fp->check[CLFILTER_CHECK_WARPSHARP_SMALL_BLUR] ? 1 : 0;
    prm.vpp.warpsharp.depth     = (float)fp->track[CLFILTER_TRACK_WARPSHARP_DEPTH];
    prm.vpp.warpsharp.chroma    = fp->check[CLFILTER_CHECK_WARPSHARP_CHROMA_MASK] ? 1 : 0;

    //tweak
    prm.vpp.tweak.enable      = fp->check[CLFILTER_CHECK_TWEAK_ENABLE] != 0;
    prm.vpp.tweak.brightness  = (float)fp->track[CLFILTER_TRACK_TWEAK_BRIGHTNESS] * 0.01f;
    prm.vpp.tweak.contrast    = (float)fp->track[CLFILTER_TRACK_TWEAK_CONTRAST] * 0.01f;
    prm.vpp.tweak.gamma       = (float)fp->track[CLFILTER_TRACK_TWEAK_GAMMA] * 0.01f;
    prm.vpp.tweak.saturation  = (float)fp->track[CLFILTER_TRACK_TWEAK_SATURATION] * 0.01f;
    prm.vpp.tweak.hue         = (float)fp->track[CLFILTER_TRACK_TWEAK_HUE];

    //deband
    prm.vpp.deband.enable        = fp->check[CLFILTER_CHECK_DEBAND_ENABLE] != 0;
    prm.vpp.deband.range         = fp->track[CLFILTER_TRACK_DEBAND_RANGE];
    prm.vpp.deband.threY         = fp->track[CLFILTER_TRACK_DEBAND_Y];
    prm.vpp.deband.threCb        = fp->track[CLFILTER_TRACK_DEBAND_C];
    prm.vpp.deband.threCr        = fp->track[CLFILTER_TRACK_DEBAND_C];
    prm.vpp.deband.ditherY       = fp->track[CLFILTER_TRACK_DEBAND_DITHER_Y];
    prm.vpp.deband.ditherC       = fp->track[CLFILTER_TRACK_DEBAND_DITHER_C];
    prm.vpp.deband.sample        = cl_exdata.deband_sample;
    prm.vpp.deband.seed          = 1234;
    prm.vpp.deband.blurFirst     = fp->check[CLFILTER_CHECK_DEBAND_BLUR_FIRST] != 0;
    prm.vpp.deband.randEachFrame = fp->check[CLFILTER_CHECK_DEBAND_RAND_EACH_FRAME] != 0;

    //TrueHDR
    prm.vppnv.ngxTrueHDR.enable       = fp->check[CLFILTER_CHECK_TRUEHDR_ENABLE] != 0;
    prm.vppnv.ngxTrueHDR.contrast     = cl_exdata.ngx_truehdr_contrast;
    prm.vppnv.ngxTrueHDR.saturation   = cl_exdata.ngx_truehdr_saturation;
    prm.vppnv.ngxTrueHDR.middleGray   = cl_exdata.ngx_truehdr_middlegray;
    prm.vppnv.ngxTrueHDR.maxLuminance = cl_exdata.ngx_truehdr_maxluminance;

    return prm;
}

BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip) {
    const int out_width  = (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) ? resize_res[cl_exdata.resize_idx].first  : fpip->w;
    const int out_height = (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) ? resize_res[cl_exdata.resize_idx].second : fpip->h;
    const bool resize_required = out_width != fpip->w || out_height != fpip->h;
    const auto prm = func_proc_get_param(fp, fpip);
    if (prm.getFilterChain(resize_required).size() == 0) { // 適用すべきフィルタがない場合
        return TRUE; // 何もしない
    }

    const int current_frame = fpip->frame; // 現在のフレーム番号
    const int frame_n = fpip->frame_n;
    if (fpip->frame_n <= 0 || current_frame < 0) {
        return TRUE; // 何もしない
    }
    // GPUリストを作成
    init_device_list();
    if (g_clfiltersAufDevices->getPlatforms().size() == 0) {
        return TRUE; // 何もしない
    }
    // 選択対象がGPUリストにあるかを確認
    auto dev = g_clfiltersAufDevices->findDevice(cl_exdata.cl_dev_id.s.platform, cl_exdata.cl_dev_id.s.device);
    if (!dev) {
        return TRUE; // 何もしない
    }
    // フィルタ処理の実行
    if (!g_clfiltersAuf || platformIsCUDA(cl_exdata.cl_dev_id.s.platform) != g_clfiltersAuf->isCUDA()) {
        init_clfilter_exe(fp);
    }
    g_clfiltersAuf->setLogLevel(cl_exdata.log_level);
    const BOOL ret = g_clfiltersAuf->funcProc(prm, fp, fpip);
    fpip->w = out_width;
    fpip->h = out_height;
    return ret;
}

BOOL clcuFiltersAuf::funcProc(const clFilterChainParam& prm, FILTER *fp, FILTER_PROC_INFO *fpip) {
    // exe側のis_saving, clfilter->getNextOutFrameId()を取得
    auto sharedPrms = (clfitersSharedPrms *)m_sharedPrms->ptr();
    auto is_saving = sharedPrms->is_saving;
    const auto prevNextOutFrameId = sharedPrms->nextOutFrameId;
    const auto prev_pd = sharedPrms->pd;
    int32_t resetPipeline = FALSE;

    if (   prev_pd.s.platform != cl_exdata.cl_dev_id.s.platform
        || prev_pd.s.device != cl_exdata.cl_dev_id.s.device) {
        std::string mes = AUF_FULL_NAME;
        mes += ": ";
        const auto dev = g_clfiltersAufDevices->findDevice(cl_exdata.cl_dev_id.s.platform, cl_exdata.cl_dev_id.s.device);
        mes += (dev) ? tchar_to_string(dev->devName) : LB_WND_OPENCL_AVAIL;
        SendMessage(fp->hwnd, WM_SETTEXT, 0, (LPARAM)mes.c_str());
        is_saving = FALSE;
        resetPipeline = TRUE;
    }

    const int current_frame = fpip->frame; // 現在のフレーム番号
    const int frame_n = fpip->frame_n;

    // どのフレームから処理を開始すべきか?
    int frameIn   = current_frame + ((is_saving) ? frameInOffset   : 0);
    int frameProc = current_frame + ((is_saving) ? frameProcOffset : 0);
    int frameOut  = current_frame + ((is_saving) ? frameOutOffset  : 0);
    if (resetPipeline
        || prevNextOutFrameId != current_frame // 出てくる予定のフレームがずれていたらリセット
        || is_saving != fp->exfunc->is_saving(fpip->editp)) { // モードが切り替わったらリセット
        resetPipeline = TRUE;
        frameIn   = current_frame;
        frameProc = current_frame;
        frameOut  = current_frame;
        is_saving = fp->exfunc->is_saving(fpip->editp);
    }
    // メモリを使用してしまうが、やむを得ない
    if (!fp->exfunc->set_ycp_filtering_cache_size(fp, fpip->max_w, fpip->max_h, frameInOffset, NULL)) {
        // 失敗 ... メモリを確保できなかったので、逐次モードに切り替え
        is_saving = FALSE;
    }

    const int frameInFin = (is_saving) ? std::min(current_frame + frameInOffset, frame_n - 1) : current_frame;

    // 共有メモリへの値の設定
    sharedPrms->pd = cl_exdata.cl_dev_id;
    sharedPrms->is_saving = is_saving;
    sharedPrms->currentFrameId = fpip->frame;
    sharedPrms->frame_n = fpip->frame_n;
    sharedPrms->frameIn = frameIn;
    sharedPrms->frameInFin = frameInFin;
    sharedPrms->frameProc = frameProc;
    sharedPrms->frameOut = frameOut;
    sharedPrms->resetPipeLine = resetPipeline;
    strcpy_s(sharedPrms->prms, tchar_to_string(prm.genCmd()).c_str());
    m_log->write(RGY_LOG_TRACE, RGY_LOGT_CORE, "currentFrameId: %d, frameIn: %d, frameInFin: %d, frameProc %d, frameOut %d, reset %d, %s\n",
        sharedPrms->currentFrameId, sharedPrms->frameIn, sharedPrms->frameInFin, sharedPrms->frameProc, sharedPrms->frameOut, sharedPrms->resetPipeLine, prm.genCmd().c_str());

    // -- フレームの転送 -----------------------------------------------------------------
    // 初期化
    for (size_t i = 0; i < _countof(sharedPrms->srcFrame); i++) {
        initPrms(&sharedPrms->srcFrame[i]);
    }
    // frameIn の終了フレーム
    // フレーム転送の実行
    for (int i = 0; frameIn <= frameInFin; frameIn++, i++) {
        int width = fpip->w, height = fpip->h;
        const PIXEL_YC *srcptr = (frameIn == current_frame) ? fpip->ycp_edit : (PIXEL_YC *)fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, frameIn, &width, &height);
        sharedPrms->srcFrame[i].frameId = frameIn;
        sharedPrms->srcFrame[i].width = width;
        sharedPrms->srcFrame[i].height = height;
        sharedPrms->srcFrame[i].pitchBytes = m_sharedFramesPitchBytes;
        
        mt_frame_copy_data copyPrm;
        copyPrm.src = (char *)srcptr;
        copyPrm.srcPitch = fpip->max_w * sizeof(PIXEL_YC);
        copyPrm.dst = (char *)m_sharedFrames[sharedPrms->currentFrameId % m_sharedFrames.size()]->ptr();
        copyPrm.dstPitch = m_sharedFramesPitchBytes;
        copyPrm.width = width;
        copyPrm.height = height;
        copyPrm.sizeOfPix = sizeof(PIXEL_YC);

        if (i > 0) {
            // 2フレーム目以降はプロセスのGPUへのフレーム転送完了を待つ
            while (WaitForSingleObject(m_eventMesEnd.get(), 100) == WAIT_TIMEOUT) {
                // exeの生存を確認
                if (!m_process->processAlive()) {
                    return FALSE;
                }
            }
        }

        fp->exfunc->exec_multi_thread_func(multi_thread_copy, (void *)&copyPrm, nullptr);

        if (frameIn < frameInFin) {
            // 複数フレームを送る場合、1フレームずつ転送するので、プロセス側を起動する
            clfitersSharedMesData *message = (clfitersSharedMesData*)m_sharedMessage->ptr();
            message->type = clfitersMes::FuncProc;
            SetEvent(m_eventMesStart.get());
        }
    }
    // プロセス側に処理開始を通知
    clfitersSharedMesData *message = (clfitersSharedMesData*)m_sharedMessage->ptr();
    message->type = clfitersMes::FuncProc;
    SetEvent(m_eventMesStart.get());

    // プロセス側の処理終了を待機
    while (WaitForSingleObject(m_eventMesEnd.get(), 100) == WAIT_TIMEOUT) {
        // exeの生存を確認
        if (!m_process->processAlive()) {
            return FALSE;
        }
    }
    // エラーの確認
    if (message->ret != TRUE) {
        std::string mes = AUF_FULL_NAME;
        mes += ": ";
        const auto dev = g_clfiltersAufDevices->findDevice(cl_exdata.cl_dev_id.s.platform, cl_exdata.cl_dev_id.s.device);
        mes += (dev) ? tchar_to_string(dev->devName) : LB_WND_OPENCL_AVAIL;
        mes += ": ";
        mes += message->data;
        SendMessage(fp->hwnd, WM_SETTEXT, 0, (LPARAM)mes.c_str());
        return FALSE;
    }
    // 共有メモリからコピー
    mt_frame_copy_data copyPrm;
    copyPrm.src = (char *)m_sharedFrames[(sharedPrms->currentFrameId + 1) % m_sharedFrames.size()]->ptr();
    copyPrm.srcPitch = m_sharedFramesPitchBytes;
    copyPrm.dst = (char *)fpip->ycp_edit;
    copyPrm.dstPitch = fpip->max_w * sizeof(PIXEL_YC);
    copyPrm.width = prm.outWidth;
    copyPrm.height = prm.outHeight;
    copyPrm.sizeOfPix = sizeof(PIXEL_YC);
    fp->exfunc->exec_multi_thread_func(multi_thread_copy, (void *)&copyPrm, nullptr);
    return TRUE;
}
