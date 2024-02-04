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
#include "clfilters_chain_prm.h"

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
    RGY_ERR proc(const int frameID, const clFilterChainParam& prm);
    RGY_ERR getOutFrame(RGYFrameInfo *pOutputFrame);
    int getNextOutFrameId() const;

    std::string getDeviceName() const { return m_deviceName; }
    int platformID() const { return m_platformID; }
    int deviceID() const { return m_deviceID; }
    const clFilterChainParam& getPrm() const { return m_prm; }
private:
    void close();
    RGY_ERR initOpenCL(const int platformID, const int deviceID, const cl_device_type device_type);
    bool filterChainEqual(const std::vector<VppType>& objchain) const;
    RGY_ERR filterChainCreate(const RGYFrameInfo *pInputFrame, const int outWidth, const int outHeight);
    RGY_ERR configureOneFilter(std::unique_ptr<RGYFilter>& filter, RGYFrameInfo& inputFrame, const VppType filterType, const int resizeWidth, const int resizeHeight);
    void PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...);
    tstring printFilterChain(const std::vector<VppType>& objchain) const;

    std::shared_ptr<RGYLog> m_log;
    clFilterChainParam m_prm;
    std::shared_ptr<RGYOpenCLContext> m_cl;
    int m_platformID;
    int m_deviceID;
    std::string m_deviceName;
    std::unique_ptr<clFilterFrameBuffer> m_frameIn;
    std::unique_ptr<clFilterFrameBuffer> m_frameOut;
    RGYOpenCLQueue m_queueSendIn;
    std::vector<std::pair<VppType, std::unique_ptr<RGYFilter>>> m_filters;
    std::unique_ptr<RGYConvertCSP> m_convert_yc48_to_yuv444_16;
    std::unique_ptr<RGYConvertCSP> m_convert_yuv444_16_to_yc48;
};


#endif //__CLFILTER_CHAIN_H__
