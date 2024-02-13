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

#include <memory>
#include <iostream>
#include "rgy_osdep.h"
#include "rgy_tchar.h"
#include "rgy_log.h"
#include "rgy_event.h"
#include "rgy_shared_mem.h"
#include "clcufilters_shared.h"
#include "clcufilters_chain_prm.h"
#include "clcufilters_exe_cmd.h"
#include "clcufilters_version.h"
#include "clfilters_exe.h"
#include "clfilters_chain.h"
#include "rgy_util.h"
#include "rgy_opencl.h"
#include "rgy_cmd.h"


clFiltersExe::clFiltersExe() :
    clcuFiltersExe(),
    m_clplatforms() { }
clFiltersExe::~clFiltersExe() {
}

std::string clFiltersExe::checkDevices() {
    return checkClPlatforms();
}

std::string clFiltersExe::checkClPlatforms() {
    if (m_clplatforms.size() == 0) {
        auto log = m_log;
        RGYOpenCL cl(m_log ? m_log : std::make_shared<RGYLog>(nullptr, RGY_LOG_INFO));
        m_clplatforms = cl.getPlatforms(nullptr);
        for (auto& platform : m_clplatforms) {
            platform->createDeviceList(CL_DEVICE_TYPE_GPU);
        }
    }
    std::string devices;
    for (size_t ip = 0; ip < m_clplatforms.size(); ip++) {
        for (int idev = 0; idev < (int)m_clplatforms[ip]->devs().size(); idev++) {
            CL_PLATFORM_DEVICE pd;
            pd.s.platform = (int16_t)ip;
            pd.s.device = (int16_t)idev;
            const auto devInfo = m_clplatforms[ip]->dev(idev).info();
            auto devName = (devInfo.board_name_amd.length() > 0) ? devInfo.board_name_amd : devInfo.name;
            devName = str_replace(devName, "(TM)", "");
            devName = str_replace(devName, "(R)", "");
            devName = str_replace(devName, "  ", " ");
            devices += strsprintf("%x/%s\n", pd.i, devName.c_str());
            AddMessage(RGY_LOG_DEBUG, _T("Found platform %d, device %d: %s.\n"), pd.s.platform, pd.s.device, char_to_tstring(devName).c_str());
        }
    }
    return devices;
}

RGY_ERR clFiltersExe::initDevice(const clfitersSharedPrms *sharedPrms, clFilterChainParam& prm) {
    m_filter = std::make_unique<clFilterChain>();
    const auto dev_pd = sharedPrms->pd;
    clFilterDeviceParam dev_param;
    dev_param.platformID = dev_pd.s.platform;
    dev_param.deviceID = dev_pd.s.device;
    dev_param.deviceType = CL_DEVICE_TYPE_GPU;
    return m_filter->init(&dev_param, prm.log_level.get(RGY_LOGT_APP), prm.log_to_file);
}

int _tmain(const int argc, const TCHAR **argv) {
    AviutlAufExeParams prms;
    if (parse_cmd(prms, false, argc, argv) != 0) {
        return 1;
    }
    if (prms.clinfo) {
        const auto str = getOpenCLInfo(CL_DEVICE_TYPE_GPU);
        _ftprintf(stdout, _T("%s\n"), str.c_str());
        return 0;
    }
    if (prms.checkDevice) {
        clFiltersExe clfilterexe;
        const auto str = clfilterexe.checkDevices();
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
    clFiltersExe clfilterexe;
    clfilterexe.init(prms);
    int ret = clfilterexe.run();
    return ret;
}


