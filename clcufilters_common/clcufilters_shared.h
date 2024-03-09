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

#ifndef __CLCUFILTERS_SHARED_H__
#define __CLCUFILTERS_SHARED_H__

#include <cstdint>
#include "rgy_util.h"
#include "rgy_tchar.h"
#include "clcufilters_chain_prm.h"

static const char *CLFILTER_SHARED_MEM_MESSAGE  = "clfilter_shared_mem_message_%08x";
static const char *CLFILTER_SHARED_MEM_PRMS     = "clfilter_shared_mem_prms_%08x";
static const char *CLFILTER_SHARED_MEM_FRAMES  = "clfilter_shared_mem_frames_in_%08x_%d";
//static const char *CLFILTER_SHARED_MEM_FRAMES_OUT = "clfilter_shared_mem_frames_out_%08x";

enum class clfitersMes : int32_t {
    None = 0,
    FuncProc,
    Abort,
    FIN
};

#pragma pack(push,1)
struct clfitersSharedMesData {
    clfitersMes type;
    int32_t ret;
    char data[16384];
};

struct clfitersSharedFrameInfo {
    int32_t frameId;
    int32_t width;
    int32_t height;
    int32_t pitchBytes;
};

static void initPrms(clfitersSharedFrameInfo *info) {
    info->frameId = -1;
    info->width = 0;
    info->height = 0;
    info->pitchBytes = 0;
}

struct clfitersSharedPrms {
    CL_PLATFORM_DEVICE pd;  // 選択されたdeviceID
    int32_t nextOutFrameId; // 次に出力されるフレーム
    int32_t is_saving;      // 保存中かどうか
    int32_t currentFrameId; // 現在のフレーム
    int32_t frame_n;        // 最大フレーム数
    int32_t frameIn;        // 処理を開始すべきフレーム
    int32_t frameInFin;     // 処理を開始すべきフレーム (終了)
    int32_t frameProc;      // 処理を開始すべきフレーム
    int32_t frameOut;       // 処理を開始すべきフレーム
    int32_t resetPipeLine;  // パイプラインをリセットするかどうか
    clfitersSharedFrameInfo srcFrame[3];  // 入力フレーム
    char prms[16384];
};
#pragma pack(pop)

static void initPrms(clfitersSharedMesData *prms) {
    prms->type = clfitersMes::None;
    memset(prms->data, 0, sizeof(prms->data));
}

static void initPrms(clfitersSharedPrms *prms) {
    prms->nextOutFrameId = -1;
    prms->is_saving = 0;
    prms->currentFrameId = -1;
    for (size_t i = 0; i < _countof(prms->srcFrame); i++) {
        initPrms(&prms->srcFrame[i]);
    }
    prms->frameIn = -1;
    prms->frameProc = -1;
    prms->frameOut = -1;
    prms->pd.s.platform = -1;
    prms->pd.s.device = -1;
    prms->resetPipeLine = false;
    memset(prms->prms, 0, sizeof(prms->prms));
}

static int get_shared_frame_pitch(const int width) {
    return ALIGN(width * SIZE_PIXEL_YC, 64);
}

#endif //__CLCUFILTERS_SHARED_H__

