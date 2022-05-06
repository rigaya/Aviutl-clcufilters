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
#define ID_LB_RESIZE_RES      40001
#define ID_CX_RESIZE_RES      40002
#define ID_BT_RESIZE_RES_ADD  40003
#define ID_BT_RESIZE_RES_DEL  40004
#define ID_CX_RESIZE_ALGO     40005

#define ID_LB_NNEDI_FIELD     40006
#define ID_CX_NNEDI_FIELD     40007
#define ID_LB_NNEDI_NSIZE     40008
#define ID_CX_NNEDI_NSIZE     40009
#define ID_LB_NNEDI_NNS       40010
#define ID_CX_NNEDI_NNS       40011
#define ID_LB_NNEDI_QUALITY   40012
#define ID_CX_NNEDI_QUALITY   40013
#define ID_LB_NNEDI_PRESCREEN 40014
#define ID_CX_NNEDI_PRESCREEN 40015
#define ID_LB_NNEDI_ERRORTYPE 40016
#define ID_CX_NNEDI_ERRORTYPE 40017

#define ID_LB_OPENCL_DEVICE   40018
#define ID_CX_OPENCL_DEVICE   40019

union CL_PLATFORM_DEVICE {
    struct {
        int16_t platform;
        int16_t device;
    } s;
    int i;
};

#pragma pack(1)
struct CLFILTER_EXDATA {
    CL_PLATFORM_DEVICE cl_dev_id;

    int resize_idx;
    int resize_algo;

    VppNnediField nnedi_field;
    int nnedi_nns;
    VppNnediNSize nnedi_nsize;
    VppNnediQuality nnedi_quality;
    VppNnediPreScreen nnedi_prescreen;
    VppNnediErrorType nnedi_errortype;

    char reserved[988];
};
# pragma pack()
static_assert(sizeof(CLFILTER_EXDATA) == 1024);

static CLFILTER_EXDATA cl_exdata;
static std::unique_ptr<clFilterChain> clfilter;
static std::vector<std::shared_ptr<RGYOpenCLPlatform>> clplatforms;


//---------------------------------------------------------------------
//        フィルタ構造体定義
//---------------------------------------------------------------------

static const int filter_count = 8;

//  トラックバーの名前
const TCHAR *track_name[] = {
    //リサイズ
    "適用半径", "強さ", "ブレンド度合い", "ブレンド閾値", //knn
    "適用回数", "強さ", "閾値", //pmd
    "品質", "QP", // smooth
    "範囲", "強さ", "閾値", //unsharp
    "特性", "閾値", "黒", "白", //エッジレベル調整
    "閾値", "ブラー", "タイプ", "深度", "色差", //warpsharp
    "輝度", "コントラスト", "ガンマ", "彩度", "色相", //tweak
    "range", "Y", "Cb", "Cr", "ditherY", "ditherC", "sample", "seed" //バンディング低減
};

enum {
    CLFILTER_TRACK_KNN_FIRST,
    CLFILTER_TRACK_KNN_RADIUS = CLFILTER_TRACK_KNN_FIRST,
    CLFILTER_TRACK_KNN_STRENGTH,
    CLFILTER_TRACK_KNN_LERP,
    CLFILTER_TRACK_KNN_TH_LERP,
    CLFILTER_TRACK_KNN_MAX,

    CLFILTER_TRACK_PMD_FIRST = CLFILTER_TRACK_KNN_MAX,
    CLFILTER_TRACK_PMD_APPLY_COUNT = CLFILTER_TRACK_PMD_FIRST,
    CLFILTER_TRACK_PMD_STRENGTH,
    CLFILTER_TRACK_PMD_THRESHOLD,
    CLFILTER_TRACK_PMD_MAX,

    CLFILTER_TRACK_SMOOTH_FIRST = CLFILTER_TRACK_PMD_MAX,
    CLFILTER_TRACK_SMOOTH_QUALITY = CLFILTER_TRACK_SMOOTH_FIRST,
    CLFILTER_TRACK_SMOOTH_QP,
    CLFILTER_TRACK_SMOOTH_MAX,

    CLFILTER_TRACK_UNSHARP_FIRST = CLFILTER_TRACK_SMOOTH_MAX,
    CLFILTER_TRACK_UNSHARP_RADIUS = CLFILTER_TRACK_UNSHARP_FIRST,
    CLFILTER_TRACK_UNSHARP_WEIGHT,
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
    CLFILTER_TRACK_WARPSHARP_BLUR,
    CLFILTER_TRACK_WARPSHARP_TYPE,
    CLFILTER_TRACK_WARPSHARP_DEPTH,
    CLFILTER_TRACK_WARPSHARP_CHROMA,
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
    CLFILTER_TRACK_DEBAND_CB,
    CLFILTER_TRACK_DEBAND_CR,
    CLFILTER_TRACK_DEBAND_DITHER_Y,
    CLFILTER_TRACK_DEBAND_DITHER_C,
    CLFILTER_TRACK_DEBAND_SAMPLE,
    CLFILTER_TRACK_DEBAND_SEED,
    CLFILTER_TRACK_DEBAND_MAX,

    CLFILTER_TRACK_NNEDI_FIRST = CLFILTER_TRACK_DEBAND_MAX,
    CLFILTER_TRACK_NNEDI_MAX = CLFILTER_TRACK_NNEDI_FIRST,

    CLFILTER_TRACK_MAX = CLFILTER_TRACK_NNEDI_MAX,
};

//  トラックバーの初期値
int track_default[] = {
    3, 8, 20, 80, //knn
    2, 100, 100, //pmd
    3, 12, //smooth
    3, 5, 10, //unsharp
    5, 20, 0, 0, //エッジレベル調整
    128, 2, 0, 16, 0, //warpsharp
    0, 100, 100, 100, 0, //tweak
    15, 15, 15, 15, 15, 15, 1, 0 //バンディング低減
};
//  トラックバーの下限値
int track_s[] = {
    1,0,0,0, //knn
    1,0,0, //pmd
    1, 1, //smooth
    1,0,0, //unsharp
    -31,0,0,0, //エッジレベル調整
    0, 1, 0, -128, 0, //warpsharp
    -100,-200,1,0,-180, //tweak
    0,0,0,0,0,0,0,0 //バンディング低減
};
//  トラックバーの上限値
int track_e[] = {
    5, 100, 100, 100, //knn
    10, 100, 255, //pmd
    6, 63, //smooth
    9, 100, 255, //unsharp
    31, 255, 31, 31, //エッジレベル調整
    255, 10, 1, 128, 1, //warpsharp
    100,200,200,200,180, //tweak
    127,31,31,31,31,31,2,8192//バンディング低減
};

//  トラックバーの数
#define    TRACK_N    (_countof(track_name))
static_assert(TRACK_N == CLFILTER_TRACK_MAX, "TRACK_N check");
static_assert(TRACK_N == _countof(track_default), "track_default check");
static_assert(TRACK_N == _countof(track_s), "track_s check");
static_assert(TRACK_N == _countof(track_e), "track_e check");

//  チェックボックスの名前
const TCHAR *check_name[] = {
    "フィールド処理",
    "リサイズ",
    "ノイズ除去 (knn)",
    "ノイズ除去 (pmd)",
    "ノイズ除去 (smooth)",
    "unsharp",
    "エッジレベル調整",
    "warpsharp",
    "色調補正",
    "バンディング低減", "ブラー処理を先に", "毎フレーム乱数を生成",
    "nnedi"
};

enum {
    CLFILTER_CHECK_FIELD,

    CLFILTER_CHECK_RESIZE_ENABLE,
    CLFILTER_CHECK_RESIZE_MAX,

    CLFILTER_CHECK_KNN_ENABLE = CLFILTER_CHECK_RESIZE_MAX,
    CLFILTER_CHECK_KNN_MAX,

    CLFILTER_CHECK_PMD_ENABLE = CLFILTER_CHECK_KNN_MAX,
    CLFILTER_CHECK_PMD_MAX,

    CLFILTER_CHECK_SMOOTH_ENABLE = CLFILTER_CHECK_PMD_MAX,
    CLFILTER_CHECK_SMOOTH_MAX,

    CLFILTER_CHECK_UNSHARP_ENABLE = CLFILTER_CHECK_SMOOTH_MAX,
    CLFILTER_CHECK_UNSHARP_MAX,

    CLFILTER_CHECK_EDGELEVEL_ENABLE = CLFILTER_CHECK_UNSHARP_MAX,
    CLFILTER_CHECK_EDGELEVEL_MAX,

    CLFILTER_CHECK_WARPSHARP_ENABLE = CLFILTER_CHECK_EDGELEVEL_MAX,
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
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, 0, 0, 0,
    0
};
//  チェックボックスの数
#define    CHECK_N    (_countof(check_name))
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
static HWND cx_resize_res;
static HWND bt_resize_res_add;
static HWND bt_resize_res_del;
static HWND cx_resize_algo;

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

static void change_cx_param(HWND hwnd) {
    LRESULT ret;

    // 選択番号取得
    ret = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    ret = SendMessage(hwnd, CB_GETITEMDATA, ret, 0);

    if (ret != CB_ERR) {
        if (hwnd == cx_opencl_device) {
            cl_exdata.cl_dev_id.i = ret;
        } else if (hwnd == cx_resize_res) {
            cl_exdata.resize_idx = ret;
        } else if (hwnd == cx_resize_algo) {
            cl_exdata.resize_algo = ret;
        } else if (hwnd == cx_nnedi_field) {
            cl_exdata.nnedi_field = (VppNnediField)ret;
        } else if (hwnd == cx_nnedi_nns) {
            cl_exdata.nnedi_nns = ret;
        } else if (hwnd == cx_nnedi_nsize) {
            cl_exdata.nnedi_nsize = (VppNnediNSize)ret;
        } else if (hwnd == cx_nnedi_quality) {
            cl_exdata.nnedi_quality = (VppNnediQuality)ret;
        } else if (hwnd == cx_nnedi_prescreen) {
            cl_exdata.nnedi_prescreen = (VppNnediPreScreen)ret;
        } else if (hwnd == cx_nnedi_errortype) {
            cl_exdata.nnedi_errortype = (VppNnediErrorType)ret;
        }
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
    if (hwnd == cx_opencl_device) {
        cl_exdata.cl_dev_id.i = current_data;
    } else if (hwnd == cx_resize_res) {
        cl_exdata.resize_idx = current_data;
    } else if (hwnd == cx_resize_algo) {
        cl_exdata.resize_algo = current_data;
    } else if (hwnd == cx_nnedi_field) {
        cl_exdata.nnedi_field = (VppNnediField)current_data;
    } else if (hwnd == cx_nnedi_nns) {
        cl_exdata.nnedi_nns = current_data;
    } else if (hwnd == cx_nnedi_nsize) {
        cl_exdata.nnedi_nsize = (VppNnediNSize)current_data;
    } else if (hwnd == cx_nnedi_quality) {
        cl_exdata.nnedi_quality = (VppNnediQuality)current_data;
    } else if (hwnd == cx_nnedi_prescreen) {
        cl_exdata.nnedi_prescreen = (VppNnediPreScreen)current_data;
    } else if (hwnd == cx_nnedi_errortype) {
        cl_exdata.nnedi_errortype = (VppNnediErrorType)current_data;
    }
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
        VppNnedi nnedi;
        cl_exdata.nnedi_field = nnedi.field;
        cl_exdata.nnedi_nns = nnedi.nns;
        cl_exdata.nnedi_nsize = nnedi.nsize;
        cl_exdata.nnedi_quality = nnedi.quality;
        cl_exdata.nnedi_prescreen = nnedi.pre_screen;
        cl_exdata.nnedi_errortype = nnedi.errortype;
    }
    select_combo_item(cx_opencl_device,   cl_exdata.cl_dev_id.i);
    select_combo_item(cx_resize_res,      cl_exdata.resize_idx);
    select_combo_item(cx_resize_algo,     cl_exdata.resize_algo);
    select_combo_item(cx_nnedi_field,     cl_exdata.nnedi_field);
    select_combo_item(cx_nnedi_nns,       cl_exdata.nnedi_nns);
    select_combo_item(cx_nnedi_nsize,     cl_exdata.nnedi_nsize);
    select_combo_item(cx_nnedi_quality,   cl_exdata.nnedi_quality);
    select_combo_item(cx_nnedi_prescreen, cl_exdata.nnedi_prescreen);
    select_combo_item(cx_nnedi_errortype, cl_exdata.nnedi_errortype);
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

void move_group(int& y_pos, int col, int col_width, int check_min, int check_max, int track_min, int track_max, const int track_bar_delta_y, const int checkbox_idx, const RECT& dialog_rc) {
    RECT rc;
    GetWindowRect(child_hwnd[checkbox_idx + check_min], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + check_min], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    y_pos += track_bar_delta_y;

    for (int i = track_min; i < track_max; i++, y_pos += track_bar_delta_y) {
        for (int j = 0; j < 5; j++) {
            GetWindowRect(child_hwnd[i*5+j+1], &rc);
            SetWindowPos(child_hwnd[i*5+j+1], HWND_TOP, rc.left - dialog_rc.left + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    for (int i = check_min+1; i < check_max; i++, y_pos += track_bar_delta_y) {
        GetWindowRect(child_hwnd[checkbox_idx+i], &rc);
        SetWindowPos(child_hwnd[checkbox_idx+i], HWND_TOP, rc.left - dialog_rc.left + 10 + col * col_width, y_pos, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }
    y_pos += track_bar_delta_y / 2;
}

void set_combobox_items(HWND hwnd_cx, const CX_DESC *cx_items, int limit = INT_MAX) {
    for (int i = 0; cx_items[i].desc && i < limit; i++) {
        set_combo_item(hwnd_cx, (char *)cx_items[i].desc, cx_items[i].value);
    }
}

void add_combobox(HWND& hwnd_cx, int id_cx, HWND& hwnd_lb, int id_lb, const char *lb_str, int& y_pos, HFONT b_font, HWND hwnd, HINSTANCE hinst, const CX_DESC *cx_items, int cx_item_limit = INT_MAX) {
    hwnd_lb = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, 8, y_pos, 60, 24, hwnd, (HMENU)id_lb, hinst, NULL);
    SendMessage(hwnd_lb, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(hwnd_lb, WM_SETTEXT, 0, (LPARAM)lb_str);
    hwnd_cx = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 68, y_pos, 145, 100, hwnd, (HMENU)id_cx, hinst, NULL);
    SendMessage(hwnd_cx, WM_SETFONT, (WPARAM)b_font, 0);
    set_combobox_items(hwnd_cx, cx_items, cx_item_limit);
    y_pos += 24;
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
    SetWindowPos(child_hwnd[0], HWND_TOP, rc.left - dialog_rc.left + (columns-1) * col_width, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

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
            set_combo_item(cx_opencl_device, clplatforms[ip]->dev(idev).info().name.c_str(), pd.i);
        }
    }

    //checkboxの移動
    const int checkbox_idx = 1+5*CLFILTER_TRACK_MAX;
    //フィールド処理
    const int cb_filed_y = cb_opencl_platform_y + 24;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_FIELD], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_FIELD], HWND_TOP, rc.left - dialog_rc.left, cb_filed_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    //リサイズ
    const int cb_resize_y = cb_filed_y + 24;
    GetWindowRect(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], &rc);
    SetWindowPos(child_hwnd[checkbox_idx + CLFILTER_CHECK_RESIZE_ENABLE], HWND_TOP, rc.left - dialog_rc.left, cb_resize_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

    lb_proc_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, 8, cb_resize_y+24, 60, 24, hwnd, (HMENU)ID_LB_RESIZE_RES, hinst, NULL);
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
    set_combo_item(cx_resize_algo, "spline36", RGY_VPP_RESIZE_SPLINE16);
    set_combo_item(cx_resize_algo, "spline64", RGY_VPP_RESIZE_SPLINE64);
    set_combo_item(cx_resize_algo, "lanczos2", RGY_VPP_RESIZE_LANCZOS2);
    set_combo_item(cx_resize_algo, "lanczos3", RGY_VPP_RESIZE_LANCZOS3);
    set_combo_item(cx_resize_algo, "lanczos4", RGY_VPP_RESIZE_LANCZOS4);
    set_combo_item(cx_resize_algo, "bilinear", RGY_VPP_RESIZE_BILINEAR);

    //nnedi
    int y_pos = cb_resize_y + track_bar_delta_y * 3 + 8;
    move_group(y_pos, 0, col_width, CLFILTER_CHECK_NNEDI_ENABLE, CLFILTER_CHECK_NNEDI_MAX, CLFILTER_TRACK_NNEDI_FIRST, CLFILTER_TRACK_NNEDI_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);
    y_pos -= track_bar_delta_y / 2;
    add_combobox(cx_nnedi_field,     ID_CX_NNEDI_FIELD,     lb_nnedi_field,     ID_LB_NNEDI_FIELD,    "field",      y_pos, b_font, hwnd, hinst, list_vpp_nnedi_field+2, 2);
    add_combobox(cx_nnedi_nns,       ID_CX_NNEDI_NNS,       lb_nnedi_nns,       ID_LB_NNEDI_NNS,       "nns",       y_pos, b_font, hwnd, hinst, list_vpp_nnedi_nns);
    add_combobox(cx_nnedi_nsize,     ID_CX_NNEDI_NSIZE,     lb_nnedi_nsize,     ID_LB_NNEDI_NSIZE,     "nsize",     y_pos, b_font, hwnd, hinst, list_vpp_nnedi_nsize);
    add_combobox(cx_nnedi_quality,   ID_CX_NNEDI_QUALITY,   lb_nnedi_quality,   ID_LB_NNEDI_QUALITY,   "品質",      y_pos, b_font, hwnd, hinst, list_vpp_nnedi_quality);
    add_combobox(cx_nnedi_prescreen, ID_CX_NNEDI_PRESCREEN, lb_nnedi_prescreen, ID_LB_NNEDI_PRESCREEN, "前処理",    y_pos, b_font, hwnd, hinst, list_vpp_nnedi_pre_screen);
    add_combobox(cx_nnedi_errortype, ID_CX_NNEDI_ERRORTYPE, lb_nnedi_errortype, ID_LB_NNEDI_ERRORTYPE, "errortype", y_pos, b_font, hwnd, hinst, list_vpp_nnedi_error_type);
    y_pos += track_bar_delta_y / 2;

    //knn
    move_group(y_pos, 0, col_width, CLFILTER_CHECK_KNN_ENABLE, CLFILTER_CHECK_KNN_MAX, CLFILTER_TRACK_KNN_FIRST, CLFILTER_TRACK_KNN_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);

    //pmd
    move_group(y_pos, 0, col_width, CLFILTER_CHECK_PMD_ENABLE, CLFILTER_CHECK_PMD_MAX, CLFILTER_TRACK_PMD_FIRST, CLFILTER_TRACK_PMD_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);

    //smooth
    move_group(y_pos, 0, col_width, CLFILTER_CHECK_SMOOTH_ENABLE, CLFILTER_CHECK_SMOOTH_MAX, CLFILTER_TRACK_SMOOTH_FIRST, CLFILTER_TRACK_SMOOTH_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);

    //tweak
    move_group(y_pos, 0, col_width, CLFILTER_CHECK_TWEAK_ENABLE, CLFILTER_CHECK_TWEAK_MAX, CLFILTER_TRACK_TWEAK_FIRST, CLFILTER_TRACK_TWEAK_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);
    int y_pos_max = y_pos;

    //unsharp
    y_pos = cb_resize_y;
    move_group(y_pos, 1, col_width, CLFILTER_CHECK_UNSHARP_ENABLE, CLFILTER_CHECK_UNSHARP_MAX, CLFILTER_TRACK_UNSHARP_FIRST, CLFILTER_TRACK_UNSHARP_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);

    //エッジレベル調整
    move_group(y_pos, 1, col_width, CLFILTER_CHECK_EDGELEVEL_ENABLE, CLFILTER_CHECK_EDGELEVEL_MAX, CLFILTER_TRACK_EDGELEVEL_FIRST, CLFILTER_TRACK_EDGELEVEL_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);

    //warpsharp
    move_group(y_pos, 1, col_width, CLFILTER_CHECK_WARPSHARP_ENABLE, CLFILTER_CHECK_WARPSHARP_MAX, CLFILTER_TRACK_WARPSHARP_FIRST, CLFILTER_TRACK_WARPSHARP_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);

    //バンディング
    move_group(y_pos, 1, col_width, CLFILTER_CHECK_DEBAND_ENABLE, CLFILTER_CHECK_DEBAND_MAX, CLFILTER_TRACK_DEBAND_FIRST, CLFILTER_TRACK_DEBAND_MAX, track_bar_delta_y, checkbox_idx, dialog_rc);
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

BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip) {
    if (!clfilter
        || clfilter->platformID() != cl_exdata.cl_dev_id.s.platform
        || clfilter->deviceID() != cl_exdata.cl_dev_id.s.device) {
        clfilter = std::make_unique<clFilterChain>();
        std::string mes = AUF_FULL_NAME;
        mes += ": ";
        if (clfilter->init(cl_exdata.cl_dev_id.s.platform, cl_exdata.cl_dev_id.s.device, CL_DEVICE_TYPE_GPU)) {
            mes += "フィルタは無効です: OpenCLを使用できません。";
            SendMessage(fp->hwnd, WM_SETTEXT, 0, (LPARAM)mes.c_str());
            return FALSE;
        }
        const auto dev_name = clfilter->getDeviceName();
        mes += (dev_name.length() == 0) ? "OpenCL 有効" : dev_name;
        SendMessage(fp->hwnd, WM_SETTEXT, 0, (LPARAM)mes.c_str());
    }
    clFilterChainParam prm;
    //dllのモジュールハンドル
    prm.hModule = fp->dll_hinst;

    //リサイズ
    prm.resize_algo        = (RGY_VPP_RESIZE_ALGO)cl_exdata.resize_algo;

    //knn
    prm.knn.enable         = fp->check[CLFILTER_CHECK_KNN_ENABLE] != 0;
    prm.knn.radius         = fp->track[CLFILTER_TRACK_KNN_RADIUS];
    prm.knn.strength       = (float)fp->track[CLFILTER_TRACK_KNN_STRENGTH] * 0.01f;
    prm.knn.lerpC          = (float)fp->track[CLFILTER_TRACK_KNN_LERP] * 0.01f;
    prm.knn.lerp_threshold = (float)fp->track[CLFILTER_TRACK_KNN_TH_LERP] * 0.01f;

    //pmd
    prm.pmd.enable         = fp->check[CLFILTER_CHECK_PMD_ENABLE] != 0;
    prm.pmd.applyCount     = fp->track[CLFILTER_TRACK_PMD_APPLY_COUNT];
    prm.pmd.strength       = (float)fp->track[CLFILTER_TRACK_PMD_STRENGTH];
    prm.pmd.threshold      = (float)fp->track[CLFILTER_TRACK_PMD_THRESHOLD];

    //smooth
    prm.smooth.enable     = fp->check[CLFILTER_CHECK_SMOOTH_ENABLE] != 0;
    prm.smooth.quality    = fp->track[CLFILTER_TRACK_SMOOTH_QUALITY];
    prm.smooth.qp         = fp->track[CLFILTER_TRACK_SMOOTH_QP];

    //unsharp
    prm.unsharp.enable    = fp->check[CLFILTER_CHECK_UNSHARP_ENABLE] != 0;
    prm.unsharp.radius    = fp->track[CLFILTER_TRACK_UNSHARP_RADIUS];
    prm.unsharp.weight    = (float)fp->track[CLFILTER_TRACK_UNSHARP_WEIGHT] * 0.1f;
    prm.unsharp.threshold = (float)fp->track[CLFILTER_TRACK_UNSHARP_THRESHOLD];

    //edgelevel
    prm.edgelevel.enable    = fp->check[CLFILTER_CHECK_EDGELEVEL_ENABLE] != 0;
    prm.edgelevel.strength  = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_STRENGTH];
    prm.edgelevel.threshold = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_THRESHOLD];
    prm.edgelevel.black     = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_BLACK];
    prm.edgelevel.white     = (float)fp->track[CLFILTER_TRACK_EDGELEVEL_WHITE];

    //warpsharp
    prm.warpsharp.enable = fp->check[CLFILTER_CHECK_WARPSHARP_ENABLE] != 0;
    prm.warpsharp.threshold = (float)fp->track[CLFILTER_TRACK_WARPSHARP_THRESHOLD];
    prm.warpsharp.blur      = fp->track[CLFILTER_TRACK_WARPSHARP_BLUR];
    prm.warpsharp.type      = fp->track[CLFILTER_TRACK_WARPSHARP_TYPE];
    prm.warpsharp.depth     = (float)fp->track[CLFILTER_TRACK_WARPSHARP_DEPTH];
    prm.warpsharp.chroma    = fp->track[CLFILTER_TRACK_WARPSHARP_CHROMA];

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
    prm.deband.threCb        = fp->track[CLFILTER_TRACK_DEBAND_CB];
    prm.deband.threCr        = fp->track[CLFILTER_TRACK_DEBAND_CR];
    prm.deband.ditherY       = fp->track[CLFILTER_TRACK_DEBAND_DITHER_Y];
    prm.deband.ditherC       = fp->track[CLFILTER_TRACK_DEBAND_DITHER_C];
    prm.deband.sample        = fp->track[CLFILTER_TRACK_DEBAND_SAMPLE];
    prm.deband.seed          = fp->track[CLFILTER_TRACK_DEBAND_SEED] + 1234;
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

    RGYFrameInfo in;
    //入力フレーム情報
    in.width      = fpip->w;
    in.height     = fpip->h;
    in.pitch[0]   = fpip->max_w * 6;
    in.csp        = RGY_CSP_YC48;
    in.picstruct  = fp->check[CLFILTER_CHECK_FIELD] ? RGY_PICSTRUCT_INTERLACED : RGY_PICSTRUCT_FRAME;
    in.ptr[0]     = (uint8_t *)fpip->ycp_edit;
    in.mem_type   = RGY_MEM_TYPE_CPU;

    RGYFrameInfo out = in;
    out.ptr[0] = (uint8_t *)fpip->ycp_temp;
    if (fp->check[CLFILTER_CHECK_RESIZE_ENABLE]) {
        out.width  = resize_res[cl_exdata.resize_idx].first;
        out.height = resize_res[cl_exdata.resize_idx].second;
    }

    if (clfilter->proc(&out, &in, prm)) {
        return FALSE;
    }
    std::swap(fpip->ycp_edit, fpip->ycp_temp);
    fpip->w = out.width;
    fpip->h = out.height;
    return TRUE;
}
