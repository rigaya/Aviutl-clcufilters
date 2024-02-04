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

#include "clfilters_chain_prm.h"
#include "rgy_cmd.h"
#include "rgy_util.h"

const TCHAR *cmd_short_opt_to_long(const TCHAR short_opt) {
    switch (short_opt) {
    case _T('p'): return _T("ppid");
    default: return nullptr;
    }
}

AviutlAufExeParams::AviutlAufExeParams() :
    log_level(RGY_LOG_INFO),
    logfile(),
    clinfo(false),
    ppid(0),
    max_w(0),
    max_h(0),
    sizeSharedPrm(0),
    sizeSharedMesData(0),
    sizePIXELYC(0),
    eventMesStart(nullptr),
    eventMesEnd(nullptr)
{}
AviutlAufExeParams::~AviutlAufExeParams() {}

clFilterChainParam::clFilterChainParam() :
    hModule(NULL),
    vpp(),
    log_level(RGY_LOG_QUIET),
    log_to_file(false),
    outWidth(),
    outHeight() {

}

bool clFilterChainParam::operator==(const clFilterChainParam &x) const {
    return hModule == x.hModule
        && vpp == x.vpp
        && log_level == x.log_level
        && log_to_file == x.log_to_file
        && outWidth == x.outWidth
        && outHeight == x.outHeight;
}
bool clFilterChainParam::operator!=(const clFilterChainParam &x) const {
    return !(*this == x);
}

tstring clFilterChainParam::genCmd() const {
    RGYParamVpp defaultPrm;
    tstring str = gen_cmd(&vpp, &defaultPrm, false);
    str += _T(" --log-level ") + log_level.to_string();
    if (log_to_file) {
        str += _T(" --log-to-file");
    }
    str += strsprintf(_T(" --out-width %d"), outWidth);
    str += strsprintf(_T(" --out-height %d"), outHeight);
    return str;
}

int ParseOneOption(const TCHAR *option_name, const TCHAR* strInput[], int& i, int nArgNum, clFilterChainParam *pParams, sArgsData *argData) {
    if (IS_OPTION("log-level")) {
        i++;
        RGYParamLogLevel loglevel;
        if (parse_log_level_param(option_name, strInput[i], loglevel) != 0) {
            print_cmd_error_invalid_value(option_name, strInput[i]);
            return 1;
        }
        pParams->log_level = loglevel;
        return 0;
    }
    if (IS_OPTION("log-to-file")) {
        pParams->log_to_file = true;
        return 0;
    }
    if (IS_OPTION("out-width")) {
        i++;
        int value = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &value) != 1) {
            print_cmd_error_invalid_value(option_name, strInput[i]);
            return 1;
        }
        pParams->outWidth = value;
        return 0;
    }
    if (IS_OPTION("out-height")) {
        i++;
        int value = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &value) != 1) {
            print_cmd_error_invalid_value(option_name, strInput[i]);
            return 1;
        }
        pParams->outHeight = value;
        return 0;
    }
    int ret = parse_one_vpp_option(option_name, strInput, i, nArgNum, &pParams->vpp, argData);
    if (ret >= 0) return ret;

    print_cmd_error_unknown_opt(strInput[i]);
    return 1;
#undef IS_OPTION
}

void clFilterChainParam::setPrmFromCmd(const tstring& cmda) {
    const bool ignore_parse_err = true;
    std::wstring cmd = char_to_wstring(cmda);
    int argc = 0;
    auto argvw = CommandLineToArgvW(cmd.c_str(), &argc);
    if (argc <= 1) {
        return;
    }
    const int nArgNum = argc;
    vector<tstring> argv_tstring;
    if (wcslen(argvw[0]) != 0) {
        argv_tstring.push_back(_T("")); // 最初は実行ファイルのパスが入っているのを模擬するため、空文字列を入れておく
    }
    for (int i = 0; i < argc; i++) {
        argv_tstring.push_back(wstring_to_tstring(argvw[i]));
    }
    LocalFree(argvw);

    std::vector<TCHAR *> argv_tchar;
    for (size_t i = 0; i < argv_tstring.size(); i++) {
        argv_tchar.push_back((TCHAR *)argv_tstring[i].data());
    }
    argv_tchar.push_back((TCHAR *)_T("")); // 最後に空白を追加
    const TCHAR **strInput = (const TCHAR **)argv_tchar.data();
    sArgsData argsData;
    for (int i = 1; i < nArgNum; i++) {
        if (strInput[i] == nullptr) {
            return;
        }

        const TCHAR *option_name = nullptr;

        if (strInput[i][0] == _T('|')) {
            break;
        } else if (strInput[i][0] == _T('-')) {
            if (strInput[i][1] == _T('-')) {
                option_name = &strInput[i][2];
            } else if (strInput[i][2] == _T('\0')) {
                if (nullptr == (option_name = cmd_short_opt_to_long(strInput[i][1]))) {
                    print_cmd_error_invalid_value(tstring(), tstring(), strsprintf(_T("Unknown option: \"%s\""), strInput[i]));
                    return;
                }
            }
            else {
                if (ignore_parse_err) continue;
                print_cmd_error_invalid_value(tstring(), tstring(), strsprintf(_T("Invalid option: \"%s\""), strInput[i]));
                return;
            }
        }

        if (option_name == nullptr) {
            if (ignore_parse_err) continue;
            print_cmd_error_unknown_opt(strInput[i]);
            return;
        }
        int sts = ParseOneOption(option_name, strInput, i, nArgNum, this, &argsData);
        if (!ignore_parse_err && sts != 0) {
            return;
        }
    }
}


std::vector<VppType> clFilterChainParam::getFilterChain(const bool resizeRequired) const {
    std::vector<VppType> allFilterOrder = vpp.filterOrder; // 指定の順序
    // 指定の順序にないフィルタを追加
    for (const auto& f : filterList) {
        if (std::find(allFilterOrder.begin(), allFilterOrder.end(), f.second) == allFilterOrder.end()) {
            allFilterOrder.push_back(f.second);
        }
    }
    // 有効なフィルタだけを抽出
    std::vector<VppType> enabledFilterOrder;
    for (const auto filterType : allFilterOrder) {
        if (  (vpp.colorspace.enable && filterType == VppType::CL_COLORSPACE)
           || (vpp.nnedi.enable      && filterType == VppType::CL_NNEDI)
           || (vpp.smooth.enable     && filterType == VppType::CL_DENOISE_SMOOTH)
           || (vpp.knn.enable        && filterType == VppType::CL_DENOISE_KNN)
           || (vpp.pmd.enable        && filterType == VppType::CL_DENOISE_PMD)
           || (resizeRequired        && filterType == VppType::CL_RESIZE)
           || (vpp.unsharp.enable    && filterType == VppType::CL_UNSHARP)
           || (vpp.edgelevel.enable  && filterType == VppType::CL_EDGELEVEL)
           || (vpp.warpsharp.enable  && filterType == VppType::CL_WARPSHARP)
           || (vpp.tweak.enable      && filterType == VppType::CL_TWEAK)
           || (vpp.deband.enable     && filterType == VppType::CL_DEBAND)) {
            enabledFilterOrder.push_back(filterType);
        }
    }
    return enabledFilterOrder;
}