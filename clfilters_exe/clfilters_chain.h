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

#ifndef __CLFILTERS_CHAIN_H__
#define __CLFILTERS_CHAIN_H__

#include "clcufilters_chain.h"
#include "rgy_filter_cl.h"

class clFilterFrameBuffer : public clcuFilterFrameBuffer {
public:
    clFilterFrameBuffer(std::shared_ptr<RGYOpenCLContext> cl);
    virtual ~clFilterFrameBuffer();

    virtual std::unique_ptr<RGYFrame> allocateFrame(const int width, const int height) override;
    virtual void resetMappedFrame(RGYFrame *frame) override;
protected:
    std::shared_ptr<RGYOpenCLContext> m_cl;
};

class clFilterDeviceParam : public clcuFilterDeviceParam {
public:
    int platformID;
    cl_device_type deviceType;

    clFilterDeviceParam() : platformID(0), deviceType(CL_DEVICE_TYPE_GPU) {};
    virtual ~clFilterDeviceParam() {};
};

class clFilterChain : public clcuFilterChain {
public:
    clFilterChain();
    virtual ~clFilterChain();

    virtual RGY_ERR sendInFrame(const RGYFrameInfo *pInputFrame) override;
    virtual RGY_ERR proc(const int frameID, const clFilterChainParam& prm) override;
    virtual RGY_ERR getOutFrame(RGYFrameInfo *pOutputFrame) override;

    virtual int platformID() const override { return m_platformID; }
protected:
    virtual RGY_ERR initDevice(const clcuFilterDeviceParam *param) override;
    virtual void close() override;
    virtual RGY_ERR configureOneFilter(std::unique_ptr<RGYFilterBase>& filter, RGYFrameInfo& inputFrame, const VppType filterType, const int resizeWidth, const int resizeHeight) override;

    std::shared_ptr<RGYOpenCLContext> m_cl;
    int m_platformID;
    RGYOpenCLQueue m_queueSendIn;
};

#endif //__CLFILTERS_CHAIN_H__
