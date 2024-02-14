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

#ifndef __CUFILTERS_CHAIN_H__
#define __CUFILTERS_CHAIN_H__

#include <cstdint>
#include <array>
#include <memory>
#include "rgy_prm.h"
#include "NVEncFilter.h"
#include "convert_csp.h"
#include "convert_csp_func.h"
#include "clcufilters_chain.h"
#include "clcufilters_chain_prm.h"

class cuFilterFrameBuffer : public clcuFilterFrameBuffer {
public:
    cuFilterFrameBuffer();
    virtual ~cuFilterFrameBuffer();

    virtual std::unique_ptr<RGYFrame> allocateFrame(const int width, const int height) override;
protected:
};

class cuDevice {
public:
    cuDevice();
    ~cuDevice();
    RGY_ERR init(const int deviceID, std::shared_ptr<RGYLog> log);
    void close();
    CUdevice dev() const { return m_device; }
    const tstring& getDeviceName() const { return m_deviceName; }
    int getDriverVersion() const { return m_cuda_driver_version; }
    std::pair<int, int> getCUDAVer() const { return m_cuda_version; }
protected:
    void PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...);

    std::shared_ptr<RGYLog> m_log;
    CUdevice m_device;
    int m_deviceID;
    tstring m_deviceName;
    int m_cuda_driver_version;
    std::pair<int, int> m_cuda_version;
};

class cuFilterChain : public clcuFilterChain {
public:
    cuFilterChain();
    virtual ~cuFilterChain();

    virtual RGY_ERR sendInFrame(const RGYFrameInfo *pInputFrame) override;
    virtual RGY_ERR proc(const int frameID, const clFilterChainParam& prm) override;
    virtual RGY_ERR getOutFrame(RGYFrameInfo *pOutputFrame) override;
    virtual int platformID() const override { return CLCU_PLATFORM_CUDA; };
private:
    virtual RGY_ERR initDevice(const clcuFilterDeviceParam *param) override;
    virtual void close() override;
    virtual RGY_ERR configureOneFilter(std::unique_ptr<RGYFilterBase>& filter, RGYFrameInfo& inputFrame, const VppType filterType, const int resizeWidth, const int resizeHeight) override;

    std::unique_ptr<cuDevice> m_cuDevice;
    std::unique_ptr<std::remove_pointer<CUcontext>::type, decltype(&cuCtxDestroy)> m_cuCtx;
    std::unique_ptr<CUFrameBuf> m_frameHostIn;
    std::unique_ptr<CUFrameBuf> m_frameHostOut;
    std::unique_ptr<cudaEvent_t, cudaevent_deleter> m_eventIn;
    std::unique_ptr<cudaEvent_t, cudaevent_deleter> m_eventOut;
    std::unique_ptr<cudaStream_t, cudastream_deleter> m_streamIn;
    std::unique_ptr<cudaStream_t, cudastream_deleter> m_streamOut;
};


#endif //__CUFILTERS_CHAIN_H__
