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

#ifndef _CLFILTERS_AUF_H_
#define _CLFILTERS_AUF_H_

#include "rgy_osdep.h"
#include <thread>
#include "rgy_shared_mem.h"
#include "rgy_pipe.h"
#include "rgy_event.h"
#include "rgy_prm.h"
#include "rgy_log.h"
#include "clcufilters_shared.h"
#include "clcufilters_chain_prm.h"
#include "filter.h"

std::string getClfiltersExePath();
std::string getCUfiltersExePath();

class clcuFiltersAufDevices {
public:
    clcuFiltersAufDevices();
    ~clcuFiltersAufDevices();
    int createList();
    const std::vector<std::pair<CL_PLATFORM_DEVICE, tstring>> getPlatforms() const { return m_platforms; }
protected:
    int createList(const tstring& exePath);
    std::vector<std::pair<CL_PLATFORM_DEVICE, tstring>> m_platforms;
};

class clcuFiltersAuf {
public:
    clcuFiltersAuf();
    ~clcuFiltersAuf();
    int runProcess(const HINSTANCE aufHandle, const int maxw, const int maxh, const bool isCUDA);
    void initShared();
    BOOL funcProc(const clFilterChainParam& prm, FILTER *fp, FILTER_PROC_INFO *fpip);
    void setLogLevel(const RGYParamLogLevel& loglevel) { m_log->setLogLevelAll(loglevel); }
    bool isCUDA() const;
protected:
    clfitersSharedMesData *getMessagePtr() { return (clfitersSharedMesData*)m_sharedMessage->ptr(); }
    
    void AddMessage(RGYLogLevel log_level, const tstring &str) {
        if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_CORE)) {
            return;
        }
        auto lines = split(str, _T("\n"));
        for (const auto &line : lines) {
            if (line[0] != _T('\0')) {
                m_log->write(log_level, RGY_LOGT_CORE, (_T("clcufilters[auf]: ") + line + _T("\n")).c_str());
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
    std::unique_ptr<RGYPipeProcess> m_process;
    unique_event m_eventMesStart;
    unique_event m_eventMesEnd;
    std::unique_ptr<RGYSharedMemWin> m_sharedMessage;
    std::unique_ptr<RGYSharedMemWin> m_sharedPrms;
    std::unique_ptr<RGYSharedMemWin> m_sharedFramesIn;
    int m_sharedFramesPitchBytes;
    std::thread m_threadProcOut;
    std::thread m_threadProcErr;
    std::shared_ptr<RGYLog> m_log;
};

#endif // _CLFILTERS_AUF_H_