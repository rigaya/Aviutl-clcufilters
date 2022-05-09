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
#include <process.h>
#include <algorithm>
#include <vector>
#include <cstdint>

#define DEFINE_GLOBAL
#include "filter.h"
#include "clfilters_version.h"
#include "clfilters.h"
#include "clfilters_chain.h"
#include "rgy_prm.h"

void init_dialog(HWND hwnd, FILTER *fp);
void update_cx(FILTER *fp);

#define ENABLE_FIELD (0)
#define ENABLE_HDR2SDR_DESAT (0)

enum {
    ID_LB_RESIZE_RES = ID_TX_RESIZE_RES_ADD+1,
    ID_CX_RESIZE_RES,
    ID_BT_RESIZE_RES_ADD,
    ID_BT_RESIZE_RES_DEL,
    ID_CX_RESIZE_ALGO,

    ID_LB_OPENCL_DEVICE,
    ID_CX_OPENCL_DEVICE,
    ID_BT_OPENCL_INFO,

    ID_LB_LOG_LEVEL,
    ID_CX_LOG_LEVEL,

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
};

#pragma pack(1)
union CL_PLATFORM_DEVICE {
    struct {
        int16_t platform;
        int16_t device;
    } s;
    int i;
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

    char reserved[900];
};
# pragma pack()
static const size_t exdatasize = sizeof(CLFILTER_EXDATA);
static_assert(exdatasize == 1024);

static CLFILTER_EXDATA cl_exdata;
static std::unique_ptr<clFilterChain> clfilter;
static std::vector<std::shared_ptr<RGYOpenCLPlatform>> clplatforms;
static BOOL is_saving;


static void cl_exdata_set_default() {
    cl_exdata.cl_dev_id.i = 0;
    cl_exdata.log_level = RGY_LOG_QUIET;

    cl_exdata.resize_idx = 0;
    cl_exdata.resize_algo = RGY_VPP_RESIZE_SPLINE36;

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

    VppSmooth smooth;
    cl_exdata.smooth_quality = smooth.quality;

    VppKnn knn;
    cl_exdata.knn_radius = knn.radius;

    VppPmd pmd;
    cl_exdata.pmd_apply_count = pmd.applyCount;

    VppUnsharp unsharp;
    cl_exdata.unsharp_radius = unsharp.radius;

    VppWarpsharp warpsharp;
    cl_exdata.warpsharp_blur = warpsharp.blur;

    VppDeband deband;
    cl_exdata.deband_sample = deband.sample;
}

//---------------------------------------------------------------------
//        フィルタ構造体定義
//---------------------------------------------------------------------

//  トラックバーの名前
const TCHAR *track_name[] = {
    //リサイズ
    "入力ピーク輝度", "目標輝度", //colorspace
#if ENABLE_HDR2SDR_DESAT
    "脱飽和ｵﾌｾｯﾄ", "脱飽和強度", "脱飽和指数", //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    "QP", // smooth
    "強さ", "ブレンド度合い", "ブレンド閾値", //knn
    "適用回数", "強さ", "閾値", //pmd
    "強さ", "閾値", //unsharp
    "特性", "閾値", "黒", "白", //エッジレベル調整
    "閾値", "深度", //warpsharp
    "輝度", "コントラスト", "ガンマ", "彩度", "色相", //tweak
    "range", "Y", "C", "ditherY", "ditherC" //バンディング低減
};

enum {
    CLFILTER_TRACK_COLORSPACE_FIRST = 0,
    CLFILTER_TRACK_COLORSPACE_SOURCE_PEAK = CLFILTER_TRACK_COLORSPACE_FIRST,
    CLFILTER_TRACK_COLORSPACE_LDR_NITS,
#if ENABLE_HDR2SDR_DESAT
    CLFILTER_TRACK_COLORSPACE_DESAT_BASE,
    CLFILTER_TRACK_COLORSPACE_DESAT_STRENGTH,
    CLFILTER_TRACK_COLORSPACE_DESAT_EXP,
#endif //#if ENABLE_HDR2SDR_DESAT
    CLFILTER_TRACK_COLORSPACE_MAX,

    CLFILTER_TRACK_SMOOTH_FIRST = CLFILTER_TRACK_COLORSPACE_MAX,
    CLFILTER_TRACK_SMOOTH_QP = CLFILTER_TRACK_SMOOTH_FIRST,
    CLFILTER_TRACK_SMOOTH_MAX,

    CLFILTER_TRACK_KNN_FIRST = CLFILTER_TRACK_SMOOTH_MAX,
    CLFILTER_TRACK_KNN_STRENGTH = CLFILTER_TRACK_KNN_FIRST,
    CLFILTER_TRACK_KNN_LERP,
    CLFILTER_TRACK_KNN_TH_LERP,
    CLFILTER_TRACK_KNN_MAX,

    CLFILTER_TRACK_PMD_FIRST = CLFILTER_TRACK_KNN_MAX,
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

    CLFILTER_TRACK_MAX = CLFILTER_TRACK_NNEDI_MAX,
};

//  トラックバーの初期値
int track_default[] = {
    1000, 100, //colorspace
#if ENABLE_HDR2SDR_DESAT
    18, 75, 15, //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    12, //smooth
    8, 20, 80, //knn
    2, 100, 100, //pmd
    5, 10, //unsharp
    5, 20, 0, 0, //エッジレベル調整
    128, 16, //warpsharp
    0, 100, 100, 100, 0, //tweak
    15, 15, 15, 15, 15 //バンディング低減
};
//  トラックバーの下限値
int track_s[] = {
    0, 0, //colorspace
#if ENABLE_HDR2SDR_DESAT
    0, 0, 1, //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    1, //smooth
    0,0,0, //knn
    1,0,0, //pmd
    0,0, //unsharp
    -31,0,0,0, //エッジレベル調整
    0, -128, //warpsharp
    -100,-200,1,0,-180, //tweak
    0,0,0,0,0 //バンディング低減
};
//  トラックバーの上限値
int track_e[] = {
    5000, 1000, //colorspace
#if ENABLE_HDR2SDR_DESAT
    100, 100, 30, //colorspace
#endif //#if ENABLE_HDR2SDR_DESAT
    63, //smooth
    100, 100, 100, //knn
    10, 100, 255, //pmd
    100, 255, //unsharp
    31, 255, 31, 31, //エッジレベル調整
    255, 128, //warpsharp
    100,200,200,200,180, //tweak
    127,31,31,31,31//バンディング低減
};

//  トラックバーの数
#define    TRACK_N    (_countof(track_name))
static_assert(TRACK_N <= 32, "TRACK_N check");
static_assert(TRACK_N == CLFILTER_TRACK_MAX, "TRACK_N check");
static_assert(TRACK_N == _countof(track_default), "track_default check");
static_assert(TRACK_N == _countof(track_s), "track_s check");
static_assert(TRACK_N == _countof(track_e), "track_e check");

//  チェックボックスの名前
const TCHAR *check_name[] = {
#if ENABLE_FIELD
    "フィールド処理",
#endif //#if ENABLE_FIELD
    "リサイズ",
    "色空間変換", "matrix", "colorprim", "transfer", "range",
    "ノイズ除去 (smooth)",
    "ノイズ除去 (knn)",
    "ノイズ除去 (pmd)",
    "unsharp",
    "エッジレベル調整",
    "warpsharp", "マスクサイズ off:13x13, on:5x5", "色差マスク",
    "色調補正",
    "バンディング低減", "ブラー処理を先に", "毎フレーム乱数を生成",
    "nnedi"
};

enum {
#if ENABLE_FIELD
    CLFILTER_CHECK_FIELD,
#endif //#if ENABLE_FIELD
    CLFILTER_CHECK_RESIZE_ENABLE,
    CLFILTER_CHECK_RESIZE_MAX,

    CLFILTER_CHECK_COLORSPACE_ENABLE = CLFILTER_CHECK_RESIZE_MAX,
    CLFILTER_CHECK_COLORSPACE_MATRIX_ENABLE,
    CLFILTER_CHECK_COLORSPACE_COLORPRIM_ENABLE,
    CLFILTER_CHECK_COLORSPACE_TRANSFER_ENABLE,
    CLFILTER_CHECK_COLORSPACE_RANGE_ENABLE,
    CLFILTER_CHECK_COLORSPACE_MAX,

    CLFILTER_CHECK_SMOOTH_ENABLE = CLFILTER_CHECK_COLORSPACE_MAX,
    CLFILTER_CHECK_SMOOTH_MAX,

    CLFILTER_CHECK_KNN_ENABLE = CLFILTER_CHECK_SMOOTH_MAX,
    CLFILTER_CHECK_KNN_MAX,

    CLFILTER_CHECK_PMD_ENABLE = CLFILTER_CHECK_KNN_MAX,
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

    CLFILTER_CHECK_NNEDI_ENABLE = CLFILTER_CHECK_DEBAND_MAX,
    CLFILTER_CHECK_NNEDI_MAX,

    CLFILTER_CHECK_MAX = CLFILTER_CHECK_NNEDI_MAX,
};

//  チェックボックスの初期値 (値は0か1)
int check_default[] = {
#if ENABLE_FIELD
    0, // field
#endif //#if ENABLE_FIELD
    0, // resize
    0, 0, 0, 0, 0, // colorspace
    0, // smooth
    0, // knn
    0, // pmd
    0, // unsharp
    0, // edgelevel
    0, 0, 0, // warpsharp
    0, // tweak
    0, 0, 0, // deband
    0 //nnedi
};
//  チェックボックスの数
#define    CHECK_N    (_countof(check_name))
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
    return &filter;
}

BOOL func_init( FILTER *fp ) {
    return TRUE;
}

BOOL func_exit( FILTER *fp ) {
    clfilter.reset();
    clplatforms.clear();
    return TRUE;
}

BOOL func_update(FILTER* fp, int status) {
    update_cx(fp);
    return TRUE;
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

static HWND lb_knn_radius;
static HWND cx_knn_radius;
static HWND lb_smooth_quality;
static HWND cx_smooth_quality;
static HWND lb_unsharp_radius;
static HWND cx_unsharp_radius;
static HWND lb_warpsharp_blur;
static HWND cx_warpsharp_blur;

static HWND lb_deband_sample;
static HWND cx_deband_sample;


static void set_cl_exdata(const HWND hwnd, const int value) {
    if (hwnd == cx_opencl_device) {
        cl_exdata.cl_dev_id.i = value;
    } else if (hwnd == cx_log_level) {
        cl_exdata.log_level = (RGYLogLevel)value;
    } else if (hwnd == cx_resize_res) {
        cl_exdata.resize_idx = value;
    } else if (hwnd == cx_resize_algo) {
        cl_exdata.resize_algo = value;
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
    } else if (hwnd == cx_knn_radius) {
        cl_exdata.knn_radius = value;
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

static void change_cx_param(HWND hwnd) {
    LRESULT ret;

    // 選択番号取得
    ret = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    ret = SendMessage(hwnd, CB_GETITEMDATA, ret, 0);

    if (ret != CB_ERR) {
        set_cl_exdata(hwnd, ret);
    }
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
    char buffer[1024] = { 0 };
    if (fp->exfunc->dlg_get_save_name(buffer, (LPSTR)".txt", (LPSTR)"clinfo.txt")) {
        const auto str = getOpenCLInfo(CL_DEVICE_TYPE_GPU);
        FILE *fp = fopen(buffer, "w");
        if (fp) {
            fprintf(fp, "%s", str.c_str());
            fclose(fp);
            return TRUE;
        }
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

static void update_cx(FILTER *fp) {
    if (cl_exdata.nnedi_nns == 0) {
        cl_exdata_set_default();
    }
    select_combo_item(cx_opencl_device,               cl_exdata.cl_dev_id.i);
    select_combo_item(cx_log_level,                   cl_exdata.log_level);
    select_combo_item(cx_resize_res,                  cl_exdata.resize_idx);
    select_combo_item(cx_resize_algo,                 cl_exdata.resize_algo);
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
    select_combo_item(cx_knn_radius,                  cl_exdata.knn_radius);
    select_combo_item(cx_smooth_quality,              cl_exdata.smooth_quality);
    select_combo_item(cx_unsharp_radius,              cl_exdata.unsharp_radius);
    select_combo_item(cx_warpsharp_blur,              cl_exdata.warpsharp_blur);
    select_combo_item(cx_deband_sample,               cl_exdata.deband_sample);
}

void check_clplatforms() {
    if (clplatforms.size() > 0) {
        return;
    }
    auto log = std::make_shared<RGYLog>(nullptr, RGY_LOG_ERROR);
    RGYOpenCL cl(log);
    clplatforms = cl.getPlatforms(nullptr);
    for (auto& platform : clplatforms) {
        platform->createDeviceList(CL_DEVICE_TYPE_GPU);
    }
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void*, FILTER *fp) {
    switch (message) {
    case WM_FILTER_FILE_OPEN:
    case WM_FILTER_FILE_CLOSE:
        break;
    case WM_FILTER_INIT:
        cl_exdata_set_default();
        check_clplatforms();
        init_dialog(hwnd, fp);
        return TRUE;
    case WM_FILTER_UPDATE: // フィルタ更新
    case WM_FILTER_SAVE_END: // セーブ終了
        update_cx(fp);
        break;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case ID_CX_OPENCL_DEVICE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param(cx_opencl_device);
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
        case ID_BT_RESIZE_RES_ADD:
            add_cx_resize_res_items(fp);
            break;
        case ID_BT_RESIZE_RES_DEL:
            del_combo_item_current(fp, cx_resize_res);
            break;
        case ID_BT_OPENCL_INFO:
            out_opencl_info(fp);
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
        default:
            break;
        }
        break;
    case WM_FILTER_EXIT:
        break;
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_MOUSEWHEEL:
        SendMessage(GetWindow(hwnd, GW_OWNER), message, wparam, lparam);
        break;
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
    for (int i = track_min; i < track_max; i++, y_pos += track_bar_delta_y) {
        for (int j = 0; j < 5; j++) {
            GetWindowRect(child_hwnd[i*5+j+1], &rc);
            SetWindowPos(child_hwnd[i*5+j+1], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }

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

void init_dialog(HWND hwnd, FILTER *fp) {
    child_hwnd.clear();
    int nCount = 0;
    EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&nCount);


    RECT rc, dialog_rc;
    GetWindowRect(hwnd, &dialog_rc);

    const int columns = 2;
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
    SendMessage(lb_opencl_device, WM_SETTEXT, 0, (LPARAM)"デバイス選択");

    cx_opencl_device = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 68, cb_opencl_platform_y, 205, 100, hwnd, (HMENU)ID_CX_OPENCL_DEVICE, hinst, NULL);
    SendMessage(cx_opencl_device, WM_SETFONT, (WPARAM)b_font, 0);

    for (size_t ip = 0; ip < clplatforms.size(); ip++) {
        for (size_t idev = 0; idev < clplatforms[ip]->devs().size(); idev++) {
            CL_PLATFORM_DEVICE pd;
            pd.s.platform = (int16_t)ip;
            pd.s.device = (int16_t)idev;
            const auto devInfo = clplatforms[ip]->dev(idev).info();
            const auto devName = (devInfo.board_name_amd.length() > 0) ? devInfo.board_name_amd : devInfo.name;
            set_combo_item(cx_opencl_device, devName.c_str(), pd.i);
        }
    }

    //OpenCL info
    bt_opencl_info = CreateWindow("BUTTON", "clinfo", WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_VCENTER, 278, cb_opencl_platform_y, 48, 22, hwnd, (HMENU)ID_BT_OPENCL_INFO, hinst, NULL);
    SendMessage(bt_opencl_info, WM_SETFONT, (WPARAM)b_font, 0);

    //log_level
    const int cb_log_level_y = cb_opencl_platform_y+24;
    lb_log_level = CreateWindow("static", "", SS_SIMPLE | WS_CHILD | WS_VISIBLE, 8, cb_log_level_y, 60, 24, hwnd, (HMENU)ID_LB_LOG_LEVEL, hinst, NULL);
    SendMessage(lb_log_level, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_log_level, WM_SETTEXT, 0, (LPARAM)"ログ出力");

    cx_log_level = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 68, cb_log_level_y, 205, 100, hwnd, (HMENU)ID_CX_LOG_LEVEL, hinst, NULL);
    SendMessage(cx_log_level, WM_SETFONT, (WPARAM)b_font, 0);

    for (const auto& log_level : RGY_LOG_LEVEL_STR) {
        set_combo_item(cx_log_level, log_level.second, log_level.first);
    }

    //checkboxの移動
    const int checkbox_idx = 1+5*CLFILTER_TRACK_MAX;
#if ENABLE_FIELD
    //フィールド処理
    const int cb_filed_y = cb_log_level_y + 24;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_FIELD], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_FIELD], HWND_TOP, rc.left - dialog_rc.left, cb_filed_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
#else
    const int cb_filed_y = cb_log_level_y;
#endif //#if ENABLE_FIELD

    //リサイズ
    const int cb_resize_y = cb_filed_y + 24;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], HWND_TOP, rc.left - dialog_rc.left, cb_resize_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    lb_proc_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, 8 + AVIUTL_1_10_OFFSET, cb_resize_y+24, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_RES, hinst, NULL);
    SendMessage(lb_proc_mode, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_proc_mode, WM_SETTEXT, 0, (LPARAM)"サイズ");

    cx_resize_res = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 68, cb_resize_y+24, 145, 100, hwnd, (HMENU)ID_CX_RESIZE_RES, hinst, NULL);
    SendMessage(cx_resize_res, WM_SETFONT, (WPARAM)b_font, 0);

    bt_resize_res_add = CreateWindow("BUTTON", "追加", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 214, cb_resize_y+24, 32, 22, hwnd, (HMENU)ID_BT_RESIZE_RES_ADD, hinst, NULL);
    SendMessage(bt_resize_res_add, WM_SETFONT, (WPARAM)b_font, 0);

    bt_resize_res_del = CreateWindow("BUTTON", "削除", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 246, cb_resize_y+24, 32, 22, hwnd, (HMENU)ID_BT_RESIZE_RES_DEL, hinst, NULL);
    SendMessage(bt_resize_res_del, WM_SETFONT, (WPARAM)b_font, 0);

    cx_resize_algo = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 68, cb_resize_y+48, 145, 100, hwnd, (HMENU)ID_CX_RESIZE_ALGO, hinst, NULL);
    SendMessage(cx_resize_algo, WM_SETFONT, (WPARAM)b_font, 0);

    update_cx_resize_res_items(fp);
    set_combo_item(cx_resize_algo, "spline16", RGY_VPP_RESIZE_SPLINE16);
    set_combo_item(cx_resize_algo, "spline36", RGY_VPP_RESIZE_SPLINE36);
    set_combo_item(cx_resize_algo, "spline64", RGY_VPP_RESIZE_SPLINE64);
    set_combo_item(cx_resize_algo, "lanczos2", RGY_VPP_RESIZE_LANCZOS2);
    set_combo_item(cx_resize_algo, "lanczos3", RGY_VPP_RESIZE_LANCZOS3);
    set_combo_item(cx_resize_algo, "lanczos4", RGY_VPP_RESIZE_LANCZOS4);
    set_combo_item(cx_resize_algo, "bilinear", RGY_VPP_RESIZE_BILINEAR);

    int y_pos_max = 0;
    // --- 最初の列 -----------------------------------------
    int col = 0;
    int y_pos = cb_resize_y + track_bar_delta_y * 3 + 8;
    int cx_y_pos = 0;
    //colorspace
    move_colorspace(y_pos, col, col_width, CLFILTER_CHECK_COLORSPACE_ENABLE, CLFILTER_CHECK_COLORSPACE_MAX, CLFILTER_TRACK_COLORSPACE_FIRST, CLFILTER_TRACK_COLORSPACE_MAX, track_bar_delta_y, checkbox_idx, dialog_rc, b_font, hwnd, hinst);

    //nnedi
    move_group(y_pos, col, col_width, CLFILTER_CHECK_NNEDI_ENABLE, CLFILTER_CHECK_NNEDI_MAX, CLFILTER_TRACK_NNEDI_FIRST, CLFILTER_TRACK_NNEDI_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);
    y_pos -= track_bar_delta_y / 2;
    add_combobox(cx_nnedi_field,     ID_CX_NNEDI_FIELD,     lb_nnedi_field,     ID_LB_NNEDI_FIELD,    "field",      col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_field+2, 2);
    add_combobox(cx_nnedi_nns,       ID_CX_NNEDI_NNS,       lb_nnedi_nns,       ID_LB_NNEDI_NNS,       "nns",       col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_nns);
    add_combobox(cx_nnedi_nsize,     ID_CX_NNEDI_NSIZE,     lb_nnedi_nsize,     ID_LB_NNEDI_NSIZE,     "nsize",     col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_nsize);
    add_combobox(cx_nnedi_quality,   ID_CX_NNEDI_QUALITY,   lb_nnedi_quality,   ID_LB_NNEDI_QUALITY,   "品質",      col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_quality);
    add_combobox(cx_nnedi_prescreen, ID_CX_NNEDI_PRESCREEN, lb_nnedi_prescreen, ID_LB_NNEDI_PRESCREEN, "前処理",    col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_pre_screen);
    add_combobox(cx_nnedi_errortype, ID_CX_NNEDI_ERRORTYPE, lb_nnedi_errortype, ID_LB_NNEDI_ERRORTYPE, "errortype", col, col_width, y_pos, b_font, hwnd, hinst, list_vpp_nnedi_error_type);
    y_pos += track_bar_delta_y / 2;

    //smooth
    move_group(y_pos, col, col_width, CLFILTER_CHECK_SMOOTH_ENABLE, CLFILTER_CHECK_SMOOTH_MAX, CLFILTER_TRACK_SMOOTH_FIRST, CLFILTER_TRACK_SMOOTH_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_smooth_quality, ID_CX_SMOOTH_QUALITY, lb_smooth_quality, ID_LB_SMOOTH_QUALITY, "品質", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_smooth_quality);

    //knn
    move_group(y_pos, col, col_width, CLFILTER_CHECK_KNN_ENABLE, CLFILTER_CHECK_KNN_MAX, CLFILTER_TRACK_KNN_FIRST, CLFILTER_TRACK_KNN_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_knn_radius, ID_CX_KNN_RADIUS, lb_knn_radius, ID_LB_KNN_RADIUS, "適用半径", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_raduis);

    //pmd
    move_group(y_pos, col, col_width, CLFILTER_CHECK_PMD_ENABLE, CLFILTER_CHECK_PMD_MAX, CLFILTER_TRACK_PMD_FIRST, CLFILTER_TRACK_PMD_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);

    y_pos_max = std::max(y_pos_max, y_pos);
    // --- 次の列 -----------------------------------------

    col = 1;
    y_pos = cb_resize_y;
    //unsharp
    move_group(y_pos, col, col_width, CLFILTER_CHECK_UNSHARP_ENABLE, CLFILTER_CHECK_UNSHARP_MAX, CLFILTER_TRACK_UNSHARP_FIRST, CLFILTER_TRACK_UNSHARP_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_unsharp_radius, ID_CX_UNSHARP_RADIUS, lb_unsharp_radius, ID_LB_UNSHARP_RADIUS, "範囲", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_raduis);

    //エッジレベル調整
    move_group(y_pos, col, col_width, CLFILTER_CHECK_EDGELEVEL_ENABLE, CLFILTER_CHECK_EDGELEVEL_MAX, CLFILTER_TRACK_EDGELEVEL_FIRST, CLFILTER_TRACK_EDGELEVEL_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);

    //warpsharp
    move_group(y_pos, col, col_width, CLFILTER_CHECK_WARPSHARP_ENABLE, CLFILTER_CHECK_WARPSHARP_MAX, CLFILTER_TRACK_WARPSHARP_FIRST, CLFILTER_TRACK_WARPSHARP_MAX, track_bar_delta_y, ADD_CX_FIRST, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_warpsharp_blur, ID_CX_WARPSHARP_BLUR, lb_warpsharp_blur, ID_LB_WARPSHARP_BLUR, "ブラー", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_1_to_10);

    //tweak
    move_group(y_pos, col, col_width, CLFILTER_CHECK_TWEAK_ENABLE, CLFILTER_CHECK_TWEAK_MAX, CLFILTER_TRACK_TWEAK_FIRST, CLFILTER_TRACK_TWEAK_MAX, track_bar_delta_y, ADD_CX_FIRST, 0, cx_y_pos, checkbox_idx, dialog_rc);

    //バンディング
    move_group(y_pos, col, col_width, CLFILTER_CHECK_DEBAND_ENABLE, CLFILTER_CHECK_DEBAND_MAX, CLFILTER_TRACK_DEBAND_FIRST, CLFILTER_TRACK_DEBAND_MAX, track_bar_delta_y, ADD_CX_AFTER_TRACK, 1, cx_y_pos, checkbox_idx, dialog_rc);
    add_combobox(cx_deband_sample, ID_CX_DEBAND_SAMPLE, lb_deband_sample, ID_LB_DEBAND_SAMPLE, "sample", col, col_width, cx_y_pos, b_font, hwnd, hinst, list_vpp_deband);
    
    y_pos_max = std::max(y_pos_max, y_pos);

    SetWindowPos(hwnd, HWND_TOP, 0, 0, (dialog_rc.right - dialog_rc.left) * columns, y_pos_max + 24, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

    update_cx(fp);
}

//---------------------------------------------------------------------
//        フィルタ処理関数
//---------------------------------------------------------------------

void multi_thread_func(int thread_id, int thread_num, void *param1, void *param2) {
    //    thread_id    : スレッド番号 ( 0 ～ thread_num-1 )
    //    thread_num    : スレッド数 ( 1 ～ )
    //    param1        : 汎用パラメータ
    //    param2        : 汎用パラメータ
    //
    //    この関数内からWin32APIや外部関数(rgb2yc,yc2rgbは除く)を使用しないでください。
    //
    FILTER *fp                = (FILTER *)param1;
    FILTER_PROC_INFO *fpip    = (FILTER_PROC_INFO *)param2;

}

static clFilterChainParam func_proc_get_param(const FILTER *fp) {
    clFilterChainParam prm;
    //dllのモジュールハンドル
    prm.hModule = fp->dll_hinst;
    prm.log_level = cl_exdata.log_level;

    //リサイズ
    prm.resize_algo        = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo;

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
    prm.colorspace.enable = fp->check[CLFILTER_CHECK_COLORSPACE_ENABLE] != 0;
    prm.colorspace.convs.push_back(conv);
    prm.colorspace.hdr2sdr.tonemap         = cl_exdata.hdr2sdr;
    prm.colorspace.hdr2sdr.hdr_source_peak = (double)fp->track[CLFILTER_TRACK_COLORSPACE_SOURCE_PEAK];
    prm.colorspace.hdr2sdr.ldr_nits        = (double)fp->track[CLFILTER_TRACK_COLORSPACE_LDR_NITS];
#if ENABLE_HDR2SDR_DESAT
    prm.colorspace.hdr2sdr.desat_base     = (float)fp->track[CLFILTER_TRACK_COLORSPACE_DESAT_BASE] * 0.01f;
    prm.colorspace.hdr2sdr.desat_strength = (float)fp->track[CLFILTER_TRACK_COLORSPACE_DESAT_STRENGTH] * 0.01f;
    prm.colorspace.hdr2sdr.desat_exp      = (float)fp->track[CLFILTER_TRACK_COLORSPACE_DESAT_EXP] * 0.1f;
#endif //#if ENABLE_HDR2SDR_DESAT

    //smooth
    prm.smooth.enable     = fp->check[CLFILTER_CHECK_SMOOTH_ENABLE] != 0;
    prm.smooth.quality    = cl_exdata.smooth_quality;
    prm.smooth.qp         = fp->track[CLFILTER_TRACK_SMOOTH_QP];

    //knn
    prm.knn.enable         = fp->check[CLFILTER_CHECK_KNN_ENABLE] != 0;
    prm.knn.radius         = cl_exdata.knn_radius;
    prm.knn.strength       = (float)fp->track[CLFILTER_TRACK_KNN_STRENGTH] * 0.01f;
    prm.knn.lerpC          = (float)fp->track[CLFILTER_TRACK_KNN_LERP] * 0.01f;
    prm.knn.lerp_threshold = (float)fp->track[CLFILTER_TRACK_KNN_TH_LERP] * 0.01f;

    //pmd
    prm.pmd.enable         = fp->check[CLFILTER_CHECK_PMD_ENABLE] != 0;
    prm.pmd.applyCount     = cl_exdata.pmd_apply_count;
    prm.pmd.strength       = (float)fp->track[CLFILTER_TRACK_PMD_STRENGTH];
    prm.pmd.threshold      = (float)fp->track[CLFILTER_TRACK_PMD_THRESHOLD];

    //unsharp
    prm.unsharp.enable    = fp->check[CLFILTER_CHECK_UNSHARP_ENABLE] != 0;
    prm.unsharp.radius    = cl_exdata.unsharp_radius;
    prm.unsharp.weight    = (float)fp->track[CLFILTER_TRACK_UNSHARP_WEIGHT] * 0.1f;
    prm.unsharp.threshold = (float)fp->track[CLFILTER_TRACK_UNSHARP_THRESHOLD];

    //edgelevel
    prm.edgelevel.enable    = fp->check[CLFILTER_CHECK_EDGELEVEL_ENABLE] != 0;
    prm.edgelevel.strength  = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_STRENGTH];
    prm.edgelevel.threshold = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_THRESHOLD];
    prm.edgelevel.black     = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_BLACK];
    prm.edgelevel.white     = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_WHITE];

    //warpsharp
    prm.warpsharp.enable    = fp->check[CLFILTER_CHECK_WARPSHARP_ENABLE] != 0;
    prm.warpsharp.threshold = (float)fp->track[CLFILTER_TRACK_WARPSHARP_THRESHOLD];
    prm.warpsharp.blur      = cl_exdata.warpsharp_blur;
    prm.warpsharp.type      = fp->check[CLFILTER_CHECK_WARPSHARP_SMALL_BLUR] ? 1 : 0;
    prm.warpsharp.depth     = (float)fp->track[CLFILTER_TRACK_WARPSHARP_DEPTH];
    prm.warpsharp.chroma    = fp->check[CLFILTER_CHECK_WARPSHARP_CHROMA_MASK] ? 1 : 0;

    //tweak
    prm.tweak.enable      = fp->check[CLFILTER_CHECK_TWEAK_ENABLE] != 0;
    prm.tweak.brightness  = (float)fp->track[CLFILTER_TRACK_TWEAK_BRIGHTNESS] * 0.01f;
    prm.tweak.contrast    = (float)fp->track[CLFILTER_TRACK_TWEAK_CONTRAST] * 0.01f;
    prm.tweak.gamma       = (float)fp->track[CLFILTER_TRACK_TWEAK_GAMMA] * 0.01f;
    prm.tweak.saturation  = (float)fp->track[CLFILTER_TRACK_TWEAK_SATURATION] * 0.01f;
    prm.tweak.hue         = (float)fp->track[CLFILTER_TRACK_TWEAK_HUE];

    //deband
    prm.deband.enable        = fp->check[CLFILTER_CHECK_DEBAND_ENABLE] != 0;
    prm.deband.range         = fp->track[CLFILTER_TRACK_DEBAND_RANGE];
    prm.deband.threY         = fp->track[CLFILTER_TRACK_DEBAND_Y];
    prm.deband.threCb        = fp->track[CLFILTER_TRACK_DEBAND_C];
    prm.deband.threCr        = fp->track[CLFILTER_TRACK_DEBAND_C];
    prm.deband.ditherY       = fp->track[CLFILTER_TRACK_DEBAND_DITHER_Y];
    prm.deband.ditherC       = fp->track[CLFILTER_TRACK_DEBAND_DITHER_C];
    prm.deband.sample        = cl_exdata.deband_sample;
    prm.deband.seed          = 1234;
    prm.deband.blurFirst     = fp->check[CLFILTER_CHECK_DEBAND_BLUR_FIRST] != 0;
    prm.deband.randEachFrame = fp->check[CLFILTER_CHECK_DEBAND_RAND_EACH_FRAME] != 0;

    //nnedi
    prm.nnedi.enable        = fp->check[CLFILTER_CHECK_NNEDI_ENABLE] != 0;
    prm.nnedi.field         = cl_exdata.nnedi_field;
    prm.nnedi.nsize         = cl_exdata.nnedi_nsize;
    prm.nnedi.nns           = cl_exdata.nnedi_nns;
    prm.nnedi.quality       = cl_exdata.nnedi_quality;
    prm.nnedi.pre_screen    = cl_exdata.nnedi_prescreen;
    prm.nnedi.errortype     = cl_exdata.nnedi_errortype;
    prm.nnedi.precision     = VPP_FP_PRECISION_AUTO;

    return prm;
}

static RGYFrameInfo func_proc_set_frame_info(const FILTER *fp, const FILTER_PROC_INFO *fpip, const int iframeID, const int width, const int height, PIXEL_YC *frame) {
    RGYFrameInfo in;
    //入力フレーム情報
    in.width      = width;
    in.height     = height;
    in.pitch[0]   = fpip->max_w * 6;
    in.csp        = RGY_CSP_YC48;
#if ENABLE_FIELD
    in.picstruct  = fp->check[CLFILTER_CHECK_FIELD] ? RGY_PICSTRUCT_INTERLACED : RGY_PICSTRUCT_FRAME;
#else
    in.picstruct  = RGY_PICSTRUCT_FRAME;
#endif //#if ENABLE_FIELD
    in.ptr[0]     = (uint8_t *)frame;
    in.mem_type   = RGY_MEM_TYPE_CPU;
    in.inputFrameId = iframeID;

    return in;
}

BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip) {
    if (!clfilter
        || clfilter->platformID() != cl_exdata.cl_dev_id.s.platform
        || clfilter->deviceID()   != cl_exdata.cl_dev_id.s.device) {
        clfilter = std::make_unique<clFilterChain>();
        std::string mes = AUF_FULL_NAME;
        mes += ": ";
        if (clfilter->init(cl_exdata.cl_dev_id.s.platform, cl_exdata.cl_dev_id.s.device, CL_DEVICE_TYPE_GPU, cl_exdata.log_level)) {
            mes += "フィルタは無効です: OpenCLを使用できません。";
            SendMessage(fp->hwnd, WM_SETTEXT, 0, (LPARAM)mes.c_str());
            return FALSE;
        }
        const auto dev_name = clfilter->getDeviceName();
        mes += (dev_name.length() == 0) ? "OpenCL 有効" : dev_name;
        SendMessage(fp->hwnd, WM_SETTEXT, 0, (LPARAM)mes.c_str());
        is_saving = FALSE;
    }

    const int out_width  = (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) ? resize_res[cl_exdata.resize_idx].first  : fpip->w;
    const int out_height = (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) ? resize_res[cl_exdata.resize_idx].second : fpip->h;
    const bool resize_required = out_width != fpip->w || out_height != fpip->h;
    const auto prm = func_proc_get_param(fp);
    if (prm.getFilterChain(resize_required).size() == 0) { // 適用すべきフィルタがない場合
        return TRUE; // 何もしない
    }

    // 保存モード(is_saving=true)の時に、どのくらい先まで処理をしておくべきか?
    static const int frameInOffset   = 2;
    static const int frameProcOffset = 1;
    static const int frameOutOffset  = 0;
    static_assert(frameInOffset < clFilterFrameBuffer::bufSize);
    const int current_frame = fpip->frame; // 現在のフレーム番号
    const int frame_n = fpip->frame_n;
    if (fpip->frame_n <= 0 || current_frame < 0) {
        return TRUE; // 何もしない
    }
    // どのフレームから処理を開始すべきか?
    int frameIn   = current_frame + ((is_saving) ? frameInOffset   : 0);
    int frameProc = current_frame + ((is_saving) ? frameProcOffset : 0);
    int frameOut  = current_frame + ((is_saving) ? frameOutOffset  : 0);
    if (clfilter->getNextOutFrameId() != current_frame // 出てくる予定のフレームがずれていたらリセット
        || is_saving != fp->exfunc->is_saving(fpip->editp)) { // モードが切り替わったらリセット
        clfilter->resetPipeline();
        frameIn   = current_frame;
        frameProc = current_frame;
        frameOut  = current_frame;
        is_saving = fp->exfunc->is_saving(fpip->editp);
    }
    fp->exfunc->set_ycp_filtering_cache_size(fp, fpip->max_w, fpip->max_h, frameInOffset+1, NULL);

    // -- フレームの転送 -----------------------------------------------------------------
    // frameIn の終了フレーム
    const int frameInFin = (is_saving) ? std::min(current_frame + frameInOffset, frame_n - 1) : current_frame;
    // フレーム転送の実行
    for (; frameIn <= frameInFin; frameIn++) {
        int width = fpip->w, height = fpip->h;
        PIXEL_YC *ptr = (frameIn == current_frame) ? fpip->ycp_edit : (PIXEL_YC *)fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, frameIn, &width, &height);
        const RGYFrameInfo in = func_proc_set_frame_info(fp, fpip, frameIn, width, height, ptr);
        if (clfilter->sendInFrame(&in) != RGY_ERR_NONE) {
            return FALSE;
        }
    }
    // -- フレームの処理 -----------------------------------------------------------------
    if (prm != clfilter->getPrm()) { // パラメータが変更されていたら、
        frameProc = current_frame;   // 現在のフレームから処理をやり直す
    }
    // frameProc の終了フレーム
    const int frameProcFin = (is_saving) ? std::min(current_frame + frameProcOffset, frame_n - 1) : current_frame;
    // フレーム処理の実行
    for (; frameProc <= frameProcFin; frameProc++) {
        if (clfilter->proc(frameProc, out_width, out_height, prm) != RGY_ERR_NONE) {
            return FALSE;
        }
    }
    // -- フレームの取得 -----------------------------------------------------------------
    RGYFrameInfo out = func_proc_set_frame_info(fp, fpip, current_frame, out_width, out_height, fpip->ycp_temp);
    if (clfilter->getOutFrame(&out) != RGY_ERR_NONE) {
        return FALSE;
    }
    std::swap(fpip->ycp_edit, fpip->ycp_temp);
    fpip->w = out_width;
    fpip->h = out_height;
    return TRUE;
}
