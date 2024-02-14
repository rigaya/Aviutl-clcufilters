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

#ifndef __CLFILTERS_EXE_H__
#define __CLFILTERS_EXE_H__

#include "clcufilters_exe.h"
#include "rgy_opencl.h"

class clFiltersExe : public clcuFiltersExe {
public:
    clFiltersExe(bool noNVCL);
    virtual ~clFiltersExe();
    virtual RGY_ERR initDevices() override;
    virtual std::string checkDevices() override;
    virtual bool isCUDA() const override { return false; }
protected:
    std::string checkClPlatforms();
    virtual RGY_ERR initDevice(const clfitersSharedPrms *sharedPrms, clFilterChainParam& prm) override;
    std::vector<std::shared_ptr<RGYOpenCLPlatform>> m_clplatforms;
    bool m_noNVCL;
};

#endif // !__CLFILTERS_EXE_H__
