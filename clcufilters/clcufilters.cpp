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
#include "clcufilters_control.h"
#include "clcufilters.h"

void init_device_list();
void init_dialog(HWND hwnd, FILTER *fp);
void update_cx(FILTER *fp);
void update_filter_enable(HWND hwnd, const size_t icol);

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

    // custom trackbarはIDを5つ使用する
    ID_TB_NGX_TRUEHDR_CONTRAST,
    ID_TB_NGX_TRUEHDR_SATURATION = ID_TB_NGX_TRUEHDR_CONTRAST + 5,
    ID_TB_NGX_TRUEHDR_MIDDLEGRAY = ID_TB_NGX_TRUEHDR_SATURATION + 5,
    ID_TB_NGX_TRUEHDR_MAXLUMINANCE = ID_TB_NGX_TRUEHDR_MIDDLEGRAY + 5,

    ID_TB_RESIZE_PL_CLAMP = ID_TB_NGX_TRUEHDR_MAXLUMINANCE + 5,
    ID_TB_RESIZE_PL_TAPER = ID_TB_RESIZE_PL_CLAMP + 5,
    ID_TB_RESIZE_PL_BLUR = ID_TB_RESIZE_PL_TAPER + 5,
    ID_TB_RESIZE_PL_ANTIRING = ID_TB_RESIZE_PL_BLUR + 5,

    ID_TB_LIBPLACEBO_DEBAND_ITERATIONS = ID_TB_RESIZE_PL_ANTIRING + 5,
    ID_TB_LIBPLACEBO_DEBAND_THRESHOLD = ID_TB_LIBPLACEBO_DEBAND_ITERATIONS + 5,
    ID_TB_LIBPLACEBO_DEBAND_RADIUS = ID_TB_LIBPLACEBO_DEBAND_THRESHOLD + 5,
    ID_TB_LIBPLACEBO_DEBAND_GRAIN = ID_TB_LIBPLACEBO_DEBAND_RADIUS + 5,
    ID_LB_LIBPLACEBO_DEBAND_DITHER = ID_TB_LIBPLACEBO_DEBAND_GRAIN + 5,
    ID_CX_LIBPLACEBO_DEBAND_DITHER,
    ID_LB_LIBPLACEBO_DEBAND_LUT_SIZE,
    ID_CX_LIBPLACEBO_DEBAND_LUT_SIZE,

    ID_LB_LIBPLACEBO_TONEMAP_SRC_CSP,
    ID_CX_LIBPLACEBO_TONEMAP_SRC_CSP,
    ID_LB_LIBPLACEBO_TONEMAP_DST_CSP,
    ID_CX_LIBPLACEBO_TONEMAP_DST_CSP,
    ID_TB_LIBPLACEBO_TONEMAP_SRC_MAX,
    ID_TB_LIBPLACEBO_TONEMAP_SRC_MIN = ID_TB_LIBPLACEBO_TONEMAP_SRC_MAX + 5,
    ID_TB_LIBPLACEBO_TONEMAP_DST_MAX = ID_TB_LIBPLACEBO_TONEMAP_SRC_MIN + 5,
    ID_TB_LIBPLACEBO_TONEMAP_DST_MIN = ID_TB_LIBPLACEBO_TONEMAP_DST_MAX + 5,
    ID_TB_LIBPLACEBO_TONEMAP_SMOOTH_PERIOD = ID_TB_LIBPLACEBO_TONEMAP_DST_MIN + 5,
    ID_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_LOW = ID_TB_LIBPLACEBO_TONEMAP_SMOOTH_PERIOD + 5,
    ID_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_HIGH = ID_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_LOW + 5,
    ID_TB_LIBPLACEBO_TONEMAP_PERCENTILE = ID_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_HIGH + 5,
    ID_TB_LIBPLACEBO_TONEMAP_BLACK_CUTOFF = ID_TB_LIBPLACEBO_TONEMAP_PERCENTILE + 5,
    ID_LB_LIBPLACEBO_TONEMAP_GAMUT_MAPPING = ID_TB_LIBPLACEBO_TONEMAP_BLACK_CUTOFF + 5,
    ID_CX_LIBPLACEBO_TONEMAP_GAMUT_MAPPING,
    ID_LB_LIBPLACEBO_TONEMAP_FUNCTION,
    ID_CX_LIBPLACEBO_TONEMAP_FUNCTION,

    ID_LB_LIBPLACEBO_TONEMAP_METADATA,
    ID_CX_LIBPLACEBO_TONEMAP_METADATA,
    ID_TB_LIBPLACEBO_TONEMAP_CONTRAST_RECOVERY = ID_CX_LIBPLACEBO_TONEMAP_METADATA + 5,
    ID_TB_LIBPLACEBO_TONEMAP_CONTRAST_SMOOTHNESS = ID_TB_LIBPLACEBO_TONEMAP_CONTRAST_RECOVERY + 5,
    ID_LB_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER,
    ID_CX_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER,
    ID_LB_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM,
    ID_CX_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM,

    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_ADAPTATION,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MIN = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_ADAPTATION + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MAX = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MIN + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_DEFAULT = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MAX + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_OFFSET = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_DEFAULT + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_TUNING = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_OFFSET + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_OFFSET = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_TUNING + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SPLINE_CONTRAST = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_OFFSET + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_REINHARD_CONTRAST = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SPLINE_CONTRAST + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_LINEAR_KNEE = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_REINHARD_CONTRAST + 5,
    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_EXPOSURE = ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_LINEAR_KNEE + 5,
};

#pragma pack(1)

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

    int resize_pl_clamp;
    int resize_pl_taper;
    int resize_pl_blur;
    int resize_pl_antiring;

    int libplacebo_deband_iterations;
    int libplacebo_deband_threshold;
    int libplacebo_deband_radius;
    int libplacebo_deband_grain;
    int libplacebo_deband_dither;
    int libplacebo_deband_lut_size;

    int libplacebo_tonemap_src_csp;
    int libplacebo_tonemap_dst_csp;
    int libplacebo_tonemap_src_max;
    int libplacebo_tonemap_src_min;
    int libplacebo_tonemap_dst_max;
    int libplacebo_tonemap_dst_min;
    int libplacebo_tonemap_smooth_period;
    int libplacebo_tonemap_scene_threshold_low;
    int libplacebo_tonemap_scene_threshold_high;
    int libplacebo_tonemap_percentile;
    int libplacebo_tonemap_black_cutoff;
    int libplacebo_tonemap_gamut_mapping;
    int libplacebo_tonemap_function;
    int libplacebo_tonemap_metadata;
    int libplacebo_tonemap_contrast_recovery;
    int libplacebo_tonemap_contrast_smoothness;
    int libplacebo_tonemap_dst_pl_transfer;
    int libplacebo_tonemap_dst_pl_colorprim;

    int libplacebo_tonemap_tone_const_knee_adaptation; // For st2094-40, st2094-10, spline
    int libplacebo_tonemap_tone_const_knee_min;        // For st2094-40, st2094-10, spline
    int libplacebo_tonemap_tone_const_knee_max;        // For st2094-40, st2094-10, spline
    int libplacebo_tonemap_tone_const_knee_default;    // For st2094-40, st2094-10, spline
    int libplacebo_tonemap_tone_const_knee_offset;     // For bt2390
    int libplacebo_tonemap_tone_const_slope_tuning;    // For spline
    int libplacebo_tonemap_tone_const_slope_offset;    // For spline
    int libplacebo_tonemap_tone_const_spline_contrast; // For spline
    int libplacebo_tonemap_tone_const_reinhard_contrast; // For reinhard
    int libplacebo_tonemap_tone_const_linear_knee;     // For mobius, gamma
    int libplacebo_tonemap_tone_const_exposure;        // For linear, linearlight

    char reserved[436];
};
# pragma pack()
static const int extra_track_data_offset = 32;
static const size_t exdatasize = sizeof(CLFILTER_EXDATA);
static_assert(exdatasize == 1024);
static const int FILTER_ROW = 3;

static int g_col_width = 235;
static int g_min_height = 360;
static CLFILTER_EXDATA cl_exdata;

static std::unique_ptr<clcuFiltersAufDevices> g_clfiltersAufDevices;
static std::unique_ptr<clcuFiltersAuf> g_clfiltersAuf;
static int g_cuda_device_nvvfx_support = -1;
static int g_resize_nvvfx_superres = -1;
static int g_resize_ngx_vsr_quality = -1;
static int g_libplacebo_tonemap_function = -1;
static int g_resize_libplacebo = -1;
static std::vector<CLFILTER_TRACKBAR_DATA> g_trackBars;
static HBRUSH g_hbrBackground = NULL;
static std::array<std::vector<std::unique_ptr<CLCU_FILTER_CONTROLS>>, FILTER_ROW> g_filterControls;

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
static const char *LB_TB_TRUEHDR_CONTRAST = "コントラスト";
static const char *LB_TB_TRUEHDR_SATURATION = "彩度";
static const char *LB_TB_TRUEHDR_MIDDLEGRAY = "目標輝度";
static const char *LB_TB_TRUEHDR_MAXLUMINANCE = "最大輝度";
static const char *LB_TB_RESIZE_PL_CLAMP = "clamp";
static const char *LB_TB_RESIZE_PL_TAPER = "taper";
static const char *LB_TB_RESIZE_PL_BLUR = "blur";
static const char *LB_TB_RESIZE_PL_ANTIRING = "antiring";
static const char *LB_TB_LIBPLACEBO_DEBAND_ITERATIONS = "iterations";
static const char *LB_TB_LIBPLACEBO_DEBAND_THRESHOLD = "threshold";
static const char *LB_TB_LIBPLACEBO_DEBAND_RADIUS = "radius";
static const char *LB_TB_LIBPLACEBO_DEBAND_GRAIN = "grain";
static const char *LB_CX_LIBPLACEBO_DEBAND_DITHER = "dither";
static const char *LB_CX_LIBPLACEBO_DEBAND_LUT_SIZE = "LUT size";
static const char *LB_CX_LIBPLACEBO_TONEMAP_SRC_CSP = "src csp";
static const char *LB_CX_LIBPLACEBO_TONEMAP_DST_CSP = "dst csp";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SRC_MAX = "src max";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SRC_MIN = "src min";
static const char *LB_TB_LIBPLACEBO_TONEMAP_DST_MAX = "dst max";
static const char *LB_TB_LIBPLACEBO_TONEMAP_DST_MIN = "dst min";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SMOOTH_PERIOD = "smooth period";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_LOW = "scene th_low";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_HIGH = "scene th_high";
static const char *LB_TB_LIBPLACEBO_TONEMAP_PERCENTILE = "percentile";
static const char *LB_TB_LIBPLACEBO_TONEMAP_BLACK_CUTOFF = "black cutoff";
static const char *LB_CX_LIBPLACEBO_TONEMAP_GAMUT_MAPPING = "gamut";
static const char *LB_CX_LIBPLACEBO_TONEMAP_FUNCTION = "function";
static const char *LB_CX_LIBPLACEBO_TONEMAP_METADATA = "metadata";
static const char *LB_TB_LIBPLACEBO_TONEMAP_CONTRAST_RECOVERY = "contrast recover";
static const char *LB_TB_LIBPLACEBO_TONEMAP_CONTRAST_SMOOTHNESS = "contrast smooth";
static const char *LB_CX_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER = "transfer";
static const char *LB_CX_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM = "colorprim";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_ADAPTATION = "knee adaptation";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MIN = "knee min";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MAX = "knee max";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_DEFAULT = "knee default";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_OFFSET = "knee offset";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_TUNING = "slope tuning";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_OFFSET = "slope offset";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SPLINE_CONTRAST = "spline contrast";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_REINHARD_CONTRAST = "reinhard contrast";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_LINEAR_KNEE = "linear knee";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_EXPOSURE = "exposure";
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
static const char *LB_TB_TRUEHDR_CONTRAST = "contrast";
static const char *LB_TB_TRUEHDR_SATURATION = "saturation";
static const char *LB_TB_TRUEHDR_MIDDLEGRAY = "middlegray";
static const char *LB_TB_TRUEHDR_MAXLUMINANCE = "maxluminance";
static const char *LB_TB_RESIZE_PL_CLAMP = "clamp";
static const char *LB_TB_RESIZE_PL_TAPER = "taper";
static const char *LB_TB_RESIZE_PL_BLUR = "blur";
static const char *LB_TB_RESIZE_PL_ANTIRING = "antiring";
static const char *LB_TB_LIBPLACEBO_DEBAND_ITERATIONS = "iterations";
static const char *LB_TB_LIBPLACEBO_DEBAND_THRESHOLD = "threshold";
static const char *LB_TB_LIBPLACEBO_DEBAND_RADIUS = "radius";
static const char *LB_TB_LIBPLACEBO_DEBAND_GRAIN = "grain";
static const char *LB_CX_LIBPLACEBO_DEBAND_DITHER = "dither";
static const char *LB_CX_LIBPLACEBO_DEBAND_LUT_SIZE = "LUT size";
static const char *LB_CX_LIBPLACEBO_TONEMAP_SRC_CSP = "src csp";
static const char *LB_CX_LIBPLACEBO_TONEMAP_DST_CSP = "dst csp";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SRC_MAX = "src max";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SRC_MIN = "src min";
static const char *LB_TB_LIBPLACEBO_TONEMAP_DST_MAX = "dst max";
static const char *LB_TB_LIBPLACEBO_TONEMAP_DST_MIN = "dst min";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SMOOTH_PERIOD = "smooth period";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_LOW = "scene th_low";
static const char *LB_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_HIGH = "scene th_high";
static const char *LB_TB_LIBPLACEBO_TONEMAP_PERCENTILE = "percentile";
static const char *LB_TB_LIBPLACEBO_TONEMAP_BLACK_CUTOFF = "black cutoff";
static const char *LB_CX_LIBPLACEBO_TONEMAP_GAMUT_MAPPING = "gamut";
static const char *LB_CX_LIBPLACEBO_TONEMAP_FUNCTION = "function";
static const char *LB_CX_LIBPLACEBO_TONEMAP_METADATA = "metadata";
static const char *LB_TB_LIBPLACEBO_TONEMAP_CONTRAST_RECOVERY = "contrast recover";
static const char *LB_TB_LIBPLACEBO_TONEMAP_CONTRAST_SMOOTHNESS = "contrast smooth";
static const char *LB_CX_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER = "transfer";
static const char *LB_CX_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM = "colorprim";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_ADAPTATION = "knee adaptation";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MIN = "knee min";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MAX = "knee max";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_DEFAULT = "knee default";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_OFFSET = "knee offset";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_TUNING = "slope tuning";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_OFFSET = "slope offset";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SPLINE_CONTRAST = "spline contrast";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_REINHARD_CONTRAST = "reinhard contrast";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_LINEAR_KNEE = "linear knee";
static const char *LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_EXPOSURE = "exposure";
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

    CLFILTER_TRACK_LIBPLACEBO_TONEMAP_FIRST = CLFILTER_TRACK_COLORSPACE_MAX,
    CLFILTER_TRACK_LIBPLACEBO_TONEMAP_MAX = CLFILTER_TRACK_LIBPLACEBO_TONEMAP_FIRST,

    CLFILTER_TRACK_NVVFX_DENOISE_FIRST = CLFILTER_TRACK_LIBPLACEBO_TONEMAP_MAX,
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

    CLFILTER_TRACK_LIBPLACEBO_DEBAND_FIRST = CLFILTER_TRACK_DEBAND_MAX,
    CLFILTER_TRACK_LIBPLACEBO_DEBAND_MAX = CLFILTER_TRACK_LIBPLACEBO_DEBAND_FIRST,

    CLFILTER_TRACK_NNEDI_FIRST = CLFILTER_TRACK_LIBPLACEBO_DEBAND_MAX,
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
    1,0,0, //knn
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
    "tonemap (libplacebo)",
    "インタレ解除 (nnedi)",
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
    "バンディング低減 (libplacebo)",
    "TrueHDR"
};
const TCHAR *check_name_en[] = {
#if ENABLE_FIELD
    "process field",
#endif //#if ENABLE_FIELD
    "log to file", // log to file
    "resize",
    "colorspace", "matrix", "colorprim", "transfer", "range",
    "libplacebo-tonemap",
    "(deint) nnedi",
    "(denoise) nvvfx-denoise",
    "(denoise) nvvfx-artifact-reduction",
    "(denoise) denoise-dct",
    "(denoise) smooth",
    "(denoise) knn",
    "(denoise) nlmeans",
    "(denoise) pmd",
    "(sharp) unsharp",
    "(sharp) edgelevel",
    "(sharp) warpsharp", "type [off:13x13, on:5x5]", "chroma",
    "tweak",
    "deband", "blurfirst", "rand_each_frame",
    "libplacebo-deband",
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

    CLFILTER_CHECK_LIBPLACEBO_TONEMAP_ENABLE = CLFILTER_CHECK_COLORSPACE_MAX,
    CLFILTER_CHECK_LIBPLACEBO_TONEMAP_MAX,

    CLFILTER_CHECK_NNEDI_ENABLE = CLFILTER_CHECK_LIBPLACEBO_TONEMAP_MAX,
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

    CLFILTER_CHECK_LIBPLACEBO_DEBAND_ENABLE = CLFILTER_CHECK_DEBAND_MAX,
    CLFILTER_CHECK_LIBPLACEBO_DEBAND_MAX,

    CLFILTER_CHECK_TRUEHDR_ENABLE = CLFILTER_CHECK_LIBPLACEBO_DEBAND_MAX,
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
    0, //libpalcebo-tonemap
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
    0, // libplacebo-deband
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

    VppLibplaceboResample resample;
    cl_exdata.resize_pl_clamp = (int)(resample.clamp_ * 100.0f + 0.5f);
    cl_exdata.resize_pl_taper = (int)(resample.taper * 100.0f + 0.5f);
    cl_exdata.resize_pl_blur = (int)(resample.blur + 0.5f);
    cl_exdata.resize_pl_antiring = (int)(resample.antiring * 100.0f + 0.5f);

    VppLibplaceboDeband libplaceboDeband;
    cl_exdata.libplacebo_deband_iterations = libplaceboDeband.iterations;
    cl_exdata.libplacebo_deband_threshold = (int)(libplaceboDeband.threshold * 10.0f + 0.5f);
    cl_exdata.libplacebo_deband_radius = (int)(libplaceboDeband.radius + 0.5f);
    cl_exdata.libplacebo_deband_grain = (int)(libplaceboDeband.grainY + 0.5f);
    cl_exdata.libplacebo_deband_dither = (int)libplaceboDeband.dither;
    cl_exdata.libplacebo_deband_lut_size = libplaceboDeband.lut_size;

    VppLibplaceboToneMapping libplaceboToneMapping;
    cl_exdata.libplacebo_tonemap_src_csp = (int)libplaceboToneMapping.src_csp;
    cl_exdata.libplacebo_tonemap_dst_csp = (int)libplaceboToneMapping.dst_csp;
    cl_exdata.libplacebo_tonemap_src_max = (int)(libplaceboToneMapping.src_max + 0.5f);
    cl_exdata.libplacebo_tonemap_src_min = (int)(libplaceboToneMapping.src_min + 0.5f);
    cl_exdata.libplacebo_tonemap_dst_max = (int)(libplaceboToneMapping.dst_max + 0.5f);
    cl_exdata.libplacebo_tonemap_dst_min = (int)(libplaceboToneMapping.dst_min + 0.5f);
    cl_exdata.libplacebo_tonemap_smooth_period = (int)(libplaceboToneMapping.smooth_period + 0.5f);
    cl_exdata.libplacebo_tonemap_scene_threshold_low = (int)(libplaceboToneMapping.scene_threshold_low * 10.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_scene_threshold_high = (int)(libplaceboToneMapping.scene_threshold_high * 10.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_percentile = (int)(libplaceboToneMapping.percentile * 10.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_black_cutoff = (int)(libplaceboToneMapping.black_cutoff + 0.5f);
    cl_exdata.libplacebo_tonemap_gamut_mapping = (int)libplaceboToneMapping.gamut_mapping;
    cl_exdata.libplacebo_tonemap_function = (int)libplaceboToneMapping.tonemapping_function;
    cl_exdata.libplacebo_tonemap_metadata = (int)libplaceboToneMapping.metadata;
    cl_exdata.libplacebo_tonemap_contrast_recovery = (int)(libplaceboToneMapping.contrast_recovery * 10.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_contrast_smoothness = (int)(libplaceboToneMapping.contrast_smoothness * 10.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_dst_pl_transfer = (int)libplaceboToneMapping.dst_pl_transfer;
    cl_exdata.libplacebo_tonemap_dst_pl_colorprim = (int)libplaceboToneMapping.dst_pl_colorprim;

    cl_exdata.libplacebo_tonemap_tone_const_knee_adaptation = (int)(libplaceboToneMapping.tone_constants.st2094.knee_adaptation * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_knee_min = (int)(libplaceboToneMapping.tone_constants.st2094.knee_min * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_knee_max = (int)(libplaceboToneMapping.tone_constants.st2094.knee_max * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_knee_default = (int)(libplaceboToneMapping.tone_constants.st2094.knee_default * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_knee_offset = (int)(libplaceboToneMapping.tone_constants.bt2390.knee_offset * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_slope_tuning = (int)(libplaceboToneMapping.tone_constants.spline.slope_tuning * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_slope_offset = (int)(libplaceboToneMapping.tone_constants.spline.slope_offset * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_spline_contrast = (int)(libplaceboToneMapping.tone_constants.spline.spline_contrast * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_reinhard_contrast = (int)(libplaceboToneMapping.tone_constants.reinhard.contrast * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_linear_knee = (int)(libplaceboToneMapping.tone_constants.mobius.linear_knee * 100.0f + 0.5f);
    cl_exdata.libplacebo_tonemap_tone_const_exposure = (int)(libplaceboToneMapping.tone_constants.linear.exposure * 100.0f + 0.5f);
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

static CLFILTER_TRACKBAR tb_resize_pl_clamp;
static CLFILTER_TRACKBAR tb_resize_pl_taper;
static CLFILTER_TRACKBAR tb_resize_pl_blur;
static CLFILTER_TRACKBAR tb_resize_pl_antiring;

static CLFILTER_TRACKBAR tb_libplacebo_deband_iterations;
static CLFILTER_TRACKBAR tb_libplacebo_deband_threshold;
static CLFILTER_TRACKBAR tb_libplacebo_deband_radius;
static CLFILTER_TRACKBAR tb_libplacebo_deband_grain;
static HWND lb_libplacebo_deband_dither;
static HWND cx_libplacebo_deband_dither;
static HWND lb_libplacebo_deband_lut_size;
static HWND cx_libplacebo_deband_lut_size;

static HWND lb_libplacebo_tonemap_src_csp;
static HWND cx_libplacebo_tonemap_src_csp;
static HWND lb_libplacebo_tonemap_dst_csp;
static HWND cx_libplacebo_tonemap_dst_csp;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_src_max;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_src_min;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_dst_max;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_dst_min;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_smooth_period;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_scene_threshold_low;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_scene_threshold_high;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_percentile;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_black_cutoff;
static HWND lb_libplacebo_tonemap_gamut_mapping;
static HWND cx_libplacebo_tonemap_gamut_mapping;
static HWND lb_libplacebo_tonemap_function;
static HWND cx_libplacebo_tonemap_function;
static HWND lb_libplacebo_tonemap_metadata;
static HWND cx_libplacebo_tonemap_metadata;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_contrast_recovery;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_contrast_smoothness;
static HWND lb_libplacebo_tonemap_dst_pl_transfer;
static HWND cx_libplacebo_tonemap_dst_pl_transfer;
static HWND lb_libplacebo_tonemap_dst_pl_colorprim;
static HWND cx_libplacebo_tonemap_dst_pl_colorprim;

static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_knee_adaptation;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_knee_min;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_knee_max;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_knee_default;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_knee_offset;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_slope_tuning;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_slope_offset;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_spline_contrast;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_reinhard_contrast;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_linear_knee;
static CLFILTER_TRACKBAR tb_libplacebo_tonemap_tone_const_exposure;

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
    } else if (hwnd == cx_libplacebo_deband_dither) {
        cl_exdata.libplacebo_deband_dither = value;
    } else if (hwnd == cx_libplacebo_deband_lut_size) {
        cl_exdata.libplacebo_deband_lut_size = value;
    } else if (hwnd == cx_libplacebo_tonemap_src_csp) {
        cl_exdata.libplacebo_tonemap_src_csp = value;
    } else if (hwnd == cx_libplacebo_tonemap_dst_csp) {
        cl_exdata.libplacebo_tonemap_dst_csp = value;
    } else if (hwnd == cx_libplacebo_tonemap_gamut_mapping) {
        cl_exdata.libplacebo_tonemap_gamut_mapping = value;
    } else if (hwnd == cx_libplacebo_tonemap_function) {
        cl_exdata.libplacebo_tonemap_function = value;
    } else if (hwnd == cx_libplacebo_tonemap_metadata) {
        cl_exdata.libplacebo_tonemap_metadata = value;
    } else if (hwnd == cx_libplacebo_tonemap_dst_pl_transfer) {
        cl_exdata.libplacebo_tonemap_dst_pl_transfer = value;
    } else if (hwnd == cx_libplacebo_tonemap_dst_pl_colorprim) {
        cl_exdata.libplacebo_tonemap_dst_pl_colorprim = value;
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
    for (int i = 0; list_vpp_resize[i].desc; i++) {
        if (_tcsncmp(list_vpp_resize[i].desc, _T("libplacebo-"), _tcslen(_T("libplacebo-"))) == 0) {
            set_combo_item(cx_resize_algo, tchar_to_string(list_vpp_resize[i].desc).c_str(), list_vpp_resize[i].value);
        }
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
    select_combo_item(cx_libplacebo_deband_dither,    cl_exdata.libplacebo_deband_dither);
    select_combo_item(cx_libplacebo_deband_lut_size,  cl_exdata.libplacebo_deband_lut_size);
    select_combo_item(cx_libplacebo_tonemap_src_csp,           cl_exdata.libplacebo_tonemap_src_csp);
    select_combo_item(cx_libplacebo_tonemap_dst_csp,           cl_exdata.libplacebo_tonemap_dst_csp);
    select_combo_item(cx_libplacebo_tonemap_gamut_mapping,     cl_exdata.libplacebo_tonemap_gamut_mapping);
    select_combo_item(cx_libplacebo_tonemap_function,          cl_exdata.libplacebo_tonemap_function);
    select_combo_item(cx_libplacebo_tonemap_metadata,          cl_exdata.libplacebo_tonemap_metadata);
    select_combo_item(cx_libplacebo_tonemap_dst_pl_transfer,   cl_exdata.libplacebo_tonemap_dst_pl_transfer);
    select_combo_item(cx_libplacebo_tonemap_dst_pl_colorprim,  cl_exdata.libplacebo_tonemap_dst_pl_colorprim);
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
        tb_ngx_truehdr_contrast.show_hide(cudaNvvfxSupport);
        tb_ngx_truehdr_saturation.show_hide(cudaNvvfxSupport);
        tb_ngx_truehdr_middlegray.show_hide(cudaNvvfxSupport);
        tb_ngx_truehdr_maxluminance.show_hide(cudaNvvfxSupport);
        if (!cudaNvvfxSupport) {
            fp->check[CLFILTER_CHECK_TRUEHDR_ENABLE] = FALSE;
            SendMessage(child_hwnd[checkbox_idx + CLFILTER_CHECK_TRUEHDR_ENABLE], BM_SETCHECK, BST_UNCHECKED, 0);
        }

        g_cuda_device_nvvfx_support = cudaNvvfxSupport;
    }
}

// リサイズ関連の表示/非表示切り替え
static void update_resize_algo_params(FILTER *fp) {
    const int resize_nvvfx_superres = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo == RGY_VPP_RESIZE_NVVFX_SUPER_RES ? 1 : 0;
    const int resize_ngx_vsr_quality = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo == RGY_VPP_RESIZE_NGX_VSR ? 1 : 0;
    const int resize_libplacebo = isLibplaceboResizeFiter((RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo) ? 1 : 0;
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
    if (g_resize_libplacebo != resize_libplacebo) {
        tb_resize_pl_clamp.show_hide(resize_libplacebo);
        tb_resize_pl_taper.show_hide(resize_libplacebo);
        tb_resize_pl_blur.show_hide(resize_libplacebo);
        tb_resize_pl_antiring.show_hide(resize_libplacebo);
        g_resize_libplacebo = resize_libplacebo;
    }
}

//tonemap関連の表示/非表示切り替え
static void update_tonemap_params() {
    const auto tonemap_libplacebo = (VppLibplaceboToneMappingFunction)cl_exdata.libplacebo_tonemap_function;
    if ((VppLibplaceboToneMappingFunction)g_libplacebo_tonemap_function != tonemap_libplacebo) {
        const bool eneable_st2094 = tonemap_libplacebo == VppLibplaceboToneMappingFunction::st2094_40
            || tonemap_libplacebo == VppLibplaceboToneMappingFunction::st2094_10
            || tonemap_libplacebo == VppLibplaceboToneMappingFunction::spline;
        tb_libplacebo_tonemap_tone_const_knee_adaptation.show_hide(eneable_st2094);
        tb_libplacebo_tonemap_tone_const_knee_min.show_hide(eneable_st2094);
        tb_libplacebo_tonemap_tone_const_knee_max.show_hide(eneable_st2094);
        tb_libplacebo_tonemap_tone_const_knee_default.show_hide(eneable_st2094);
        tb_libplacebo_tonemap_tone_const_slope_tuning.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::spline);
        tb_libplacebo_tonemap_tone_const_slope_offset.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::spline);
        tb_libplacebo_tonemap_tone_const_spline_contrast.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::spline);
        tb_libplacebo_tonemap_tone_const_knee_offset.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::bt2390);
        tb_libplacebo_tonemap_tone_const_reinhard_contrast.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::reinhard);
        tb_libplacebo_tonemap_tone_const_linear_knee.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::mobius || tonemap_libplacebo == VppLibplaceboToneMappingFunction::gamma);
        tb_libplacebo_tonemap_tone_const_exposure.show_hide(tonemap_libplacebo == VppLibplaceboToneMappingFunction::linear || tonemap_libplacebo == VppLibplaceboToneMappingFunction::linearlight);
        g_libplacebo_tonemap_function = cl_exdata.libplacebo_tonemap_function;
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
    case WM_FILTER_CHANGE_PARAM:
        {
            const int targetParam = HIWORD(wparam); // 1 ... trackbar, 2 ... checkbox
            if (targetParam == 2) {
                const int targetID = LOWORD(wparam);
                bool targetFound = false;
                for (size_t icol = 0; !targetFound && icol < g_filterControls.size(); icol++) {
                    for (auto& filterControl : g_filterControls[icol]) {
                        if (filterControl->first_check_idx() == targetID) {
                            update_filter_enable(hwnd, icol);
                            targetFound = true;
                            break;
                        }
                    }
                }
            }
            update_cuda_enable(fp);
            update_resize_algo_params(fp);
            update_tonemap_params();
        }
        break;
    case WM_FILTER_UPDATE: // フィルタ更新
    case WM_FILTER_SAVE_END: // セーブ終了
        update_cx(fp);
        init_filter_order_list(fp);
        for (size_t icol = 0; icol < g_filterControls.size(); icol++) {
            update_filter_enable(hwnd, icol);
        }
        update_cuda_enable(fp);
        update_resize_algo_params(fp);
        update_tonemap_params();
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

void set_combobox_items(HWND hwnd_cx, const CX_DESC *cx_items, int limit = INT_MAX) {
    for (int i = 0; cx_items[i].desc && i < limit; i++) {
        set_combo_item(hwnd_cx, (char *)cx_items[i].desc, cx_items[i].value);
    }
}

void move_track_bar(int& y_pos, int col, int col_width, int track_min, int track_max, int track_bar_delta_y, const RECT& dialog_rc) {
    for (int i = track_min; i < track_max; i++, y_pos += track_bar_delta_y) {
        for (int j = 0; j < 5; j++) {
            RECT rc;
            GetWindowRect(child_hwnd[i * 5 + j + 1], &rc);
            SetWindowPos(child_hwnd[i * 5 + j + 1], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }
}

void add_combobox(HWND& hwnd_cx, int id_cx, HWND& hwnd_lb, int id_lb, const char *lb_str, HFONT b_font, HWND hwnd, HINSTANCE hinst, const CX_DESC *cx_items, int cx_item_limit = INT_MAX) {
    hwnd_lb = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, 0, 0, 58, 24, hwnd, (HMENU)id_lb, hinst, NULL);
    SendMessage(hwnd_lb, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(hwnd_lb, WM_SETTEXT, 0, (LPARAM)lb_str);
    hwnd_cx = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 145, 100, hwnd, (HMENU)id_cx, hinst, NULL);
    SendMessage(hwnd_cx, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx, cx_items, cx_item_limit);
}

void update_filter_enable(HWND hwnd, const size_t icol_change) {
    const int cb_row_start_y_pos = 8 + 28;
    int y_pos = cb_row_start_y_pos;
    // 変更対象の列のフィルタの高さ調整
    for (auto& filterControl : g_filterControls[icol_change]) {
        filterControl->show_hide();
        filterControl->move_group(y_pos, icol_change, g_col_width);
    }

    // 変更対象意外の列の高さを計算
    int y_pos_max = y_pos;
    for (size_t icol = 0; icol < g_filterControls.size(); icol++) {
        y_pos = cb_row_start_y_pos;
        if (icol != icol_change) {
            for (auto& filterControl : g_filterControls[icol]) {
                y_pos += filterControl->y_actual_size();
            }
        }
        y_pos_max = std::max(y_pos_max, y_pos);
    }
    // ダイアログの高さを計算、g_min_heightとも比較する
    y_pos_max = std::max(y_pos_max, g_min_height);
    const int columns = (int)g_filterControls.size() + 1;
    SetWindowPos(hwnd, HWND_TOP, 0, 0, g_col_width * columns, y_pos_max + 36 /*狭くなりすぎるので調整*/, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
}

struct CLCX_COMBOBOX {
    HWND& hwnd_cx;
    int id_cx;
    HWND& hwnd_lb;
    int id_lb;
    const char *lb_str;
    const CX_DESC *cx_items;
    int cx_item_limit;

    CLCX_COMBOBOX(HWND& hwnd_cx, int id_cx, HWND& hwnd_lb, int id_lb, const char *lb_str, const CX_DESC *cx_items, int cx_item_limit = INT_MAX)
        : hwnd_cx(hwnd_cx), id_cx(id_cx), hwnd_lb(hwnd_lb), id_lb(id_lb), lb_str(lb_str), cx_items(cx_items), cx_item_limit(cx_item_limit) {
    }
};

void add_combobox(CLCU_FILTER_CONTROLS *filter_controls, const std::vector<CLCX_COMBOBOX>& add_cx, int cx_y_pos, HFONT b_font, HWND hwnd, HINSTANCE hinst) {
    for (auto& cx : add_cx) {
        add_combobox(cx.hwnd_cx, cx.id_cx, cx.hwnd_lb, cx.id_lb, cx.lb_str, b_font, hwnd, hinst, cx.cx_items, cx.cx_item_limit);
        CLCU_CONTROL control;
        control.hwnd = cx.hwnd_lb;
        control.id = cx.id_lb;
        control.offset_x = 10 + AVIUTL_1_10_OFFSET;
        control.offset_y = cx_y_pos;
        filter_controls->add_control(control);

        control.hwnd = cx.hwnd_cx;
        control.id = cx.id_cx;
        control.offset_x = 68 + AVIUTL_1_10_OFFSET;
        control.offset_y = cx_y_pos;
        filter_controls->add_control(control);

        cx_y_pos += CX_HEIGHT;
    }
}

void add_checkboxs_excpet_key(CLCU_FILTER_CONTROLS *filter_controls, int& offset_y, const int checkbox_idx, int check_min, int check_max, const int track_bar_delta_y, const RECT& dialog_rc) {
    for (int i = check_min + 1; i < check_max; i++, offset_y += track_bar_delta_y) {
        RECT rc;
        GetWindowRect(child_hwnd[checkbox_idx + i], &rc);
        CLCU_CONTROL control;
        control.hwnd = child_hwnd[checkbox_idx + i];
        control.id = GetDlgCtrlID(control.hwnd);
        control.offset_x = rc.left - dialog_rc.left + 10;
        control.offset_y = offset_y;
        filter_controls->add_control(control);
    }
}

void add_trackbars(CLCU_FILTER_CONTROLS *filter_controls, int& offset_y, int track_min, int track_max, const int track_bar_delta_y, const RECT& dialog_rc) {
    for (int i = track_min; i < track_max; i++, offset_y += track_bar_delta_y) {
        for (int j = 0; j < 5; j++) {
            RECT rc;
            GetWindowRect(child_hwnd[i * 5 + j + 1], &rc);
            CLCU_CONTROL control;
            control.hwnd = child_hwnd[i * 5 + j + 1];
            control.id = GetDlgCtrlID(control.hwnd);
            control.offset_x = rc.left - dialog_rc.left;
            control.offset_y = offset_y;
            filter_controls->add_control(control);
        }
    }
}

void create_trackbars_ex(CLCU_FILTER_CONTROLS *filter_controls, const std::vector<CLFILTER_TRACKBAR_DATA>& list_track_bar_ex, HWND hwndParent, HINSTANCE hInstance, const int x, int& offset_y, const int track_bar_delta_y, const RECT& dialog_rc) {
    for (auto& track_bar_ex : list_track_bar_ex) {
        (*track_bar_ex.tb) = create_trackbar_ex(hwndParent, hInstance, track_bar_ex.labelText, x, offset_y, track_bar_ex.label_id, track_bar_ex.val_min, track_bar_ex.val_max, track_bar_ex.val_default);
        g_trackBars.push_back(track_bar_ex);

        int control_id = track_bar_ex.label_id;
        for (auto hwnd : { track_bar_ex.tb->label, track_bar_ex.tb->trackbar, track_bar_ex.tb->bt_left, track_bar_ex.tb->bt_right, track_bar_ex.tb->bt_text }) {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            CLCU_CONTROL control;
            control.hwnd = hwnd;
            control.id = control_id++;
            control.offset_x = rc.left - dialog_rc.left;
            control.offset_y = offset_y;
            filter_controls->add_control(control);
        }
        offset_y += track_bar_delta_y;
    }
}

std::unique_ptr<CLCU_FILTER_CONTROLS> create_group(int check_min, int check_max, int track_min, int track_max, const int track_bar_delta_y, const std::vector<CLFILTER_TRACKBAR_DATA>& list_track_bar_ex,
    const AddCXMode add_cx_mode, const std::vector<CLCX_COMBOBOX>& add_cx, const int checkbox_idx, const RECT& dialog_rc, HFONT b_font, HWND hwnd, HINSTANCE hinst) {
    auto filter_controls = std::make_unique<CLCU_FILTER_CONTROLS>(check_min, check_max, track_min, track_max);
    int offset_y = 0;
    int cx_y_pos = 0;

    RECT rc;
    GetWindowRect(child_hwnd[checkbox_idx + check_min], &rc);
    // 有効無効のボックス
    CLCU_CONTROL control;
    control.hwnd = child_hwnd[checkbox_idx + check_min];
    control.id = GetDlgCtrlID(control.hwnd);
    control.offset_x = rc.left - dialog_rc.left;
    control.offset_y = offset_y;
    filter_controls->add_control(control);
    offset_y += track_bar_delta_y;

    if (add_cx.size() > 0 && add_cx_mode == ADD_CX_FIRST) {
        cx_y_pos = offset_y + 2;                // すこし窮屈なので +2pix
        offset_y += CX_HEIGHT * add_cx.size() + 4; // すこし窮屈なので +4pix
        add_combobox(filter_controls.get(), add_cx, cx_y_pos, b_font, hwnd, hinst);
    }

    // trackbar
    add_trackbars(filter_controls.get(), offset_y, track_min, track_max, track_bar_delta_y, dialog_rc);
    create_trackbars_ex(filter_controls.get(), list_track_bar_ex, hwnd, hinst, 0, offset_y, track_bar_delta_y, dialog_rc);

    if (add_cx.size() > 0 && add_cx_mode == ADD_CX_AFTER_TRACK) {
        cx_y_pos = offset_y + 2;                // すこし窮屈なので +2pix
        offset_y += CX_HEIGHT * add_cx.size() + 4; // すこし窮屈なので +4pix
        add_combobox(filter_controls.get(), add_cx, cx_y_pos, b_font, hwnd, hinst);
    }

    // checkbox
    add_checkboxs_excpet_key(filter_controls.get(), offset_y, checkbox_idx, check_min, check_max, track_bar_delta_y, dialog_rc);

    if (add_cx.size() > 0 && add_cx_mode == ADD_CX_AFTER_CHECK) {
        cx_y_pos = offset_y + 2;                // すこし窮屈なので +2pix
        offset_y += CX_HEIGHT * add_cx.size() + 4; // すこし窮屈なので +4pix
        add_combobox(filter_controls.get(), add_cx, cx_y_pos, b_font, hwnd, hinst);
    }
    offset_y += track_bar_delta_y;
    filter_controls->set_y_size(offset_y);
    return filter_controls;
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

void add_combobox_from_to(
    CLCU_FILTER_CONTROLS *filter_controls,
    HWND& hwnd_cx_from, int id_cx_from, HWND& hwnd_cx_to, int id_cx_to,
    HWND& hwnd_lb_from_to, int id_lb_from_to,
    const char *lb_str, int& offset_y, HFONT b_font, HWND hwnd, HINSTANCE hinst, const CX_DESC *cx_items, int cx_item_limit = INT_MAX) {
    CLCU_CONTROL control;
    control.offset_y = offset_y;

    // from
    hwnd_cx_from = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 98 + AVIUTL_1_10_OFFSET, offset_y, 90, 100, hwnd, (HMENU)id_cx_from, hinst, NULL);
    SendMessage(hwnd_cx_from, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx_from, cx_items, cx_item_limit);
    control.hwnd = hwnd_cx_from;
    control.id = id_cx_from;
    control.offset_x = 98 + AVIUTL_1_10_OFFSET;
    filter_controls->add_control(control);
    // label (from->to)
    hwnd_lb_from_to = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, 192 + AVIUTL_1_10_OFFSET, offset_y, 10, 24, hwnd, (HMENU)id_lb_from_to, hinst, NULL);
    SendMessage(hwnd_lb_from_to, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(hwnd_lb_from_to, WM_SETTEXT, 0, (LPARAM)"→");
    control.hwnd = hwnd_lb_from_to;
    control.id = id_lb_from_to;
    control.offset_x = 192 + AVIUTL_1_10_OFFSET;
    filter_controls->add_control(control);
    // to
    hwnd_cx_to = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 208 + AVIUTL_1_10_OFFSET, offset_y, 90, 100, hwnd, (HMENU)id_cx_to, hinst, NULL);
    SendMessage(hwnd_cx_to, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx_to, cx_items, cx_item_limit);
    control.hwnd = hwnd_cx_to;
    control.id = id_cx_to;
    control.offset_x = 208 + AVIUTL_1_10_OFFSET;
    filter_controls->add_control(control);

    offset_y += CX_HEIGHT;
}

std::unique_ptr<CLCU_FILTER_CONTROLS> create_colorspace(int check_min, int check_max, int track_min, int track_max, const int track_bar_delta_y, const int checkbox_idx, const RECT& dialog_rc, HFONT b_font, HWND hwnd, HINSTANCE hinst) {
    auto filter_controls = std::make_unique<CLCU_FILTER_CONTROLS>(check_min, check_max, track_min, track_max);
    int offset_y = 0;
    RECT rc;
    GetWindowRect(child_hwnd[checkbox_idx + check_min], &rc);
    // 有効無効のボックス
    CLCU_CONTROL control;
    control.hwnd = child_hwnd[checkbox_idx + check_min];
    control.id = GetDlgCtrlID(control.hwnd);
    control.offset_x = rc.left - dialog_rc.left;
    control.offset_y = offset_y;
    filter_controls->add_control(control);
    offset_y += track_bar_delta_y;

    int cx_y_pos = offset_y;

    // checkbox
    add_checkboxs_excpet_key(filter_controls.get(), offset_y, checkbox_idx, check_min, check_max, track_bar_delta_y, dialog_rc);

    add_combobox_from_to(filter_controls.get(), cx_colorspace_colormatrix_from, ID_CX_COLORSPACE_COLORMATRIX_FROM, cx_colorspace_colormatrix_to, ID_CX_COLORSPACE_COLORMATRIX_TO,
        lb_colorspace_colormatrix_from_to, ID_LB_COLORSPACE_COLORMATRIX_FROM_TO,
        "matrix", cx_y_pos, b_font, hwnd, hinst, list_colormatrix + 3);
    add_combobox_from_to(filter_controls.get(), cx_colorspace_colorprim_from, ID_CX_COLORSPACE_COLORPRIM_FROM, cx_colorspace_colorprim_to, ID_CX_COLORSPACE_COLORPRIM_TO,
        lb_colorspace_colorprim_from_to, ID_LB_COLORSPACE_COLORPRIM_FROM_TO,
        "prim", cx_y_pos, b_font, hwnd, hinst, list_colorprim + 4);
    add_combobox_from_to(filter_controls.get(), cx_colorspace_transfer_from, ID_CX_COLORSPACE_TRANSFER_FROM, cx_colorspace_transfer_to, ID_CX_COLORSPACE_TRANSFER_TO,
        lb_colorspace_transfer_from_to, ID_LB_COLORSPACE_TRANSFER_FROM_TO,
        "transfer", cx_y_pos, b_font, hwnd, hinst, list_transfer + 4);
    add_combobox_from_to(filter_controls.get(), cx_colorspace_colorrange_from, ID_CX_COLORSPACE_COLORRANGE_FROM, cx_colorspace_colorrange_to, ID_CX_COLORSPACE_COLORRANGE_TO,
        lb_colorspace_colorrange_from_to, ID_LB_COLORSPACE_COLORRANGE_FROM_TO,
        "range", cx_y_pos, b_font, hwnd, hinst, list_colorrange + 2, 2);

    offset_y = std::max(offset_y, cx_y_pos);

    const std::vector<CLCX_COMBOBOX> add_cx = { CLCX_COMBOBOX(cx_colorspace_hdr2sdr, ID_CX_COLORSPACE_HDR2SDR, lb_colorspace_hdr2sdr, ID_LB_COLORSPACE_HDR2SDR, "hdr2sdr", list_vpp_hdr2sdr) };
    cx_y_pos = offset_y + 2;                // すこし窮屈なので +2pix
    offset_y += CX_HEIGHT * add_cx.size() + 4; // すこし窮屈なので +4pix
    add_combobox(filter_controls.get(), add_cx, cx_y_pos, b_font, hwnd, hinst);

    // trackbar
    add_trackbars(filter_controls.get(), offset_y, track_min, track_max, track_bar_delta_y, dialog_rc);

    offset_y += track_bar_delta_y;
    filter_controls->set_y_size(offset_y);
    return filter_controls;
}

std::unique_ptr<CLCU_FILTER_CONTROLS> create_resize(const int track_bar_delta_y, const int checkbox_idx, const RECT& dialog_rc, HFONT b_font, HWND hwnd, HINSTANCE hinst, FILTER *fp) {
    auto filter_controls = std::make_unique<CLCU_FILTER_CONTROLS>(CLFILTER_CHECK_RESIZE_ENABLE, CLFILTER_CHECK_RESIZE_ENABLE, -1, -1);
    int offset_y = 0;

    RECT rc;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], &rc);
    // 有効無効のボックス
    CLCU_CONTROL control;
    control.hwnd = child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE];
    control.id = GetDlgCtrlID(control.hwnd);
    control.offset_x = rc.left - dialog_rc.left;
    control.offset_y = offset_y;
    filter_controls->add_control(control);
    offset_y += track_bar_delta_y;

    control.id = ID_LB_RESIZE_RES;
    control.offset_x = 8 + AVIUTL_1_10_OFFSET;
    control.offset_y = 24;
    lb_proc_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, control.offset_x, control.offset_y, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_RES, hinst, NULL);
    SendMessage(lb_proc_mode, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_proc_mode, WM_SETTEXT, 0, (LPARAM)LB_CX_RESIZE_SIZE);
    control.hwnd = lb_proc_mode;
    filter_controls->add_control(control);

    control.id = ID_CX_RESIZE_RES;
    control.offset_x = 68;
    cx_resize_res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, control.offset_x, control.offset_y, 145, 100, hwnd, (HMENU)ID_CX_RESIZE_RES, hinst, NULL);
    SendMessage(cx_resize_res, WM_SETFONT, (WPARAM)b_font, 0);
    control.hwnd = cx_resize_res;
    filter_controls->add_control(control);

    control.id = ID_BT_RESIZE_RES_ADD;
    control.offset_x = 214;
    bt_resize_res_add = CreateWindow("BUTTON", LB_BT_RESIZE_ADD, WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, control.offset_x, control.offset_y, 32, 22, hwnd, (HMENU)ID_BT_RESIZE_RES_ADD, hinst, NULL);
    SendMessage(bt_resize_res_add, WM_SETFONT, (WPARAM)b_font, 0);
    control.hwnd = bt_resize_res_add;
    filter_controls->add_control(control);

    control.id = ID_BT_RESIZE_RES_DEL;
    control.offset_x = 246;
    bt_resize_res_del = CreateWindow("BUTTON", LB_BT_RESIZE_DELETE, WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, control.offset_x, control.offset_y, 32, 22, hwnd, (HMENU)ID_BT_RESIZE_RES_DEL, hinst, NULL);
    SendMessage(bt_resize_res_del, WM_SETFONT, (WPARAM)b_font, 0);
    control.hwnd = bt_resize_res_del;
    filter_controls->add_control(control);

    control.id = ID_CX_RESIZE_ALGO;
    control.offset_x = 68;
    control.offset_y = 48;
    cx_resize_algo = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, control.offset_x, control.offset_y, 210, 160, hwnd, (HMENU)ID_CX_RESIZE_ALGO, hinst, NULL);
    SendMessage(cx_resize_algo, WM_SETFONT, (WPARAM)b_font, 0);
    control.hwnd = cx_resize_algo;
    filter_controls->add_control(control);

    update_cx_resize_res_items(fp);
    set_resize_algo_items(cl_exdata.cl_dev_id.s.platform == CLCU_PLATFORM_CUDA);

    // --- 最初の列 -----------------------------------------
    // NGXは1行
    control.id = ID_LB_RESIZE_NGX_VSR_QUALITY;
    control.offset_x = 8 + AVIUTL_1_10_OFFSET;
    control.offset_y = 72;
    lb_resize_ngx_vsr_quality = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, control.offset_x, control.offset_y, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_NGX_VSR_QUALITY, hinst, NULL);
    SendMessage(lb_resize_ngx_vsr_quality, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_resize_ngx_vsr_quality, WM_SETTEXT, 0, (LPARAM)LB_CX_RESIZE_NGX_VSR_QUALITY);
    control.hwnd = lb_resize_ngx_vsr_quality;
    filter_controls->add_control(control);

    control.id = ID_CX_RESIZE_NGX_VSR_QUALITY;
    control.offset_x = 68;
    cx_resize_ngx_vsr_quality = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, control.offset_x, control.offset_y, 145, 160, hwnd, (HMENU)ID_CX_RESIZE_NGX_VSR_QUALITY, hinst, NULL);
    SendMessage(cx_resize_ngx_vsr_quality, WM_SETFONT, (WPARAM)b_font, 0);
    set_combo_item(cx_resize_ngx_vsr_quality, "1 - fast", 1);
    set_combo_item(cx_resize_ngx_vsr_quality, "2", 2);
    set_combo_item(cx_resize_ngx_vsr_quality, "3", 3);
    set_combo_item(cx_resize_ngx_vsr_quality, "4 - slow", 4);
    control.hwnd = cx_resize_ngx_vsr_quality;
    filter_controls->add_control(control);

    // NVVFXはNGXに重ねて同じ位置から2行
    control.id = ID_LB_NVVFX_SUPRERES_MODE;
    control.offset_x = 8 + AVIUTL_1_10_OFFSET;
    lb_nvvfx_superres_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, control.offset_x, control.offset_y, 60, 24, hwnd, (HMENU)ID_LB_NVVFX_SUPRERES_MODE, hinst, NULL);
    SendMessage(lb_nvvfx_superres_mode, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_nvvfx_superres_mode, WM_SETTEXT, 0, (LPARAM)LB_CX_NVVFX_SUPRERES_MODE);
    control.hwnd = lb_nvvfx_superres_mode;
    filter_controls->add_control(control);

    control.id = ID_CX_NVVFX_SUPRERES_MODE;
    control.offset_x = 68;
    cx_nvvfx_superres_mode = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, control.offset_x, control.offset_y, 145, 100, hwnd, (HMENU)ID_CX_NVVFX_SUPRERES_MODE, hinst, NULL);
    SendMessage(cx_nvvfx_superres_mode, WM_SETFONT, (WPARAM)b_font, 0);
    set_combo_item(cx_nvvfx_superres_mode, "0 - light", 0);
    set_combo_item(cx_nvvfx_superres_mode, "1 - strong", 1);
    control.hwnd = cx_nvvfx_superres_mode;
    filter_controls->add_control(control);

    // NVVFXの2行目
    offset_y = track_bar_delta_y * 4 + 8;

    offset_y -= track_bar_delta_y / 4;
    add_trackbars(filter_controls.get(), offset_y, CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH, CLFILTER_TRACK_RESIZE_NVVFX_SUPRERES_STRENGTH + 1, track_bar_delta_y, dialog_rc);
    offset_y += track_bar_delta_y / 4;

    // libplacebo(resize)もNGXに重ねて同じ位置から
    const std::vector<CLFILTER_TRACKBAR_DATA> list_track_bar_ex = {
        { &tb_resize_pl_clamp,    LB_TB_RESIZE_PL_CLAMP,    ID_TB_RESIZE_PL_CLAMP,      0,  100,  0, &cl_exdata.resize_pl_clamp    },
        { &tb_resize_pl_taper,    LB_TB_RESIZE_PL_TAPER,    ID_TB_RESIZE_PL_TAPER,      0,  100,  0, &cl_exdata.resize_pl_taper    },
        { &tb_resize_pl_blur,     LB_TB_RESIZE_PL_BLUR,     ID_TB_RESIZE_PL_BLUR,       0,  100,  0, &cl_exdata.resize_pl_blur     },
        { &tb_resize_pl_antiring, LB_TB_RESIZE_PL_ANTIRING, ID_TB_RESIZE_PL_ANTIRING,   0,  100,  0, &cl_exdata.resize_pl_antiring }
    };
    int cx_y_pos = 72;
    create_trackbars_ex(filter_controls.get(), list_track_bar_ex, hwnd, hinst, 0, cx_y_pos, track_bar_delta_y, dialog_rc);

    // libplacebo(resize)とNVVFXの2行目の大きいほう
    filter_controls->set_y_size(std::max(offset_y, cx_y_pos) + 8);
    return filter_controls;
}

std::unique_ptr<CLCU_FILTER_CONTROLS> create_libplacebo_tonemapping(const int track_bar_delta_y, const int checkbox_idx, const RECT& dialog_rc, HFONT b_font, HWND hwnd, HINSTANCE hinst) {
    const int check_min = CLFILTER_CHECK_LIBPLACEBO_TONEMAP_ENABLE;

    auto filter_controls = std::make_unique<CLCU_FILTER_CONTROLS>(CLFILTER_CHECK_LIBPLACEBO_TONEMAP_ENABLE, CLFILTER_CHECK_LIBPLACEBO_TONEMAP_MAX, CLFILTER_TRACK_LIBPLACEBO_TONEMAP_FIRST, CLFILTER_TRACK_LIBPLACEBO_TONEMAP_MAX);
    int offset_y = 0;
    RECT rc;
    GetWindowRect(child_hwnd[checkbox_idx + check_min], &rc);
    // 有効無効のボックス
    CLCU_CONTROL control;
    control.hwnd = child_hwnd[checkbox_idx + check_min];
    control.id = GetDlgCtrlID(control.hwnd);
    control.offset_x = rc.left - dialog_rc.left;
    control.offset_y = offset_y;
    filter_controls->add_control(control);
    offset_y += track_bar_delta_y;

    int cx_y_pos = offset_y;

    const std::vector<CLCX_COMBOBOX> add_cx = {
        CLCX_COMBOBOX(cx_libplacebo_tonemap_src_csp,          ID_CX_LIBPLACEBO_TONEMAP_SRC_CSP,          lb_libplacebo_tonemap_src_csp,          ID_LB_LIBPLACEBO_TONEMAP_SRC_CSP,          LB_CX_LIBPLACEBO_TONEMAP_SRC_CSP,          list_vpp_libplacebo_tone_mapping_csp),
        CLCX_COMBOBOX(cx_libplacebo_tonemap_dst_csp,          ID_CX_LIBPLACEBO_TONEMAP_DST_CSP,          lb_libplacebo_tonemap_dst_csp,          ID_LB_LIBPLACEBO_TONEMAP_DST_CSP,          LB_CX_LIBPLACEBO_TONEMAP_DST_CSP,          list_vpp_libplacebo_tone_mapping_csp)
    };
    cx_y_pos = offset_y + 2;                // すこし窮屈なので +2pix
    offset_y += CX_HEIGHT * add_cx.size() + 4; // すこし窮屈なので +4pix
    add_combobox(filter_controls.get(), add_cx, cx_y_pos, b_font, hwnd, hinst);

    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_tonemap = {
        { &tb_libplacebo_tonemap_src_max,              LB_TB_LIBPLACEBO_TONEMAP_SRC_MAX,              ID_TB_LIBPLACEBO_TONEMAP_SRC_MAX,              1,  2000, 1000, &cl_exdata.libplacebo_tonemap_src_max },
        { &tb_libplacebo_tonemap_src_min,              LB_TB_LIBPLACEBO_TONEMAP_SRC_MIN,              ID_TB_LIBPLACEBO_TONEMAP_SRC_MIN,              0,  2000,    1, &cl_exdata.libplacebo_tonemap_src_min },
        { &tb_libplacebo_tonemap_dst_max,              LB_TB_LIBPLACEBO_TONEMAP_DST_MAX,              ID_TB_LIBPLACEBO_TONEMAP_DST_MAX,              1,  2000,  203, &cl_exdata.libplacebo_tonemap_dst_max },
        { &tb_libplacebo_tonemap_dst_min,              LB_TB_LIBPLACEBO_TONEMAP_DST_MIN,              ID_TB_LIBPLACEBO_TONEMAP_DST_MIN,              0,  2000,    1, &cl_exdata.libplacebo_tonemap_dst_min },
        { &tb_libplacebo_tonemap_smooth_period,        LB_TB_LIBPLACEBO_TONEMAP_SMOOTH_PERIOD,        ID_TB_LIBPLACEBO_TONEMAP_SMOOTH_PERIOD,        0,  1000,   20, &cl_exdata.libplacebo_tonemap_smooth_period },
        { &tb_libplacebo_tonemap_scene_threshold_low,  LB_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_LOW,  ID_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_LOW,  0,  1000,   10, &cl_exdata.libplacebo_tonemap_scene_threshold_low },
        { &tb_libplacebo_tonemap_scene_threshold_high, LB_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_HIGH, ID_TB_LIBPLACEBO_TONEMAP_SCENE_THRESHOLD_HIGH, 0,  1000,   30, &cl_exdata.libplacebo_tonemap_scene_threshold_high },
        //{ &tb_libplacebo_tonemap_percentile,           LB_TB_LIBPLACEBO_TONEMAP_PERCENTILE,           ID_TB_LIBPLACEBO_TONEMAP_PERCENTILE,           0,  100,     1, &cl_exdata.libplacebo_tonemap_percentile },
        { &tb_libplacebo_tonemap_black_cutoff,         LB_TB_LIBPLACEBO_TONEMAP_BLACK_CUTOFF,         ID_TB_LIBPLACEBO_TONEMAP_BLACK_CUTOFF,         0,  100,     1, &cl_exdata.libplacebo_tonemap_black_cutoff },
        { &tb_libplacebo_tonemap_contrast_recovery,    LB_TB_LIBPLACEBO_TONEMAP_CONTRAST_RECOVERY,    ID_TB_LIBPLACEBO_TONEMAP_CONTRAST_RECOVERY,    0,  100,     3, &cl_exdata.libplacebo_tonemap_contrast_recovery },
        { &tb_libplacebo_tonemap_contrast_smoothness,  LB_TB_LIBPLACEBO_TONEMAP_CONTRAST_SMOOTHNESS,  ID_TB_LIBPLACEBO_TONEMAP_CONTRAST_SMOOTHNESS,  0,  100,    35, &cl_exdata.libplacebo_tonemap_contrast_smoothness }
    };
    create_trackbars_ex(filter_controls.get(), tb_libplacebo_tonemap, hwnd, hinst, 0, offset_y, track_bar_delta_y, dialog_rc);

    std::vector<CLCX_COMBOBOX> cx_list_libplacebo_tonemap = {
        CLCX_COMBOBOX(cx_libplacebo_tonemap_gamut_mapping,    ID_CX_LIBPLACEBO_TONEMAP_GAMUT_MAPPING,    lb_libplacebo_tonemap_gamut_mapping,    ID_LB_LIBPLACEBO_TONEMAP_GAMUT_MAPPING,    LB_CX_LIBPLACEBO_TONEMAP_GAMUT_MAPPING,    list_vpp_libplacebo_tone_mapping_gamut_mapping),
        CLCX_COMBOBOX(cx_libplacebo_tonemap_metadata,         ID_CX_LIBPLACEBO_TONEMAP_METADATA,         lb_libplacebo_tonemap_metadata,         ID_LB_LIBPLACEBO_TONEMAP_METADATA,         LB_CX_LIBPLACEBO_TONEMAP_METADATA,         list_vpp_libplacebo_tone_mapping_metadata),
        CLCX_COMBOBOX(cx_libplacebo_tonemap_dst_pl_transfer,  ID_CX_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER,  lb_libplacebo_tonemap_dst_pl_transfer,  ID_LB_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER,  LB_CX_LIBPLACEBO_TONEMAP_DST_PL_TRANSFER,  list_vpp_libplacebo_tone_mapping_transfer),
        CLCX_COMBOBOX(cx_libplacebo_tonemap_dst_pl_colorprim, ID_CX_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM, lb_libplacebo_tonemap_dst_pl_colorprim, ID_LB_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM, LB_CX_LIBPLACEBO_TONEMAP_DST_PL_COLORPRIM, list_vpp_libplacebo_tone_mapping_colorprim),
        CLCX_COMBOBOX(cx_libplacebo_tonemap_function,         ID_CX_LIBPLACEBO_TONEMAP_FUNCTION,         lb_libplacebo_tonemap_function,         ID_LB_LIBPLACEBO_TONEMAP_FUNCTION,         LB_CX_LIBPLACEBO_TONEMAP_FUNCTION,         list_vpp_libplacebo_tone_mapping_function)
    };
    cx_y_pos = offset_y + 2;                // すこし窮屈なので +2pix
    offset_y += CX_HEIGHT * cx_list_libplacebo_tonemap.size() + 4; // すこし窮屈なので +4pix
    add_combobox(filter_controls.get(), cx_list_libplacebo_tonemap, cx_y_pos, b_font, hwnd, hinst);

    //tonemap_functionによって変化するトラックバー
    const int tone_const_param_y = offset_y;

    // st2094 & spline
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_tonemap_tone_const_knee = {
        // st2094
        { &tb_libplacebo_tonemap_tone_const_knee_adaptation, LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_ADAPTATION, ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_ADAPTATION, 0, 100, 40, &cl_exdata.libplacebo_tonemap_tone_const_knee_adaptation },
        { &tb_libplacebo_tonemap_tone_const_knee_min,        LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MIN,        ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MIN,        0,  50, 10, &cl_exdata.libplacebo_tonemap_tone_const_knee_min },
        { &tb_libplacebo_tonemap_tone_const_knee_max,        LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MAX,        ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_MAX,       50, 100, 80, &cl_exdata.libplacebo_tonemap_tone_const_knee_max },
        { &tb_libplacebo_tonemap_tone_const_knee_default,    LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_DEFAULT,    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_DEFAULT,    0, 100, 40, &cl_exdata.libplacebo_tonemap_tone_const_knee_default },
        // spline
        { &tb_libplacebo_tonemap_tone_const_slope_tuning,       LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_TUNING,       ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_TUNING,       0, 1000, 150, &cl_exdata.libplacebo_tonemap_tone_const_slope_tuning },
        { &tb_libplacebo_tonemap_tone_const_slope_offset,       LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_OFFSET,       ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SLOPE_OFFSET,       0,  100,  20, &cl_exdata.libplacebo_tonemap_tone_const_slope_offset },
        { &tb_libplacebo_tonemap_tone_const_spline_contrast,    LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SPLINE_CONTRAST,    ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_SPLINE_CONTRAST,    0,  150,  50, &cl_exdata.libplacebo_tonemap_tone_const_spline_contrast }
    };
    cx_y_pos = tone_const_param_y;
    create_trackbars_ex(filter_controls.get(), tb_libplacebo_tonemap_tone_const_knee, hwnd, hinst, 0, cx_y_pos, track_bar_delta_y, dialog_rc);
    offset_y = std::max(offset_y, cx_y_pos);

    // For bt2390
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_tonemap_bt2390 = {
        { &tb_libplacebo_tonemap_tone_const_knee_offset, LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_OFFSET, ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_KNEE_OFFSET, 50, 200, 100, &cl_exdata.libplacebo_tonemap_tone_const_knee_offset }
    };
    cx_y_pos = tone_const_param_y;
    create_trackbars_ex(filter_controls.get(), tb_libplacebo_tonemap_bt2390, hwnd, hinst, 0, cx_y_pos, track_bar_delta_y, dialog_rc);
    offset_y = std::max(offset_y, cx_y_pos);

    // For reinhard
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_tonemap_reinhard = {
        { &tb_libplacebo_tonemap_tone_const_reinhard_contrast, LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_REINHARD_CONTRAST, ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_REINHARD_CONTRAST, 0, 100, 50, &cl_exdata.libplacebo_tonemap_tone_const_reinhard_contrast }
    };
    cx_y_pos = tone_const_param_y;
    create_trackbars_ex(filter_controls.get(), tb_libplacebo_tonemap_reinhard, hwnd, hinst, 0, cx_y_pos, track_bar_delta_y, dialog_rc);
    offset_y = std::max(offset_y, cx_y_pos);

    // For mobius
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_tonemap_mobius = {
        { &tb_libplacebo_tonemap_tone_const_linear_knee, LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_LINEAR_KNEE, ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_LINEAR_KNEE, 0, 100, 30, &cl_exdata.libplacebo_tonemap_tone_const_linear_knee }
    };
    cx_y_pos = tone_const_param_y;
    create_trackbars_ex(filter_controls.get(), tb_libplacebo_tonemap_mobius, hwnd, hinst, 0, cx_y_pos, track_bar_delta_y, dialog_rc);
    offset_y = std::max(offset_y, cx_y_pos);

    // For linear
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_tonemap_linear = {
        { &tb_libplacebo_tonemap_tone_const_exposure, LB_TB_LIBPLACEBO_TONEMAP_TONE_CONST_EXPOSURE, ID_TB_LIBPLACEBO_TONEMAP_TONE_CONST_EXPOSURE, 0, 1000, 100, &cl_exdata.libplacebo_tonemap_tone_const_exposure }
    };
    cx_y_pos = tone_const_param_y;
    create_trackbars_ex(filter_controls.get(), tb_libplacebo_tonemap_linear, hwnd, hinst, 0, cx_y_pos, track_bar_delta_y, dialog_rc);
    offset_y = std::max(offset_y, cx_y_pos);

    offset_y += track_bar_delta_y;
    filter_controls->set_y_size(offset_y);
    return filter_controls;
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

    const int offset_width = -8; // ウィンドウの幅を少し狭くするためのoffset
    const int columns = 4;
    const int col_width = dialog_rc.right - dialog_rc.left + offset_width;
    g_col_width = col_width;

    //clfilterのチェックボックス
    GetWindowRect(child_hwnd[0], &rc);
    SetWindowPos(child_hwnd[0], HWND_TOP, rc.left - dialog_rc.left + offset_width + (columns-1) * col_width - AVIUTL_1_10_OFFSET, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

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
    int y_pos_max = 0;

    //リサイズ
    int col = 0;
    int y_pos = cb_row_start_y_pos;
    auto filter_resize = create_resize(track_bar_delta_y, checkbox_idx, dialog_rc, b_font, hwnd, hinst, fp);
    filter_resize->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_resize));

    //colorspace
    auto filter_colorspace = create_colorspace(CLFILTER_CHECK_COLORSPACE_ENABLE, CLFILTER_CHECK_COLORSPACE_MAX, CLFILTER_TRACK_COLORSPACE_FIRST, CLFILTER_TRACK_COLORSPACE_MAX, track_bar_delta_y, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_colorspace->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_colorspace));

    //libplacebo-tonemap
    auto filter_libplacebo_tonemap = create_libplacebo_tonemapping(track_bar_delta_y, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_libplacebo_tonemap->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_libplacebo_tonemap));

    //nnedi
    std::vector<CLCX_COMBOBOX> cx_list_nnedi = {
        CLCX_COMBOBOX(cx_nnedi_field,     ID_CX_NNEDI_FIELD,     lb_nnedi_field,     ID_LB_NNEDI_FIELD,     LB_CX_NNEDI_FIELD,     list_vpp_nnedi_field+2, 2),
        CLCX_COMBOBOX(cx_nnedi_nns,       ID_CX_NNEDI_NNS,       lb_nnedi_nns,       ID_LB_NNEDI_NNS,       LB_CX_NNEDI_NNS,       list_vpp_nnedi_nns),
        CLCX_COMBOBOX(cx_nnedi_nsize,     ID_CX_NNEDI_NSIZE,     lb_nnedi_nsize,     ID_LB_NNEDI_NSIZE,     LB_CX_NNEDI_NSIZE,     list_vpp_nnedi_nsize),
        CLCX_COMBOBOX(cx_nnedi_quality,   ID_CX_NNEDI_QUALITY,   lb_nnedi_quality,   ID_LB_NNEDI_QUALITY,   LB_CX_NNEDI_QUALITY,   list_vpp_nnedi_quality),
        CLCX_COMBOBOX(cx_nnedi_prescreen, ID_CX_NNEDI_PRESCREEN, lb_nnedi_prescreen, ID_LB_NNEDI_PRESCREEN, LB_CX_NNEDI_PRESCREEN, list_vpp_nnedi_pre_screen),
        CLCX_COMBOBOX(cx_nnedi_errortype, ID_CX_NNEDI_ERRORTYPE, lb_nnedi_errortype, ID_LB_NNEDI_ERRORTYPE, LB_CX_NNEDI_ERRORTYPE, list_vpp_nnedi_error_type)
    };
    auto filter_nnedi = create_group(CLFILTER_CHECK_NNEDI_ENABLE, CLFILTER_CHECK_NNEDI_MAX, CLFILTER_TRACK_NNEDI_FIRST, CLFILTER_TRACK_NNEDI_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_nnedi, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_nnedi->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_nnedi));

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
    std::vector<CLCX_COMBOBOX> cx_list_nvvfx_denoise = {
        CLCX_COMBOBOX(cx_nvvfx_denoise_strength, ID_CX_NVVFX_DENOISE_STRENGTH, lb_nvvfx_denoise_strength, ID_LB_NVVFX_DENOISE_STRENGTH, LB_CX_NVVFX_DENOISE_STRENGTH, list_vpp_nvvfx_denoise_strength)
    };
    auto filter_nvvfx_denoise = create_group(CLFILTER_CHECK_NVVFX_DENOISE_ENABLE, CLFILTER_CHECK_NVVFX_DENOISE_MAX, CLFILTER_TRACK_NVVFX_DENOISE_FIRST, CLFILTER_TRACK_NVVFX_DENOISE_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_nvvfx_denoise, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_nvvfx_denoise->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_nvvfx_denoise));

    //nvvfx-artifact-reduction
    const CX_DESC list_vpp_nvvfx_artifact_reduction_mode[] = {
        { _T("0 - light"),  0 },
        { _T("1 - strong"), 1 },
        { NULL, 0 }
    };
    std::vector<CLCX_COMBOBOX> cx_list_nvvfx_artifact_reduction = {
        CLCX_COMBOBOX(cx_nvvfx_artifact_reduction_mode, ID_CX_NVVFX_ARTIFACT_REDUCTION_MODE, lb_nvvfx_artifact_reduction_mode, ID_LB_NVVFX_ARTIFACT_REDUCTION_MODE, LB_CX_NVVFX_ARTIFACT_REDUCTION_MODE, list_vpp_nvvfx_artifact_reduction_mode)
    };
    auto filter_nvvfx_artifact_reduction = create_group(CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_ENABLE, CLFILTER_CHECK_NVVFX_ARTIFACT_REDUCTION_MAX, CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_FIRST, CLFILTER_TRACK_NVVFX_ARTIFACT_REDUCTION_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_nvvfx_artifact_reduction, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_nvvfx_artifact_reduction->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_nvvfx_artifact_reduction));

    //denoise-dct
    std::vector<CLCX_COMBOBOX> cx_list_denoise_dct = {
        CLCX_COMBOBOX(cx_denoise_dct_step,       ID_CX_DENOISE_DCT_STEP,       lb_denoise_dct_step,       ID_LB_DENOISE_DCT_STEP,       LB_CX_DENOISE_DCT_STEP,       list_vpp_denoise_dct_step),
        CLCX_COMBOBOX(cx_denoise_dct_block_size, ID_CX_DENOISE_DCT_BLOCK_SIZE, lb_denoise_dct_block_size, ID_LB_DENOISE_DCT_BLOCK_SIZE, LB_CX_DENOISE_DCT_BLOCK_SIZE, list_vpp_denoise_dct_block_size)
    };
    auto filter_denoise_dct = create_group(CLFILTER_CHECK_DENOISE_DCT_ENABLE, CLFILTER_CHECK_DENOISE_DCT_MAX, CLFILTER_TRACK_DENOISE_DCT_FIRST, CLFILTER_TRACK_DENOISE_DCT_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_denoise_dct, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_denoise_dct->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_denoise_dct));

    //smooth
    std::vector<CLCX_COMBOBOX> cx_list_smooth = {
        CLCX_COMBOBOX(cx_smooth_quality, ID_CX_SMOOTH_QUALITY, lb_smooth_quality, ID_LB_SMOOTH_QUALITY, LB_CX_SMOOTH_QUALITY, list_vpp_smooth_quality)
    };
    auto filter_smooth = create_group(CLFILTER_CHECK_SMOOTH_ENABLE, CLFILTER_CHECK_SMOOTH_MAX, CLFILTER_TRACK_SMOOTH_FIRST, CLFILTER_TRACK_SMOOTH_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_smooth, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_smooth->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_smooth));

    //knn
    std::vector<CLCX_COMBOBOX> cx_list_knn = {
        CLCX_COMBOBOX(cx_knn_radius, ID_CX_KNN_RADIUS, lb_knn_radius, ID_LB_KNN_RADIUS, LB_CX_KNN_RADIUS, list_vpp_raduis)
    };
    auto filter_knn = create_group(CLFILTER_CHECK_KNN_ENABLE, CLFILTER_CHECK_KNN_MAX, CLFILTER_TRACK_KNN_FIRST, CLFILTER_TRACK_KNN_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_knn, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_knn->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_knn));

    //nlmeans
    std::vector<CLCX_COMBOBOX> cx_list_nlmeans = {
        CLCX_COMBOBOX(cx_nlmeans_patch,  ID_CX_DENOISE_NLMEANS_PATCH,  lb_nlmeans_patch,  ID_LB_DENOISE_NLMEANS_PATCH,  LB_CX_DENOISE_NLMEANS_PATCH,  list_vpp_nlmeans_block_size),
        CLCX_COMBOBOX(cx_nlmeans_search, ID_CX_DENOISE_NLMEANS_SEARCH, lb_nlmeans_search, ID_LB_DENOISE_NLMEANS_SEARCH, LB_CX_DENOISE_NLMEANS_SEARCH, list_vpp_nlmeans_block_size)
    };
    auto filter_nlmeans = create_group(CLFILTER_CHECK_NLMEANS_ENABLE, CLFILTER_CHECK_NLMEANS_MAX, CLFILTER_TRACK_NLMEANS_FIRST, CLFILTER_TRACK_NLMEANS_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_nlmeans, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_nlmeans->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_nlmeans));

    //pmd
    std::vector<CLCX_COMBOBOX> cx_list_pmd = { };
    auto filter_pmd = create_group(CLFILTER_CHECK_PMD_ENABLE, CLFILTER_CHECK_PMD_MAX, CLFILTER_TRACK_PMD_FIRST, CLFILTER_TRACK_PMD_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_pmd, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_pmd->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_pmd));

    y_pos_max = std::max(y_pos_max, y_pos);
    // --- 次の列 -----------------------------------------
    col = 2;
    y_pos = cb_row_start_y_pos;
    //unsharp
    std::vector<CLCX_COMBOBOX> cx_list_unsharp = {
        CLCX_COMBOBOX(cx_unsharp_radius, ID_CX_UNSHARP_RADIUS, lb_unsharp_radius, ID_LB_UNSHARP_RADIUS, LB_CX_UNSHARP_RADIUS, list_vpp_raduis)
    };
    auto filter_unsharp = create_group(CLFILTER_CHECK_UNSHARP_ENABLE, CLFILTER_CHECK_UNSHARP_MAX, CLFILTER_TRACK_UNSHARP_FIRST, CLFILTER_TRACK_UNSHARP_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_unsharp, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_unsharp->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_unsharp));

    //エッジレベル調整
    std::vector<CLCX_COMBOBOX> cx_list_edgelevel = { };
    auto filter_edgelevel = create_group(CLFILTER_CHECK_EDGELEVEL_ENABLE, CLFILTER_CHECK_EDGELEVEL_MAX, CLFILTER_TRACK_EDGELEVEL_FIRST, CLFILTER_TRACK_EDGELEVEL_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_edgelevel, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_edgelevel->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_edgelevel));

    //warpsharp
    std::vector<CLCX_COMBOBOX> cx_list_warpsharp = {
        CLCX_COMBOBOX(cx_warpsharp_blur, ID_CX_WARPSHARP_BLUR, lb_warpsharp_blur, ID_LB_WARPSHARP_BLUR, LB_CX_WARPSHARP_BLUR, list_vpp_1_to_10)
    };
    auto filter_warpsharp = create_group(CLFILTER_CHECK_WARPSHARP_ENABLE, CLFILTER_CHECK_WARPSHARP_MAX, CLFILTER_TRACK_WARPSHARP_FIRST, CLFILTER_TRACK_WARPSHARP_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_warpsharp, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_warpsharp->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_warpsharp));

    //tweak
    std::vector<CLCX_COMBOBOX> cx_list_tweak = { };
    auto filter_tweak = create_group(CLFILTER_CHECK_TWEAK_ENABLE, CLFILTER_CHECK_TWEAK_MAX, CLFILTER_TRACK_TWEAK_FIRST, CLFILTER_TRACK_TWEAK_MAX, track_bar_delta_y, {}, ADD_CX_FIRST, cx_list_tweak, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_tweak->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_tweak));

    //バンディング
    std::vector<CLCX_COMBOBOX> cx_list_deband = {
        CLCX_COMBOBOX(cx_deband_sample, ID_CX_DEBAND_SAMPLE, lb_deband_sample, ID_LB_DEBAND_SAMPLE, LB_CX_DEBAND_SAMPLE, list_vpp_deband_sample)
    };
    auto fitler_deband = create_group(CLFILTER_CHECK_DEBAND_ENABLE, CLFILTER_CHECK_DEBAND_MAX, CLFILTER_TRACK_DEBAND_FIRST, CLFILTER_TRACK_DEBAND_MAX, track_bar_delta_y, {}, ADD_CX_AFTER_TRACK, cx_list_deband, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    fitler_deband->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(fitler_deband));

    //libplacebo-deband
    std::vector<CLCX_COMBOBOX> cx_list_libplacebo_deband = {
        CLCX_COMBOBOX(cx_libplacebo_deband_dither, ID_CX_LIBPLACEBO_DEBAND_DITHER, lb_libplacebo_deband_dither, ID_LB_LIBPLACEBO_DEBAND_DITHER, LB_CX_LIBPLACEBO_DEBAND_DITHER, list_vpp_libplacebo_deband_dither_mode),
        CLCX_COMBOBOX(cx_libplacebo_deband_lut_size, ID_CX_LIBPLACEBO_DEBAND_LUT_SIZE, lb_libplacebo_deband_lut_size, ID_LB_LIBPLACEBO_DEBAND_LUT_SIZE, LB_CX_LIBPLACEBO_DEBAND_LUT_SIZE, list_vpp_libplacebo_deband_lut_size)
    };
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_libplacebo_deband = {
        { &tb_libplacebo_deband_iterations, LB_TB_LIBPLACEBO_DEBAND_ITERATIONS, ID_TB_LIBPLACEBO_DEBAND_ITERATIONS, 1,  16,  1, &cl_exdata.libplacebo_deband_iterations },
        { &tb_libplacebo_deband_threshold,  LB_TB_LIBPLACEBO_DEBAND_THRESHOLD,  ID_TB_LIBPLACEBO_DEBAND_THRESHOLD,  0, 500, 40, &cl_exdata.libplacebo_deband_threshold  },
        { &tb_libplacebo_deband_radius,     LB_TB_LIBPLACEBO_DEBAND_RADIUS,     ID_TB_LIBPLACEBO_DEBAND_RADIUS,     1, 128, 16, &cl_exdata.libplacebo_deband_radius     },
        { &tb_libplacebo_deband_grain,      LB_TB_LIBPLACEBO_DEBAND_GRAIN,      ID_TB_LIBPLACEBO_DEBAND_GRAIN,      0, 255,  6, &cl_exdata.libplacebo_deband_grain      }
    };
    auto filter_libplacebo_deband = create_group(CLFILTER_CHECK_LIBPLACEBO_DEBAND_ENABLE, CLFILTER_CHECK_LIBPLACEBO_DEBAND_MAX, CLFILTER_TRACK_LIBPLACEBO_DEBAND_FIRST, CLFILTER_TRACK_LIBPLACEBO_DEBAND_MAX, track_bar_delta_y, tb_libplacebo_deband, ADD_CX_AFTER_TRACK, cx_list_libplacebo_deband, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_libplacebo_deband->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_libplacebo_deband));

    //TrueHDR
    std::vector<CLCX_COMBOBOX> cx_list_ngx_truehdr = { };
    const std::vector<CLFILTER_TRACKBAR_DATA> tb_truehdr = {
        { &tb_ngx_truehdr_contrast,     LB_TB_TRUEHDR_CONTRAST,     ID_TB_NGX_TRUEHDR_CONTRAST,       0,  200,  100, &cl_exdata.ngx_truehdr_contrast     },
        { &tb_ngx_truehdr_saturation,   LB_TB_TRUEHDR_SATURATION,   ID_TB_NGX_TRUEHDR_SATURATION,     0,  200,  100, &cl_exdata.ngx_truehdr_saturation   },
        { &tb_ngx_truehdr_middlegray,   LB_TB_TRUEHDR_MIDDLEGRAY,   ID_TB_NGX_TRUEHDR_MIDDLEGRAY,    10,  100,   50, &cl_exdata.ngx_truehdr_middlegray   },
        { &tb_ngx_truehdr_maxluminance, LB_TB_TRUEHDR_MAXLUMINANCE, ID_TB_NGX_TRUEHDR_MAXLUMINANCE, 400, 2000, 1000, &cl_exdata.ngx_truehdr_maxluminance }
    };
    auto filter_ngx_truhdr = create_group(CLFILTER_CHECK_TRUEHDR_ENABLE, CLFILTER_CHECK_TRUEHDR_MAX, CLFILTER_TRACK_NGX_TRUEHDR_FIRST, CLFILTER_TRACK_NGX_TRUEHDR_MAX, track_bar_delta_y, tb_truehdr, ADD_CX_AFTER_TRACK, cx_list_ngx_truehdr, checkbox_idx, dialog_rc, b_font, hwnd, hinst);
    filter_ngx_truhdr->move_group(y_pos, col, col_width);
    g_filterControls[col].push_back(std::move(filter_ngx_truhdr));
    
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

    static const int CX_LOG_LEVEL_W = 120;
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
    static const int list_filter_oder_height = 320;
    y_pos = list_filter_order_y + 24 + list_filter_oder_height;
    ls_filter_order = CreateWindow("LISTBOX", "clinfo", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_BORDER | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, list_filter_oder_x, list_filter_order_y + 24, list_filter_oder_width, list_filter_oder_height, hwnd, (HMENU)ID_LS_FILTER_ORDER, hinst, NULL);
    SendMessage(ls_filter_order, WM_SETFONT, (WPARAM)b_font, 0);
    //ls_filter_orderの中身はexdataが転送されてから、init_filter_order_listで初期化する

    bt_filter_order_up = CreateWindow("BUTTON", "↑", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_VCENTER, list_filter_oder_x + list_filter_oder_width + 8, list_filter_order_y + list_filter_oder_height / 2 - 24, bt_filter_order_width, 22, hwnd, (HMENU)ID_BT_FILTER_ORDER_UP, hinst, NULL);
    SendMessage(bt_filter_order_up, WM_SETFONT, (WPARAM)b_font, 0);

    bt_filter_order_down = CreateWindow("BUTTON", "↓", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_VCENTER, list_filter_oder_x + list_filter_oder_width + 8, list_filter_order_y + list_filter_oder_height / 2, bt_filter_order_width, 22, hwnd, (HMENU)ID_BT_FILTER_ORDER_DOWN, hinst, NULL);
    SendMessage(bt_filter_order_down, WM_SETFONT, (WPARAM)b_font, 0);

    g_min_height = std::max(g_min_height, y_pos); // 最小の高さとしてこの列の高さを登録する
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

    //libplacebo-tonemap
    prm.vpp.libplacebo_tonemapping.enable = fp->check[CLFILTER_CHECK_LIBPLACEBO_TONEMAP_ENABLE] != 0;
    prm.vpp.libplacebo_tonemapping.src_csp = (VppLibplaceboToneMappingCSP)cl_exdata.libplacebo_tonemap_src_csp;
    prm.vpp.libplacebo_tonemapping.dst_csp = (VppLibplaceboToneMappingCSP)cl_exdata.libplacebo_tonemap_dst_csp;
    prm.vpp.libplacebo_tonemapping.src_max = (float)cl_exdata.libplacebo_tonemap_src_max;
    prm.vpp.libplacebo_tonemapping.src_min = (float)cl_exdata.libplacebo_tonemap_src_min;
    prm.vpp.libplacebo_tonemapping.dst_max = (float)cl_exdata.libplacebo_tonemap_dst_max;
    prm.vpp.libplacebo_tonemapping.dst_min = (float)cl_exdata.libplacebo_tonemap_dst_min;
    prm.vpp.libplacebo_tonemapping.smooth_period = (float)cl_exdata.libplacebo_tonemap_smooth_period;
    prm.vpp.libplacebo_tonemapping.scene_threshold_low = (float)cl_exdata.libplacebo_tonemap_scene_threshold_low * 0.1f;
    prm.vpp.libplacebo_tonemapping.scene_threshold_high = (float)cl_exdata.libplacebo_tonemap_scene_threshold_high * 0.1f;
    //prm.vpp.libplacebo_tonemapping.percentile = (float)cl_exdata.libplacebo_tonemap_percentile;
    prm.vpp.libplacebo_tonemapping.black_cutoff = (float)cl_exdata.libplacebo_tonemap_black_cutoff;
    prm.vpp.libplacebo_tonemapping.gamut_mapping = (VppLibplaceboToneMappingGamutMapping)cl_exdata.libplacebo_tonemap_gamut_mapping;
    prm.vpp.libplacebo_tonemapping.tonemapping_function = (VppLibplaceboToneMappingFunction)cl_exdata.libplacebo_tonemap_function;
    prm.vpp.libplacebo_tonemapping.metadata = (VppLibplaceboToneMappingMetadata)cl_exdata.libplacebo_tonemap_metadata;
    prm.vpp.libplacebo_tonemapping.contrast_recovery = (float)cl_exdata.libplacebo_tonemap_contrast_recovery * 0.1f;
    prm.vpp.libplacebo_tonemapping.contrast_smoothness = (float)cl_exdata.libplacebo_tonemap_contrast_smoothness * 0.1f;
    prm.vpp.libplacebo_tonemapping.dst_pl_transfer = (VppLibplaceboToneMappingTransfer)cl_exdata.libplacebo_tonemap_dst_pl_transfer;
    prm.vpp.libplacebo_tonemapping.dst_pl_colorprim = (VppLibplaceboToneMappingColorprim)cl_exdata.libplacebo_tonemap_dst_pl_colorprim;

    prm.vpp.libplacebo_tonemapping.tone_constants.st2094.knee_adaptation = cl_exdata.libplacebo_tonemap_tone_const_knee_adaptation * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.st2094.knee_min = cl_exdata.libplacebo_tonemap_tone_const_knee_min * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.st2094.knee_max = cl_exdata.libplacebo_tonemap_tone_const_knee_max * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.st2094.knee_default = cl_exdata.libplacebo_tonemap_tone_const_knee_default * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.bt2390.knee_offset = cl_exdata.libplacebo_tonemap_tone_const_knee_offset * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.spline.slope_tuning = cl_exdata.libplacebo_tonemap_tone_const_slope_tuning * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.spline.slope_offset = cl_exdata.libplacebo_tonemap_tone_const_slope_offset * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.spline.spline_contrast = cl_exdata.libplacebo_tonemap_tone_const_spline_contrast * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.reinhard.contrast = cl_exdata.libplacebo_tonemap_tone_const_reinhard_contrast * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.mobius.linear_knee = cl_exdata.libplacebo_tonemap_tone_const_linear_knee * 0.01f;
    prm.vpp.libplacebo_tonemapping.tone_constants.linear.exposure = cl_exdata.libplacebo_tonemap_tone_const_exposure * 0.01f;

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

    //libplacebo(resize)
    prm.vpp.resize_libplacebo.clamp_ = (float)cl_exdata.resize_pl_clamp * 0.01f;
    prm.vpp.resize_libplacebo.taper = (float)cl_exdata.resize_pl_taper * 0.01f;
    prm.vpp.resize_libplacebo.blur = (float)cl_exdata.resize_pl_blur;
    prm.vpp.resize_libplacebo.antiring = (float)cl_exdata.resize_pl_antiring * 0.01f;

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

    //libplacebo-deband
    prm.vpp.libplacebo_deband.enable = fp->check[CLFILTER_CHECK_LIBPLACEBO_DEBAND_ENABLE] != 0;
    prm.vpp.libplacebo_deband.iterations = cl_exdata.libplacebo_deband_iterations;
    prm.vpp.libplacebo_deband.threshold = cl_exdata.libplacebo_deband_threshold * 0.1f;
    prm.vpp.libplacebo_deband.radius = (float)cl_exdata.libplacebo_deband_radius;
    prm.vpp.libplacebo_deband.grainY = (float)cl_exdata.libplacebo_deband_grain;
    prm.vpp.libplacebo_deband.grainC = (float)cl_exdata.libplacebo_deband_grain;
    prm.vpp.libplacebo_deband.dither = (VppLibplaceboDebandDitherMode)cl_exdata.libplacebo_deband_dither;
    prm.vpp.libplacebo_deband.lut_size = cl_exdata.libplacebo_deband_lut_size;

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
    m_log->write(RGY_LOG_TRACE, RGY_LOGT_CORE, "auf: currentFrameId: %d, frameIn: %d, frameInFin: %d, frameProc %d, frameOut %d, reset %d, %s\n",
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
        m_log->write(RGY_LOG_TRACE, RGY_LOGT_CORE, "auf:   set frame In: srcFrame[%d]=%d - m_sharedFrames[%d]\n",
            i, sharedPrms->srcFrame[i].frameId, sharedPrms->currentFrameId % m_sharedFrames.size());

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
    m_log->write(RGY_LOG_TRACE, RGY_LOGT_CORE, "auf:   get frame Out: m_sharedFrames[%d] -> %d\n",
        (sharedPrms->currentFrameId + 1) % m_sharedFrames.size(), sharedPrms->currentFrameId);
    return TRUE;
}
