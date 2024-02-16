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

#include "clcufilters_version.h"
#include "clcufilters_chain.h"
#include "clcufilters_chain_prm.h"
#include "rgy_cmd.h"

clcuFilterFrameBuffer::clcuFilterFrameBuffer() :
    m_frame(),
    m_in(0),
    m_out(0) {

}

clcuFilterFrameBuffer::~clcuFilterFrameBuffer() {
    freeFrames();
}

void clcuFilterFrameBuffer::freeFrames() {
    for (auto& f : m_frame) {
        if (f) {
            resetMappedFrame(f.get());
        }
        f.reset();
    }
    m_in = 0;
    m_out = 0;
}

void clcuFilterFrameBuffer::resetCachedFrames() {
    for (auto& f : m_frame) {
        if (f) {
            resetMappedFrame(f.get());
        }
    }
    m_in = 0;
    m_out = 0;
};

RGYFrame *clcuFilterFrameBuffer::get_in(const int width, const int height) {
    if (!m_frame[m_in] || m_frame[m_in]->width() != width || m_frame[m_in]->height() != height) {
        m_frame[m_in] = allocateFrame(width, height);
    }
    return m_frame[m_in].get();
}
RGYFrame *clcuFilterFrameBuffer::get_out() {
    return m_frame[m_out].get();
}
RGYFrame *clcuFilterFrameBuffer::get_out(const int frameID) {
    for (auto& f : m_frame) {
        if (f && f->inputFrameId() == frameID) {
            return f.get();
        }
    }
    return nullptr;
}
void clcuFilterFrameBuffer::in_to_next() { m_in = (m_in + 1) % m_frame.size(); }
void clcuFilterFrameBuffer::out_to_next() { m_out = (m_out + 1) % m_frame.size(); }


clcuFilterChain::clcuFilterChain() :
    m_log(),
    m_prm(),
    m_deviceID(-1),
    m_deviceName(),
    m_frameIn(),
    m_frameOut(),
    m_filters(),
    m_convert_yc48_to_yuv444_16(),
    m_convert_yuv444_16_to_yc48() {

}

clcuFilterChain::~clcuFilterChain() {
    close();
}

void clcuFilterChain::close() {
    m_filters.clear();
    m_frameIn.reset();
    m_frameOut.reset();
    m_deviceName.clear();
    m_convert_yc48_to_yuv444_16.reset();
    m_convert_yuv444_16_to_yc48.reset();
    m_log.reset();
}

void clcuFilterChain::PrintMes(const RGYLogLevel logLevel, const TCHAR *format, ...) {
    va_list args;
    va_start(args, format);

    int len = _vsctprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    vector<TCHAR> buffer(len, 0);
    _vstprintf_s(buffer.data(), len, format, args);
    va_end(args);

    m_log->write_log(logLevel, RGY_LOGT_APP, buffer.data());
}

RGY_ERR clcuFilterChain::init(const clcuFilterDeviceParam *param, const RGYLogLevel log_level, const bool log_to_file) {
    m_log = std::make_shared<RGYLog>(log_to_file ? LOG_FILE_NAME : nullptr, log_level);

    if (auto err = initDevice(param); err != RGY_ERR_NONE) {
        return err;
    }

    m_convert_yc48_to_yuv444_16 = std::make_unique<RGYConvertCSP>();
    if (m_convert_yc48_to_yuv444_16->getFunc(RGY_CSP_YC48, RGY_CSP_YUV444_16, false, RGY_SIMD::SIMD_ALL) == nullptr) {
        PrintMes(RGY_LOG_ERROR, _T("color conversion not supported: %s -> %s.\n"),
                 RGY_CSP_NAMES[RGY_CSP_YC48], RGY_CSP_NAMES[RGY_CSP_YUV444_16]);
        return RGY_ERR_INVALID_COLOR_FORMAT;
    }
    PrintMes(RGY_LOG_INFO, _T("color conversion %s -> %s [%s].\n"),
        RGY_CSP_NAMES[RGY_CSP_YC48], RGY_CSP_NAMES[RGY_CSP_YUV444_16], get_simd_str(m_convert_yc48_to_yuv444_16->getFunc()->simd));

    m_convert_yuv444_16_to_yc48 = std::make_unique<RGYConvertCSP>();
    if (m_convert_yuv444_16_to_yc48->getFunc(RGY_CSP_YUV444_16, RGY_CSP_YC48, false, RGY_SIMD::SIMD_ALL) == nullptr) {
        PrintMes(RGY_LOG_ERROR, _T("unsupported color format conversion, %s -> %s\n"), RGY_CSP_NAMES[RGY_CSP_YUV444_16], RGY_CSP_NAMES[RGY_CSP_YC48]);
        return RGY_ERR_INVALID_COLOR_FORMAT;
    }
    PrintMes(RGY_LOG_INFO, _T("color conversion %s -> %s [%s].\n"),
        RGY_CSP_NAMES[RGY_CSP_YUV444_16], RGY_CSP_NAMES[RGY_CSP_YC48], get_simd_str(m_convert_yuv444_16_to_yc48->getFunc()->simd));
    return RGY_ERR_NONE;
}

tstring clcuFilterChain::printFilterChain(const std::vector<VppType>& filterChain) const {
    tstring str;
    for (auto& filter : filterChain) {
        if (str.length() > 0) str += _T(", ");
        str += vppfilter_type_to_str(filter);
    }
    return str;
}

bool clcuFilterChain::filterChainEqual(const std::vector<VppType>& target) const {
    if (m_filters.size() != target.size()) {
        return false;
    }
    for (size_t ifilter = 0; ifilter < m_filters.size(); ifilter++) {
        if (m_filters[ifilter].first != target[ifilter]) {
            return false;
        }
    }
    return true;
}

RGY_ERR clcuFilterChain::filterChainCreate(const RGYFrameInfo *pInputFrame, const int outWidth, const int outHeight) {
    RGYFrameInfo inputFrame = *pInputFrame;
    for (size_t i = 0; i < _countof(inputFrame.ptr); i++) {
        inputFrame.ptr[i] = nullptr;
    }

    const bool resizeRequired = pInputFrame->width != outWidth || pInputFrame->height != outHeight;
    const auto filterChain = m_prm.getFilterChain(resizeRequired);
    if (!filterChainEqual(filterChain)) {
        PrintMes(RGY_LOG_INFO, _T("clcuFilterChain changed: %s\n"), printFilterChain(filterChain).c_str());

        decltype(m_filters) newFilters;
        newFilters.reserve(filterChain.size());
        for (const auto filterType : filterChain) {
            auto filter = std::make_pair(filterType, std::unique_ptr<RGYFilterBase>());
            for (auto& oldFilter : m_filters) {
                if (oldFilter.first == filterType && oldFilter.second) {
                    filter.second = std::move(oldFilter.second);
                    break;
                }
            }
            newFilters.push_back(std::move(filter));
        }
        m_filters.clear();
        m_filters = std::move(newFilters);
    }
    for (auto& fitler : m_filters) {
        auto err = configureOneFilter(fitler.second, inputFrame, fitler.first, outWidth, outHeight);
        if (err != RGY_ERR_NONE) {
            return err;
        }
    }
    return RGY_ERR_NONE;
}

void clcuFilterChain::resetPipeline() {
    m_frameIn->resetCachedFrames();
    m_frameOut->resetCachedFrames();
    PrintMes(RGY_LOG_DEBUG, _T("clcuFilterChain reset pipeline.\n"));
}

int clcuFilterChain::getNextOutFrameId() const {
    if (!m_frameOut) return -1;

    auto frameDevOut = m_frameOut->get_out();
    return (frameDevOut) ? frameDevOut->inputFrameId() : -1;
}
