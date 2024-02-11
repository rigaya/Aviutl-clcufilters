#include <memory>
#include <iostream>
#include "rgy_osdep.h"
#include "rgy_tchar.h"
#include "rgy_log.h"
#include "rgy_event.h"
#include "rgy_shared_mem.h"
#include "clfilters_shared.h"
#include "clfilters_chain.h"
#include "clfilters_chain_prm.h"
#include "clfilters_exe_cmd.h"
#include "clfilters_version.h"
#include "rgy_util.h"
#include "rgy_opencl.h"
#include "rgy_cmd.h"



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
        const auto str = clfilterexe.checkClPlatforms();
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


