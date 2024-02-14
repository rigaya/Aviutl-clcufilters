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

#ifndef __CLCUFILTERS_EXE_H__
#define __CLCUFILTERS_EXE_H__

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
#include "clcufilters_chain.h"
#include "rgy_util.h"
#include "rgy_cmd.h"

//---------------------------------------------------------------------
//        ラベル
//---------------------------------------------------------------------
#if !CLFILTERS_EN
static const char *LB_WND_OPENCL_UNAVAIL = "フィルタは無効です: OpenCLを使用できません。";
static const char *LB_WND_OPENCL_AVAIL = "OpenCL 有効";
static const char *LB_CX_OPENCL_DEVICE = "デバイス選択";
#else
static const char *LB_WND_OPENCL_UNAVAIL = "Filter disabled, OpenCL could not be used.";
static const char *LB_WND_OPENCL_AVAIL = "OpenCL Enabled";
static const char *LB_CX_OPENCL_DEVICE = "Device";
#endif

class clcuFiltersExe {
public:
    clcuFiltersExe();
    virtual ~clcuFiltersExe();
    int init(AviutlAufExeParams& prms);
    int run();
    void AddMessage(RGYLogLevel log_level, const tstring &str) {
        if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_APP)) {
            return;
        }
        if (log_level >= RGY_LOG_ERROR && m_sharedMessage && m_sharedMessage->is_open()) {
            strcat_s(getMessagePtr()->data, tchar_to_string(str).c_str());
        }
        auto lines = split(str, _T("\n"));
        for (const auto &line : lines) {
            if (line[0] != _T('\0')) {
                m_log->write(log_level, RGY_LOGT_APP, (_T("clfilters[exe]: ") + line + _T("\n")).c_str());
            }
        }
    }
    void AddMessage(RGYLogLevel log_level, const TCHAR *format, ...) {
        if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_CORE)) {
            return;
        }
        va_list args;
        va_start(args, format);
        int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
        tstring buffer;
        buffer.resize(len, _T('\0'));
        _vstprintf_s(&buffer[0], len, format, args);
        va_end(args);
        AddMessage(log_level, buffer);
    }
    virtual RGY_ERR initDevices() = 0;
    virtual std::string checkDevices() = 0;
    virtual bool isCUDA() const = 0;
protected:
    virtual RGY_ERR initDevice(const clfitersSharedPrms *sharedPrms, clFilterChainParam& prm) = 0;
    int funcProc();
    clfitersSharedMesData *getMessagePtr() { return (clfitersSharedMesData*)m_sharedMessage->ptr(); }
    RGYFrameInfo setFrameInfo(const int iframeID, const int width, const int height, void *frame);
    std::unique_ptr<clcuFilterChain> m_filter;
    std::unique_ptr<std::remove_pointer<HANDLE>::type, handle_deleter> m_aviutlHandle;
    HANDLE m_eventMesStart;
    HANDLE m_eventMesEnd;
    std::unique_ptr<RGYSharedMemWin> m_sharedMessage;
    std::unique_ptr<RGYSharedMemWin> m_sharedPrms;
    std::unique_ptr<RGYSharedMemWin> m_sharedFramesIn;
    size_t m_ppid;
    int m_maxWidth;
    int m_maxHeight;
    int m_pitchBytes;
    std::shared_ptr<RGYLog> m_log;
};

#endif // !__CLCUFILTERS_EXE_H__
