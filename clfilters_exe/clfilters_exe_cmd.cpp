#include "rgy_util.h"
#include "rgy_def.h"
#include "rgy_log.h"
#include "rgy_cmd.h"
#include "clfilters_exe_cmd.h"
#include "clfilters_chain_prm.h"

#if 0
void print_cmd_error_unknown_opt(tstring strErrorValue) {
    _ftprintf(stderr, _T("Error: Unknown option: %s\n\n"), strErrorValue.c_str());
}
template<typename T>
void print_cmd_error_invalid_value(tstring strOptionName, tstring strErrorValue, tstring strErrorMessage, const T* list, int list_length) {
    if (!FOR_AUO && strOptionName.length() > 0) {
        if (strErrorValue.length() > 0) {
            if (0 == _tcsnccmp(strErrorValue.c_str(), _T("--"), _tcslen(_T("--")))
                || (strErrorValue[0] == _T('-') && strErrorValue[2] == _T('\0') && cmd_short_opt_to_long(strErrorValue[1]) != nullptr)) {
                _ftprintf(stderr, _T("Error: \"--%s\" requires value.\n\n"), strOptionName.c_str());
            } else {
                tstring str = _T("Error: Invalid value \"") + strErrorValue + _T("\" for \"--") + strOptionName + _T("\"");
                if (strErrorMessage.length() > 0) {
                    str += _T(": ") + strErrorMessage;
                }
                _ftprintf(stderr, _T("%s\n"), str.c_str());
            }
            if (list) {
                _ftprintf(stderr, _T("  Option value should be one of below...\n"));
                tstring str = _T("    ");
                for (int i = 0; list[i].desc && i < list_length; i++) {
                    str += tstring(list[i].desc) + _T(", ");
                    if (str.length() > 70) {
                        _ftprintf(stderr, _T("%s\n"), str.c_str());
                        str = _T("    ");
                    }
                }
                _ftprintf(stderr, _T("%s\n"), str.substr(0, str.length() - 2).c_str());
            }
        } else {
            _ftprintf(stderr, _T("Error: %s for --%s\n\n"), strErrorMessage.c_str(), strOptionName.c_str());
        }
    }
}

void print_cmd_error_invalid_value(tstring strOptionName, tstring strErrorValue, const CX_DESC* list) {
    print_cmd_error_invalid_value(strOptionName, strErrorValue, _T(""), list);
}
void print_cmd_error_invalid_value(tstring strOptionName, tstring strErrorValue, const FEATURE_DESC* list) {
    print_cmd_error_invalid_value(strOptionName, strErrorValue, _T(""), list);
}

void print_cmd_error_invalid_value(tstring strOptionName, tstring strErrorValue) {
    print_cmd_error_invalid_value(strOptionName, strErrorValue, _T(""), (const CX_DESC*)nullptr);
}

void print_cmd_error_invalid_value(tstring strOptionName, tstring strErrorValue, tstring strErrorMessage) {
    print_cmd_error_invalid_value(strOptionName, strErrorValue, strErrorMessage, (const CX_DESC*)nullptr);
}
#endif

int parse_one_arg(const TCHAR* option_name, const TCHAR* strInput[], int& i, [[maybe_unused]] int nArgNum, AviutlAufExeParams *prm) {
    if (IS_OPTION("ppid")) {
        i++;
        uint32_t ppid = 0;
        if (_stscanf_s(strInput[i], _T("0x%08x"), &ppid) == 1) {
            prm->ppid = ppid;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("maxw")) {
        i++;
        int maxw = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &maxw) == 1) {
            prm->max_w = maxw;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("maxh")) {
        i++;
        int maxh = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &maxh) == 1) {
            prm->max_h = maxh;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("size-shared-prm")) {
        i++;
        int size = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &size) == 1) {
            prm->sizeSharedPrm = size;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("size-shared-mesdata")) {
        i++;
        int size = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &size) == 1) {
            prm->sizeSharedMesData = size;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("size-pixelyc")) {
        i++;
        int size = 0;
        if (_stscanf_s(strInput[i], _T("%d"), &size) == 1) {
            prm->sizePIXELYC = size;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("event-mes-start")) {
        i++;
        HANDLE handle = 0;
        if (_stscanf_s(strInput[i], _T("%p"), &handle) == 1) {
            prm->eventMesStart = handle;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("event-mes-end")) {
        i++;
        HANDLE handle = 0;
        if (_stscanf_s(strInput[i], _T("%p"), &handle) == 1) {
            prm->eventMesEnd = handle;
        } else {
            print_cmd_error_invalid_value(option_name, strInput[i], _T("Invalid value"));
            return 1;
        }
        return 0;
    }
    if (IS_OPTION("log-level")) {
        i++;
        RGYParamLogLevel loglevel;
        if (parse_log_level_param(option_name, strInput[i], loglevel) == 0) {
            prm->log_level = loglevel;
        }
    }
    if (IS_OPTION("log")) {
        i++;
        prm->logfile = strInput[i];
        return 0;
    }
    if (IS_OPTION("clinfo")) {
        prm->clinfo = true;
        return 0;
    }
    return 1;
}

int parse_cmd(AviutlAufExeParams& prms, const bool ignore_parse_err, const int nArgNum, const TCHAR** strInput) {
    bool debug_cmd_parser = false;
    for (int i = 1; i < nArgNum; i++) {
        if (tstring(strInput[i]) == _T("--debug-cmd-parser")) {
            debug_cmd_parser = true;
            break;
        }
    }

    if (debug_cmd_parser) {
        for (int i = 1; i < nArgNum; i++) {
            _ftprintf(stderr, _T("arg[%3d]: %s\n"), i, strInput[i]);
        }
    }

    for (int i = 1; i < nArgNum; i++) {
        if (strInput[i] == nullptr) {
            return 1;
        }

        const TCHAR* option_name = nullptr;

        if (strInput[i][0] == _T('|')) {
            break;
        } else if (strInput[i][0] == _T('-')) {
            if (strInput[i][1] == _T('-')) {
                option_name = &strInput[i][2];
            } else if (strInput[i][2] == _T('\0')) {
                if (nullptr == (option_name = cmd_short_opt_to_long(strInput[i][1]))) {
                    print_cmd_error_invalid_value(tstring(), tstring(), strsprintf(_T("Unknown option: \"%s\""), strInput[i]));
                    return 1;
                }
            } else {
                if (ignore_parse_err) continue;
                print_cmd_error_invalid_value(tstring(), tstring(), strsprintf(_T("Invalid option: \"%s\""), strInput[i]));
                return 1;
            }
        }

        if (option_name == nullptr) {
            if (ignore_parse_err) continue;
            print_cmd_error_unknown_opt(strInput[i]);
            return 1;
        }
        if (debug_cmd_parser) {
            _ftprintf(stderr, _T("parsing %3d: %s: "), i, strInput[i]);
        }
        auto sts = parse_one_arg(option_name, strInput, i, nArgNum, &prms);
        if (debug_cmd_parser) {
            _ftprintf(stderr, _T("%s\n"), (sts == 0) ? _T("OK") : _T("ERR"));
        }
        if (!ignore_parse_err && sts != 0) {
            return sts;
        }
    }
    return 0;
}
