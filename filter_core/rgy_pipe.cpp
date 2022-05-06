﻿// -----------------------------------------------------------------------------------------
// QSVEnc/NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2011-2016 rigaya
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
// --------------------------------------------------------------------------------------------

#include "rgy_pipe.h"
#include "rgy_util.h"
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#include <cstring>

RGYPipeProcessWin::RGYPipeProcessWin() {
    memset(&m_pi, 0, sizeof(m_pi));
}

RGYPipeProcessWin::~RGYPipeProcessWin() {

}

void RGYPipeProcessWin::init() {
    close();
}


int RGYPipeProcessWin::startPipes(ProcessPipe *pipes) {
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    if (pipes->stdOut.mode) {
        if (!CreatePipe(&pipes->stdOut.h_read, &pipes->stdOut.h_write, &sa, pipes->stdOut.bufferSize) ||
            !SetHandleInformation(pipes->stdOut.h_read, HANDLE_FLAG_INHERIT, 0))
            return 1;
    }
    if (pipes->stdErr.mode) {
        if (!CreatePipe(&pipes->stdErr.h_read, &pipes->stdErr.h_write, &sa, pipes->stdErr.bufferSize) ||
            !SetHandleInformation(pipes->stdErr.h_read, HANDLE_FLAG_INHERIT, 0))
            return 1;
    }
    if (pipes->stdIn.mode) {
        if (!CreatePipe(&pipes->stdIn.h_read, &pipes->stdIn.h_write, &sa, pipes->stdIn.bufferSize) ||
            !SetHandleInformation(pipes->stdIn.h_write, HANDLE_FLAG_INHERIT, 0))
            return 1;
        if ((pipes->f_stdin = _fdopen(_open_osfhandle((intptr_t)pipes->stdIn.h_write, _O_BINARY), "wb")) == NULL) {
            return 1;
        }
    }
    return 0;
}

int RGYPipeProcessWin::run(const std::vector<const TCHAR *>& args, const TCHAR *exedir, ProcessPipe *pipes, uint32_t priority, bool hidden, bool minimized) {
    BOOL Inherit = FALSE;
    DWORD flag = priority;
    STARTUPINFO si;
    memset(&si, 0, sizeof(STARTUPINFO));
    memset(&m_pi, 0, sizeof(PROCESS_INFORMATION));
    si.cb = sizeof(STARTUPINFO);

    startPipes(pipes);

    if (pipes->stdOut.mode)
        si.hStdOutput = pipes->stdOut.h_write;
    if (pipes->stdErr.mode)
        si.hStdError = (pipes->stdErr.mode == PIPE_MODE_MUXED) ? pipes->stdOut.h_write : pipes->stdErr.h_write;
    if (pipes->stdIn.mode)
        si.hStdInput = pipes->stdIn.h_read;
    si.dwFlags |= STARTF_USESTDHANDLES;
    Inherit = TRUE;
    //flag |= DETACHED_PROCESS; //このフラグによるコンソール抑制よりCREATE_NO_WINDOWの抑制を使用する
    if (minimized) {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow |= SW_SHOWMINNOACTIVE;
    }
    if (hidden)
        flag |= CREATE_NO_WINDOW;

    tstring cmd_line;
    for (auto arg : args) {
        if (arg) {
            cmd_line += tstring(arg) + _T(" ");
        }
    }

    int ret = (CreateProcess(NULL, (TCHAR *)cmd_line.c_str(), NULL, NULL, Inherit, flag, NULL, exedir, &si, &m_pi)) ? 0 : 1;
    m_phandle = m_pi.hProcess;
    if (pipes->stdOut.mode) {
        CloseHandle(pipes->stdOut.h_write);
        if (ret) {
            CloseHandle(pipes->stdOut.h_read);
            pipes->stdOut.mode = PIPE_MODE_DISABLE;
        }
    }
    if (pipes->stdErr.mode) {
        if (pipes->stdErr.mode)
            CloseHandle(pipes->stdErr.h_write);
        if (ret) {
            CloseHandle(pipes->stdErr.h_read);
            pipes->stdErr.mode = PIPE_MODE_DISABLE;
        }
    }
    if (pipes->stdIn.mode) {
        CloseHandle(pipes->stdIn.h_read);
        if (ret) {
            CloseHandle(pipes->stdIn.h_write);
            pipes->stdIn.mode = PIPE_MODE_DISABLE;
        }
    }
    return ret;
}

std::string RGYPipeProcessWin::getOutput(ProcessPipe *pipes) {
    std::string outstr;
    auto read_from_pipe = [&]() {
        DWORD pipe_read = 0;
        if (!PeekNamedPipe(pipes->stdOut.h_read, NULL, 0, NULL, &pipe_read, NULL))
            return -1;
        if (pipe_read) {
            char read_buf[1024] = { 0 };
            ReadFile(pipes->stdOut.h_read, read_buf, sizeof(read_buf) - 1, &pipe_read, NULL);
            outstr += read_buf;
        }
        return (int)pipe_read;
    };

    while (WAIT_TIMEOUT == WaitForSingleObject(m_phandle, 10)) {
        read_from_pipe();
    }
    for (;;) {
        if (read_from_pipe() <= 0) {
            break;
        }
    }
    return outstr;
}

const PROCESS_INFORMATION& RGYPipeProcessWin::getProcessInfo() {
    return m_pi;
}

void RGYPipeProcessWin::close() {
    if (m_pi.hProcess) {
        CloseHandle(m_pi.hProcess);
    }
    if (m_pi.hThread) {
        CloseHandle(m_pi.hThread);
    }
    memset(&m_pi, 0, sizeof(m_pi));
}

bool RGYPipeProcessWin::processAlive() {
    return WAIT_OBJECT_0 == WaitForSingleObject(m_phandle, 0);
}
#endif //defined(_WIN32) || defined(_WIN64)


std::unique_ptr<RGYPipeProcess> createRGYPipeProcess() {
#if defined(_WIN32) || defined(_WIN64)
    auto process = std::make_unique<RGYPipeProcessWin>();
#else
    auto process = std::make_unique<RGYPipeProcessLinux>();
#endif
    return std::move(process);
}