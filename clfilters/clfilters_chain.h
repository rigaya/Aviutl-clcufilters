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
};

struct clFilterChainParam {
    HMODULE hModule;
    VppColorspace colorspace;
    VppNnedi nnedi;
    VppKnn knn;
    VppPmd pmd;
    VppSmooth smooth;
    RGY_VPP_RESIZE_ALGO resize_algo;
    RGY_VPP_RESIZE_MODE resize_mode;
    VppUnsharp unsharp;
    VppEdgelevel edgelevel;
    VppWarpsharp warpsharp;
    VppDeband deband;
    VppTweak tweak;
    RGYLogLevel log_level;

    clFilterChainParam();
    std::vector<clFilter> getFilterChain(const bool resizeRequired) const;
};

class clFilterChain {
public:
    clFilterChain();
    ~clFilterChain();

    RGY_ERR init(const int platformID, const int deviceID, const cl_device_type device_type, const RGYLogLevel log_level);
    std::string getDeviceName() const;
    RGY_ERR proc(RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame, const clFilterChainParam& prm);
    int platformID() const { return m_platformID; }
    int deviceID() const { return m_deviceID; }
private:
    void close();
    bool resizeRequired(const RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame) const;
    RGY_ERR initOpenCL(const int platformID, const int deviceID, const cl_device_type device_type);
    RGY_ERR allocateBuffer(const RGYFrameInfo *pInputFrame, const RGYFrameInfo *pOutputFrame);
    bool filterChainEqual(const std::vector<clFilter>& objchain) const;
    RGY_ERR filterChainCreate(const RGYFrameInfo *pInputFrame, const RGYFrameInfo *pOutputFrame);
    RGY_ERR configureOneFilter(std::unique_ptr<RGYFilter>& filter, RGYFrameInfo& inputFrame, const clFilter filterType, const int resizeWidth, const int resizeHeight);
    void PrintMes(int logLevel, const TCHAR *format, ...);

    std::shared_ptr<RGYLog> m_log;
    clFilterChainParam m_prm;
    std::shared_ptr<RGYOpenCLContext> m_cl;
    int m_platformID;
    int m_deviceID;
    std::string m_deviceName;
    std::array<std::unique_ptr<RGYCLFrame>, 2> m_dev;
    std::vector<std::pair<clFilter, std::unique_ptr<RGYFilter>>> m_filters;
    std::unique_ptr<RGYConvertCSP> m_convert_yc48_to_yuv444_16;
    std::unique_ptr<RGYConvertCSP> m_convert_yuv444_16_to_yc48;
};


#endif //__CLFILTER_CHAIN_H__
