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

#include "clcufilters_auf.h"
#include "rgy_filesystem.h"

static void print_exe_log(const std::string& mes, RGYLog *log) {
    auto strLines = split(mes, "\r\n");
    RGYLogLevel logLevel = RGY_LOG_INFO;
    for (size_t i = 0; i < strLines.size(); i++) {
        auto& str = strLines[i];
        const bool lastLine = i == strLines.size() - 1;
        if (lastLine && str.length() == 0) break;

        auto levelstrpos = str.find(_T(":"));
        if (levelstrpos != std::string::npos) {
            auto levelstr = str.substr(0, levelstrpos);
            for (const auto& param : RGY_LOG_LEVEL_STR) {
                if (_tcscmp(param.second, levelstr.c_str()) == 0) {
                    logLevel = param.first;
                    str = str.substr(levelstrpos + 1);
                    break;
                }
            }
        }
        log->write(logLevel, RGY_LOGT_APP, _T("%s%s"), char_to_tstring(str).c_str(), lastLine ? _T("") : _T("\n"));
    }
}

std::vector<clcuFiltersAufDevInfo> clcuFiltersAufDevices::createDeviceList(const tstring& exePath) {
    std::string deviceListStr;
    auto proc = createRGYPipeProcess();
    proc->init(PIPE_MODE_DISABLE, PIPE_MODE_ENABLE, PIPE_MODE_DISABLE);
    const std::vector<tstring> args = { exePath, _T("--check-device") };
    if (proc->run(args, nullptr, 0, true, true) != 0) {
        return {};
    }
    deviceListStr = proc->getOutput();
    proc->close();
    proc.reset();

    // platformとdeviceの情報を取得
    std::vector<clcuFiltersAufDevInfo> platform_dev_list;
    const auto platforms = split(deviceListStr, _T("\n"));
    for (const auto& pstr : platforms) {
        const auto pdstr = pstr.find(_T("/"));
        CL_PLATFORM_DEVICE pd;
        if (pdstr != std::string::npos) {
            if (sscanf_s(pstr.substr(0, pdstr).c_str(), "%x", &pd.i) != 1) {
                continue;
            }
            auto devstr = pstr.substr(pdstr + 1);
            const auto ccstr = devstr.find(_T("/"));
            if (ccstr != std::string::npos) {
                std::pair<int, int> cudaver;
                if (sscanf_s(devstr.substr(0, ccstr).c_str(), "%d.%d", &cudaver.first, &cudaver.second) != 2) {
                    continue;
                }
                platform_dev_list.push_back(clcuFiltersAufDevInfo(pd, cudaver, trim(devstr.substr(ccstr + 1))));
            }
        }
    }
    return platform_dev_list;
}

static std::string getClCUfiltersExePath(const tstring& exeName) {
    char aviutlPath[4096] = { 0 };
    GetModuleFileName(nullptr, aviutlPath, _countof(aviutlPath) - 1);
    auto [ret, aviutlDir] = PathRemoveFileSpecFixed(aviutlPath);
    auto exeDir = PathCombineS(PathCombineS(PathCombineS(aviutlDir, "exe_files"), exeName), "x64");
    return PathCombineS(exeDir, exeName + ".exe");
}

std::string getClfiltersExePath() {
    return getClCUfiltersExePath("clfilters");
}

std::string getCUfiltersExePath() {
    return getClCUfiltersExePath("cufilters");
}

clcuFiltersAufDevices::clcuFiltersAufDevices() :
    m_platforms() {
}
clcuFiltersAufDevices::~clcuFiltersAufDevices() {};

int clcuFiltersAufDevices::createList() {
    m_platforms.clear();
    m_platformAsync.clear();
    for (auto& exe : { getClfiltersExePath(), getCUfiltersExePath() }) {
        m_platformAsync.push_back(std::async(std::launch::async, [exe]() { return clcuFiltersAufDevices::createDeviceList(exe); }));
    }
    return 0;
}

const std::vector<clcuFiltersAufDevInfo>& clcuFiltersAufDevices::getPlatforms() {
    if (m_platformAsync.size() > 0) {
        m_platforms.clear();
        for (auto& async : m_platformAsync) {
            if (async.valid()) {
                auto platform_dev_list = async.get();
                m_platforms.insert(m_platforms.end(), platform_dev_list.begin(), platform_dev_list.end());
            }
        }
        m_platformAsync.clear();
    }
    return m_platforms;
}

const clcuFiltersAufDevInfo *clcuFiltersAufDevices::findDevice(const int platform, const int device) {
    const auto& devices = getPlatforms();
    for (const auto& dev : devices) {
        if (dev.pd.s.platform == platform && dev.pd.s.device == device) {
            return &dev;
        }
    }
    return nullptr;
}

clcuFiltersAuf::clcuFiltersAuf() :
    m_process(createRGYPipeProcess()),
    m_eventMesStart(unique_event(nullptr, CloseEvent)),
    m_eventMesEnd(unique_event(nullptr, CloseEvent)),
    m_sharedMessage(),
    m_sharedPrms(),
    m_sharedFrames(),
    m_sharedFramesPitchBytes(0),
    m_threadProcOut(),
    m_threadProcErr(),
    m_log(std::make_shared<RGYLog>(nullptr, RGY_LOG_DEBUG)) {
}
clcuFiltersAuf::~clcuFiltersAuf() {
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

    for (auto& f : m_sharedFrames) {
        f.reset();
    }
    m_sharedFramesPitchBytes = 0;
    m_sharedPrms.reset();
    m_sharedMessage.reset();
    m_eventMesEnd.reset();
    m_eventMesStart.reset();

}
void clcuFiltersAuf::initShared() {
    auto sharedMes = (clfitersSharedMesData *)m_sharedMessage->ptr();
    auto sharedPrms = (clfitersSharedPrms *)m_sharedPrms->ptr();
    initPrms(sharedMes);
    initPrms(sharedPrms);
}

bool clcuFiltersAuf::isCUDA() const {
    if (!m_sharedPrms) return false;
    auto sharedPrms = (const clfitersSharedPrms *)m_sharedPrms->ptr();
    return sharedPrms->pd.s.platform == CLCU_PLATFORM_CUDA;
}

int clcuFiltersAuf::runProcess(const HINSTANCE aufHandle, const int maxw, const int maxh, const bool isCUDA) {
    const auto aviutlPid = GetCurrentProcessId();
    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    m_eventMesStart = CreateEventUnique(&sa, FALSE, FALSE);
    m_eventMesEnd = CreateEventUnique(&sa, FALSE, FALSE);
    if (!m_eventMesStart || !m_eventMesEnd) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to create event handles.\n"));
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Created event handles.\n"));

    m_sharedMessage = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_MESSAGE, aviutlPid).c_str(), sizeof(clfitersSharedMesData));
    if (!m_sharedMessage || !m_sharedMessage->is_open()) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to open shared mem for messages.\n"));
        m_sharedMessage.reset();
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Opened shared mem for messages.\n"));

    m_sharedPrms = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_PRMS, aviutlPid).c_str(), sizeof(clfitersSharedPrms));
    if (!m_sharedPrms || !m_sharedPrms->is_open()) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to open shared mem for parameters.\n"));
        m_sharedPrms.reset();
        return 1;
    }
    AddMessage(RGY_LOG_DEBUG, _T("Opened shared mem for parameters.\n"));

    m_sharedFramesPitchBytes = get_shared_frame_pitch(maxw);
    const int frameSize = m_sharedFramesPitchBytes * maxh;
    AddMessage(RGY_LOG_DEBUG, _T("Frame max %dx%d, pitch %d, size %d.\n"), maxw, maxh, m_sharedFramesPitchBytes, frameSize);

    for (size_t i = 0; i < m_sharedFrames.size(); i++) {
        m_sharedFrames[i] = std::make_unique<RGYSharedMemWin>(strsprintf(CLFILTER_SHARED_MEM_FRAMES, aviutlPid, i).c_str(), frameSize);
        if (!m_sharedFrames[i] || !m_sharedFrames[i]->is_open()) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to open shared mem for frame(in).\n"));
            return 1;
        }
        AddMessage(RGY_LOG_DEBUG, _T("Opened shared mem for frame(%d).\n"), i);
    }

    //初期化
    initShared();

    // exeのパスを取得
    const auto exePath = (isCUDA) ? getCUfiltersExePath() : getClfiltersExePath();
    if (!rgy_file_exists(exePath)) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to find exe: %s.\n"), exePath.c_str());
        return 1;
    }

    // コマンドライン作成
    const std::vector<tstring> args = {
        exePath.c_str(),
        _T("--ppid"), strsprintf("0x%08x", aviutlPid),
        _T("--maxw"), strsprintf("%d", maxw),
        _T("--maxh"), strsprintf("%d", maxh),
        _T("--size-shared-prm"), strsprintf("%d", sizeof(clfitersSharedPrms)),
        _T("--size-shared-mesdata"), strsprintf("%d", sizeof(clfitersSharedMesData)),
        _T("--size-pixelyc"), strsprintf("%d", sizeof(PIXEL_YC)),
        _T("--event-mes-start"), strsprintf("%p", m_eventMesStart.get()),
        _T("--event-mes-end"), strsprintf("%p", m_eventMesEnd.get())
    };
    tstring cmd_line;
    for (const auto& arg : args) {
        if (!arg.empty()) {
            cmd_line += tstring(arg) + _T(" ");
        }
    }
    // プロセスの起動
    AddMessage(RGY_LOG_INFO, _T("Run: %s.\n"), cmd_line.c_str());
    m_process->init(PIPE_MODE_DISABLE, PIPE_MODE_ENABLE, PIPE_MODE_ENABLE);
    int ret = m_process->run(args, nullptr, 0, true, true);
    if (ret != 0) {
        AddMessage(RGY_LOG_ERROR, _T("Failed to run process: %s.\n"), cmd_line.c_str());
        return ret;
    }
    // プロセスのstdout, stderrの取得スレッドを起動
    m_threadProcOut = std::thread([&]() {
        AddMessage(RGY_LOG_DEBUG, _T("Start thread to receive stdout messages from process.\n"));
        std::vector<uint8_t> buffer;
        while (m_process->stdOutRead(buffer) >= 0) {
            if (buffer.size() > 0) {
                auto str = std::string(buffer.data(), buffer.data() + buffer.size());
                print_exe_log(str, m_log.get());
                buffer.clear();
            }
        }
        m_process->stdOutRead(buffer);
        if (buffer.size() > 0) {
            auto str = std::string(buffer.data(), buffer.data() + buffer.size());
            print_exe_log(str, m_log.get());
            buffer.clear();
        }
        AddMessage(RGY_LOG_DEBUG, _T("Reached process stdout EOF.\n"));
    });

    m_threadProcErr = std::thread([&]() {
        AddMessage(RGY_LOG_DEBUG, _T("Start thread to receive stderr messages from process.\n"));
        std::vector<uint8_t> buffer;
        while (m_process->stdErrRead(buffer) >= 0) {
            if (buffer.size() > 0) {
                auto str = std::string(buffer.data(), buffer.data() + buffer.size());
                print_exe_log(str, m_log.get());
                buffer.clear();
            }
        }
        m_process->stdErrRead(buffer);
        if (buffer.size() > 0) {
            auto str = std::string(buffer.data(), buffer.data() + buffer.size());
            print_exe_log(str, m_log.get());
            buffer.clear();
        }
        AddMessage(RGY_LOG_DEBUG, _T("Reached process stderr EOF.\n"));
    });

    //デバッグ用
    SetEvent(m_eventMesStart.get());

    // プロセス初期化処理の終了を待機
    while (WaitForSingleObject(m_eventMesEnd.get(), 1000) == WAIT_TIMEOUT) {
        if (!m_process->processAlive()) {
            AddMessage(RGY_LOG_ERROR, _T("Process terminated before initialization.\n"));
            return 1;
        }
    }
    return 0;
}
