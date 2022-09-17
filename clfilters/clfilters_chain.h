﻿// -----------------------------------------------------------------------------------------
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

#ifndef __CLFILTER_CHAIN_H__
#define __CLFILTER_CHAIN_H__

#include <cstdint>
#include <array>
#include <memory>
#include "rgy_prm.h"
#include "rgy_filter.h"
#include "convert_csp.h"
#include "convert_csp_func.h"

enum class clFilter {
    UNKNOWN = 0,
    COLORSPACE,
    NNEDI,
    KNN,
    PMD,
    SMOOTH,
    RESIZE,
    UNSHARP,
    EDGELEVEL,
    WARPSHARP,
    TWEAK,
    DEBAND,
    FILTER_MAX,
};

static const int FILTER_NAME_MAX_LENGTH = 1024;

#if !CLFILTERS_EN
static const char *FILTER_NAME_COLORSPACE = _T("色空間変換");
static const char *FILTER_NAME_NNEDI      = _T("nnedi");
static const char *FILTER_NAME_KNN        = _T("ノイズ除去 (knn)");
static const char *FILTER_NAME_PMD        = _T("ノイズ除去 (pmd)");
static const char *FILTER_NAME_SMOOTH     = _T("ノイズ除去 (smooth)");
static const char *FILTER_NAME_RESIZE     = _T("リサイズ");
static const char *FILTER_NAME_UNSHARP    = _T("unsharp");
static const char *FILTER_NAME_EDGELEVEL  = _T("エッジレベル調整");
static const char *FILTER_NAME_WARPSHARP  = _T("warpsharp");
static const char *FILTER_NAME_TWEAK      = _T("色調補正");
static const char *FILTER_NAME_DEBAND     = _T("バンディング低減");
#else
static const char *FILTER_NAME_COLORSPACE = _T("colorspace");
static const char *FILTER_NAME_NNEDI      = _T("nnedi");
static const char *FILTER_NAME_KNN        = _T("knn");
static const char *FILTER_NAME_PMD        = _T("pmd");
static const char *FILTER_NAME_SMOOTH     = _T("smooth");
static const char *FILTER_NAME_RESIZE     = _T("resize");
static const char *FILTER_NAME_UNSHARP    = _T("unsharp");
static const char *FILTER_NAME_EDGELEVEL  = _T("edgelevel");
static const char *FILTER_NAME_WARPSHARP  = _T("warpsharp");
static const char *FILTER_NAME_TWEAK      = _T("tweak");
static const char *FILTER_NAME_DEBAND     = _T("deband");
#endif

#define FILTER_NAME(x) std::make_pair(FILTER_NAME_ ## x, clFilter::x)

static const std::array<std::pair<const TCHAR*, clFilter>, ((int)clFilter::FILTER_MAX - (int)clFilter::UNKNOWN-1)> filterList = {
    FILTER_NAME(COLORSPACE),
    FILTER_NAME(NNEDI),
    FILTER_NAME(KNN),
    FILTER_NAME(PMD),
    FILTER_NAME(SMOOTH),
    FILTER_NAME(RESIZE),
    FILTER_NAME(UNSHARP),
    FILTER_NAME(EDGELEVEL),
    FILTER_NAME(WARPSHARP),
    FILTER_NAME(TWEAK),
    FILTER_NAME(DEBAND)
};

#undef FILTER_NAME

struct clFilterChainParam {
    HMODULE hModule;
    std::vector<clFilter> filterOrder;
    VppColorspace colorspace;
    VppNnedi nnedi;
    VppSmooth smooth;
    VppKnn knn;
    VppPmd pmd;
    RGY_VPP_RESIZE_ALGO resize_algo;
    RGY_VPP_RESIZE_MODE resize_mode;
    VppUnsharp unsharp;
    VppEdgelevel edgelevel;
    VppWarpsharp warpsharp;
    VppDeband deband;
    VppTweak tweak;
    RGYLogLevel log_level;
    bool log_to_file;

    clFilterChainParam();
    bool operator==(const clFilterChainParam &x) const;
    bool operator!=(const clFilterChainParam &x) const;
    std::vector<clFilter> getFilterChain(const bool resizeRequired) const;
};

class clFilterFrameBuffer {
public:
    static const int bufSize = 4;
    clFilterFrameBuffer(std::shared_ptr<RGYOpenCLContext> cl);
    ~clFilterFrameBuffer();

    void freeFrames();
    void resetCachedFrames();
    RGYCLFrame *get_in(const int width, const int height);
    RGYCLFrame *get_out();
    RGYCLFrame *get_out(const int frameID);
    void in_to_next();
    void out_to_next();
private:
    std::shared_ptr<RGYOpenCLContext> m_cl;
    std::array<std::unique_ptr<RGYCLFrame>, bufSize> m_frame;
    int m_in;
    int m_out;
};

class clFilterChain {
public:
    clFilterChain();
    ~clFilterChain();

    RGY_ERR init(const int platformID, const int deviceID, const cl_device_type device_type, const RGYLogLevel log_level, const bool log_to_file);

    void resetPipeline();
    RGY_ERR sendInFrame(const RGYFrameInfo *pInputFrame);
    RGY_ERR proc(const int frameID, const int outWidth, const int outHeight, const clFilterChainParam& prm);
    RGY_ERR getOutFrame(RGYFrameInfo *pOutputFrame);
    int getNextOutFrameId() const;

    std::string getDeviceName() const { return m_deviceName; }
    int platformID() const { return m_platformID; }
    int deviceID() const { return m_deviceID; }
    const clFilterChainParam& getPrm() const { return m_prm; }
private:
    void close();
    RGY_ERR initOpenCL(const int platformID, const int deviceID, const cl_device_type device_type);
    bool filterChainEqual(const std::vector<clFilter>& objchain) const;
    RGY_ERR filterChainCreate(const RGYFrameInfo *pInputFrame, const int outWidth, const int outHeight);
    RGY_ERR configureOneFilter(std::unique_ptr<RGYFilter>& filter, RGYFrameInfo& inputFrame, const clFilter filterType, const int resizeWidth, const int resizeHeight);
    void PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...);
    tstring printFilterChain(const std::vector<clFilter>& objchain) const;

    std::shared_ptr<RGYLog> m_log;
    clFilterChainParam m_prm;
    std::shared_ptr<RGYOpenCLContext> m_cl;
    int m_platformID;
    int m_deviceID;
    std::string m_deviceName;
    std::unique_ptr<clFilterFrameBuffer> m_frameIn;
    std::unique_ptr<clFilterFrameBuffer> m_frameOut;
    RGYOpenCLQueue m_queueSendIn;
    std::vector<std::pair<clFilter, std::unique_ptr<RGYFilter>>> m_filters;
    std::unique_ptr<RGYConvertCSP> m_convert_yc48_to_yuv444_16;
    std::unique_ptr<RGYConvertCSP> m_convert_yuv444_16_to_yc48;
};


#endif //__CLFILTER_CHAIN_H__
