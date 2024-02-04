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

#include "clfilters_auf.h"
#include "rgy_filesystem.h"

clFiltersAuf::clFiltersAuf() :
    m_process(createRGYPipeProcess()),
    m_eventMesStart(unique_event(nullptr, CloseEvent)),
    m_eventMesEnd(unique_event(nullptr, CloseEvent)),
    m_sharedMessage(),
    m_sharedPrms(),
    m_sharedFramesIn(),
    m_sharedFramesPitchBytes(0),
    m_sharedFramesOut(),
    m_threadProcOut(),
    m_threadProcErr(),
    m_platforms(),
    m_log(std::make_shared<RGYLog>(nullptr, RGY_LOG_DEBUG)) {
}
clFiltersAuf::~clFiltersAuf() {
    if (m_process && m_process->processAlive()) {
        // プロセス側に処理開始を通知
        clfitersSharedMesData *message = (clfitersSharedMesData*)m_sharedMessage->ptr();
        message->type = clfitersMes::Abort;
        SetEvent(m_eventMesStart.get());
        // プロセス側の処理終了を待機
        while (WaitForSingleObject(m_eventMesEnd.get(), 100) == WAIT_TIMEOUT && m_process->processAlive()) {
            ;
        }
        // プロセスの終了を待機
        for (int i = 0; i < 10; i++) {
            if (!m_process->processAlive()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        m_process->close();
    }
    m_process.reset();

    if (m_threadProcOut.joinable()) {
        m_threadProcOut.join();
    }
    if (m_threadProcErr.joinable()) {
        m_threadProcErr.join();
    }

    for (size_t i = 0; i < m_sharedFramesIn.size(); i++) {
        m_sharedFramesIn[i].reset();
    }
    m_sharedFramesPitchBytes = 0;
    m_sharedPrms.reset();
    m_sharedMessage.reset();
    m_eventMesEnd.reset();
    m_eventMesStart.reset();

}
void clFiltersAuf::initShared() {
    auto sharedMes = (clfitersSharedMesData *)m_sharedMessage->ptr();
    auto sharedPrms = (clfitersSharedPrms *)m_sharedPrms->ptr();
    initPrms(sharedMes);
    initPrms(sharedPrms);
}
int clFiltersAuf::runProcess(const HINSTANCE aufHandle, const int maxw, const int maxh) {
    const auto aviutlPid = GetCurrentProcessId();
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    m_eventMesStart = CreateEventUnique(&sa, FALSE, FALSE);
    m_eventMesEnd = CreateEventUnique(&sa, FALSE, FALSE);
    m_sharedMessage = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_MESSAGE, aviutlPid).c_str(), sizeof(clfitersSharedMesData));
    m_sharedPrms = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_PRMS, aviutlPid).c_str(), sizeof(clfitersSharedPrms));
    m_sharedFramesPitchBytes = get_shared_frame_pitch(maxw);
    const int frameSize = m_sharedFramesPitchBytes * maxh;
    for (size_t i = 0; i < m_sharedFramesIn.size(); i++) {
        m_sharedFramesIn[i] = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_FRAMES_IN, aviutlPid, i).c_str(), frameSize);
    }
    m_sharedFramesOut = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_FRAMES_OUT, aviutlPid).c_str(), frameSize);
    //初期化
    initShared();
    // プロセスの起動
    char aviutlPath[4096] = { 0 };
    GetModuleFileName(nullptr, aviutlPath, _countof(aviutlPath) - 1);
    auto [ret2, aviutlDir ] = PathRemoveFileSpecFixed(aviutlPath);
    char aufPath[4096] = { 0 };
    GetModuleFileName(aufHandle, aufPath, _countof(aufPath) - 1);
    const auto exePath = PathRemoveExtensionS(aufPath) + ".exe";
    const auto aviutlPidStr = strsprintf("0x%08x", aviutlPid);
    const auto maxWidthStr = strsprintf("%d", maxw);
    const auto maxHeightStr = strsprintf("%d", maxh);
    const auto sizeSharedPrmStr = strsprintf("%d", sizeof(clfitersSharedPrms));
    const auto sizeSharedMesDataStr = strsprintf("%d", sizeof(clfitersSharedMesData));
    const auto sizePIXELYCStr = strsprintf("%d", sizeof(PIXEL_YC));
    const auto eventMesStartStr = strsprintf("%p", m_eventMesStart.get());
    const auto eventMesEndStr = strsprintf("%p", m_eventMesEnd.get());
    const std::vector<const TCHAR*> args = {
        exePath.c_str(),
        _T("--ppid"), aviutlPidStr.c_str(),
        _T("--maxw"), maxWidthStr.c_str(),
        _T("--maxh"), maxHeightStr.c_str(),
        _T("--size-shared-prm"), sizeSharedPrmStr.c_str(),
        _T("--size-shared-mesdata"), sizeSharedMesDataStr.c_str(),
        _T("--size-pixelyc"), sizePIXELYCStr.c_str(),
        _T("--event-mes-start"), eventMesStartStr.c_str(),
        _T("--event-mes-end"), eventMesEndStr.c_str()
    };
    m_process->init(PIPE_MODE_DISABLE, PIPE_MODE_ENABLE, PIPE_MODE_ENABLE);
    int ret = m_process->run(args, aviutlDir.c_str(), 0, true, true);
    if (ret != 0) {
        return ret;
    }
    // プロセスのstdout, stderrの取得スレッドを起動
    m_threadProcOut = std::thread([&]() {
        m_log->write(RGY_LOG_DEBUG, RGY_LOGT_APP, _T("Start thread to receive stdout messages from process.\n"));
        std::vector<uint8_t> buffer;
        while (m_process->stdOutRead(buffer) >= 0) {
            if (buffer.size() > 0) {
                auto str = std::string(buffer.data(), buffer.data() + buffer.size());
                m_log->write(RGY_LOG_INFO, RGY_LOGT_APP, _T("%s"), char_to_tstring(str).c_str());
                buffer.clear();
            }
        }
        m_process->stdOutRead(buffer);
        if (buffer.size() > 0) {
            auto str = std::string(buffer.data(), buffer.data() + buffer.size());
            m_log->write(RGY_LOG_INFO, RGY_LOGT_APP, _T("%s"), char_to_tstring(str).c_str());
            buffer.clear();
        }
        m_log->write(RGY_LOG_DEBUG, RGY_LOGT_APP, _T("Reached process stdout EOF.\n"));
    });

    m_threadProcErr = std::thread([&]() {
        m_log->write(RGY_LOG_DEBUG, RGY_LOGT_APP, _T("Start thread to receive stderr messages from process.\n"));
        std::vector<uint8_t> buffer;
        while (m_process->stdErrRead(buffer) >= 0) {
            if (buffer.size() > 0) {
                auto str = std::string(buffer.data(), buffer.data() + buffer.size());
                m_log->write(RGY_LOG_INFO, RGY_LOGT_APP, _T("%s"), char_to_tstring(str).c_str());
                buffer.clear();
            }
        }
        m_process->stdErrRead(buffer);
        if (buffer.size() > 0) {
            auto str = std::string(buffer.data(), buffer.data() + buffer.size());
            m_log->write(RGY_LOG_INFO, RGY_LOGT_APP, _T("%s"), char_to_tstring(str).c_str());
            buffer.clear();
        }
        m_log->write(RGY_LOG_DEBUG, RGY_LOGT_APP, _T("Reached process stderr EOF.\n"));
    });
    // プロセス初期化処理の終了を待機
    SetEvent(m_eventMesStart.get());
    while (WaitForSingleObject(m_eventMesEnd.get(), 1000) == WAIT_TIMEOUT) {
        if (!m_process->processAlive()) {
            m_log->write(RGY_LOG_ERROR, RGY_LOGT_APP, _T("Process terminated before initialization.\n"));
            return 1;
        }
    }
    // platformとdeviceの情報を取得
    const auto platforms = split(getMessagePtr()->data, _T("\n"));
    for (const auto& pstr : platforms) {
        const auto pdstr = pstr.find(_T("/"));
        CL_PLATFORM_DEVICE pd;
        if (pdstr != std::string::npos) {
            if (sscanf_s(pstr.substr(0, pdstr).c_str(), "%x", &pd.i) != 1) {
                continue;
            }
            m_platforms.push_back({ pd, pstr.substr(pdstr+1) });
        }
    }
    return 0;
}
