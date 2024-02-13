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

#ifndef __CLCUFILTERS_CHAIN_H__
#define __CLCUFILTERS_CHAIN_H__

#include <cstdint>
#include <array>
#include <memory>
#include "rgy_prm.h"
#include "rgy_frame.h"
#include "rgy_filter.h"
#include "convert_csp.h"
#include "convert_csp_func.h"
#include "clcufilters_chain_prm.h"

static const TCHAR *LOG_FILE_NAME = "clcufilters.auf.log";

static void copyFramePropWithoutCsp(RGYFrameInfo *dst, const RGYFrameInfo *src) {
    dst->width = src->width;
    dst->height = src->height;
    dst->picstruct = src->picstruct;
    dst->timestamp = src->timestamp;
    dst->duration = src->duration;
    dst->flags = src->flags;
    dst->inputFrameId = src->inputFrameId;
}

class clcuFilterFrameBuffer {
public:
    static const int bufSize = 4;
    clcuFilterFrameBuffer();
    virtual ~clcuFilterFrameBuffer();

    virtual std::unique_ptr<RGYFrame> allocateFrame(const int width, const int height) = 0;
    virtual void resetMappedFrame(RGYFrame *frame) = 0;
    void freeFrames();
    void resetCachedFrames();
    RGYFrame *get_in(const int width, const int height);
    RGYFrame *get_out();
    RGYFrame *get_out(const int frameID);
    void in_to_next();
    void out_to_next();
protected:
    std::array<std::unique_ptr<RGYFrame>, bufSize> m_frame;
    int m_in;
    int m_out;
};

struct clcuFilterDeviceParam {
    int deviceID;

    clcuFilterDeviceParam() : deviceID(0) {};
};

class clcuFilterChain {
public:
    clcuFilterChain();
    virtual ~clcuFilterChain();

    RGY_ERR init(const clcuFilterDeviceParam *param, const RGYLogLevel log_level, const bool log_to_file);

    void resetPipeline();
    virtual RGY_ERR sendInFrame(const RGYFrameInfo *pInputFrame) = 0;
    virtual RGY_ERR proc(const int frameID, const clFilterChainParam& prm) = 0;
    virtual RGY_ERR getOutFrame(RGYFrameInfo *pOutputFrame) = 0;
    int getNextOutFrameId() const;

    int deviceID() const { return m_deviceID; }
    virtual int platformID() const = 0;
    std::string getDeviceName() const { return m_deviceName; }
    const clFilterChainParam& getPrm() const { return m_prm; }
protected:
    virtual RGY_ERR initDevice(const clcuFilterDeviceParam *param) = 0;
    virtual void close() = 0;
    bool filterChainEqual(const std::vector<VppType>& objchain) const;
    RGY_ERR filterChainCreate(const RGYFrameInfo *pInputFrame, const int outWidth, const int outHeight);
    virtual RGY_ERR configureOneFilter(std::unique_ptr<RGYFilterBase>& filter, RGYFrameInfo& inputFrame, const VppType filterType, const int resizeWidth, const int resizeHeight) = 0;
    void PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...);
    tstring printFilterChain(const std::vector<VppType>& objchain) const;

    std::shared_ptr<RGYLog> m_log;
    clFilterChainParam m_prm;
    int m_deviceID;
    std::string m_deviceName;
    std::unique_ptr<clcuFilterFrameBuffer> m_frameIn;
    std::unique_ptr<clcuFilterFrameBuffer> m_frameOut;
    std::vector<std::pair<VppType, std::unique_ptr<RGYFilterBase>>> m_filters;
    std::unique_ptr<RGYConvertCSP> m_convert_yc48_to_yuv444_16;
    std::unique_ptr<RGYConvertCSP> m_convert_yuv444_16_to_yc48;
};


#endif //__CLCUFILTERS_CHAIN_H__
