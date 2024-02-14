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

#include <memory>
#include <iostream>
#include "rgy_osdep.h"
#include "rgy_tchar.h"
#include "rgy_log.h"
#include "rgy_event.h"
#include "rgy_shared_mem.h"
#include "clcufilters_shared.h"
#include "clcufilters_exe_cmd.h"
#include "clcufilters_version.h"
#include "cufilters_chain.h"
#include "cufilters_exe.h"
#include "rgy_util.h"
#include "rgy_cmd.h"
#include "rgy_err.h"

cuFiltersExe::cuFiltersExe() :
    clcuFiltersExe() { }
cuFiltersExe::~cuFiltersExe() {
}

RGY_ERR cuFiltersExe::initDevices() {
    //ひとまず、これまでのすべてのエラーをflush
    cudaGetLastError();

    auto err = cuInit(0);
    return err_to_rgy(err);
}

std::string cuFiltersExe::checkDevices() {
    return checkCUDADevices();
}

std::string cuFiltersExe::checkCUDADevices() {
    std::string devices;

    //ひとまず、これまでのすべてのエラーをflush
    cudaGetLastError();

    int deviceCount = 0;
    auto sts = err_to_rgy(cuDeviceGetCount(&deviceCount));
    if (sts != RGY_ERR_NONE) {
        AddMessage(RGY_LOG_ERROR, _T("cuDeviceGetCount error: %s\n"), get_err_mes(sts));
        return devices;
    }
    AddMessage(RGY_LOG_DEBUG, _T("cuDeviceGetCount: Success, %d.\n"), deviceCount);
    if (deviceCount == 0) {
        return devices;
    }

    for (int idev = 0; idev < deviceCount; idev++) {
        cuDevice dev;
        if (dev.init(idev, m_log) != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to find device #%d.\n"), idev);
        } else {
            auto deviceName = dev.getDeviceName();
            AddMessage(RGY_LOG_DEBUG, _T("Found device %d: %s.\n"), idev, char_to_tstring(deviceName).c_str());
            CL_PLATFORM_DEVICE pd_dev;
            pd_dev.s.platform = CLCU_PLATFORM_CUDA;
            pd_dev.s.device = (int16_t)idev;
            devices += strsprintf("%x/%s\n", pd_dev.i, deviceName.c_str());
        }
    }
    return devices;
}

RGY_ERR cuFiltersExe::initDevice(const clfitersSharedPrms *sharedPrms, clFilterChainParam& prm) {
    m_filter = std::make_unique<cuFilterChain>();
    const auto dev_pd = sharedPrms->pd;
    clcuFilterDeviceParam dev_param;
    dev_param.deviceID = dev_pd.s.device;
    return m_filter->init(&dev_param, prm.log_level.get(RGY_LOGT_APP), prm.log_to_file);
}

int _tmain(const int argc, const TCHAR **argv) {
    AviutlAufExeParams prms;
    if (parse_cmd(prms, false, argc, argv) != 0) {
        return 1;
    }
    if (prms.clinfo) {
        return 0;
    }
    if (prms.checkDevice) {
        cuFiltersExe cufilterexe;
        const auto str = cufilterexe.checkDevices();
        _ftprintf(stdout, _T("%s\n"), str.c_str());
        return 0;
    }
    // 構造体サイズの一致を確認
    if (prms.sizeSharedPrm != sizeof(clfitersSharedPrms)) {
        _ftprintf(stderr, _T("Invalid size for shared param: %d\n"), prms.sizeSharedPrm);
        return 1;
    }
    if (prms.sizeSharedMesData != sizeof(clfitersSharedMesData)) {
        _ftprintf(stderr, _T("Invalid size for shared message data: %d\n"), prms.sizeSharedMesData);
        return 1;
    }
    if (prms.sizePIXELYC != SIZE_PIXEL_YC) {
        _ftprintf(stderr, _T("Invalid size for PIXEL_YC: %d\n"), prms.sizeSharedMesData);
        return 1;
    }
    if (!prms.eventMesStart) {
        _ftprintf(stderr, _T("Event handle(start) not set.\n"));
        return 1;
    }
    if (!prms.eventMesEnd) {
        _ftprintf(stderr, _T("Event handle(end) not set.\n"));
        return 1;
    }
    // 実行開始
    cuFiltersExe cufilterexe;
    cufilterexe.init(prms);
    int ret = cufilterexe.run();
    return ret;
}


