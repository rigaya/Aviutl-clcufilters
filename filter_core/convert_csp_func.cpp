// -----------------------------------------------------------------------------------------
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
// ------------------------------------------------------------------------------------------

#include <algorithm>
#include "cpu_info.h"
#include "convert_csp_func.h"

RGYConvertCSPPrm::RGYConvertCSPPrm() :
    abort(false),
    dst(nullptr),
    src(nullptr),
    interlaced(false),
    width(0),
    src_y_pitch_byte(0),
    src_uv_pitch_byte(0),
    dst_y_pitch_byte(0),
    height(0),
    dst_height(0),
    crop(nullptr) {

}


RGYConvertCSP::RGYConvertCSP() : RGYConvertCSP(0, RGYParamThread()) {
}

RGYConvertCSP::RGYConvertCSP(int threads, RGYParamThread threadParam) :
    m_csp(nullptr),
    m_csp_from(RGY_CSP_NA),
    m_csp_to(RGY_CSP_NA),
    m_uv_only(false),
    m_threads(threads),
    m_th(), m_heStart(), m_heFin(), m_heFinCopy(),
    m_threadParam(threadParam), m_prm() {
};

RGYConvertCSP::~RGYConvertCSP() {
    m_prm.abort = true;
    for (size_t i = 0; i < m_heStart.size(); i++) {
        SetEvent(m_heStart[i].get());
    }
    for (size_t i = 0; i < m_th.size(); i++) {
        m_th[i].join();
    }
    m_heFinCopy.clear();
    m_heStart.clear();
    m_heFin.clear();
    m_th.clear();
};
const ConvertCSP *RGYConvertCSP::getFunc(RGY_CSP csp_from, RGY_CSP csp_to, bool uv_only, RGY_SIMD simd) {
    if (m_csp == nullptr
        || (m_csp_from != csp_from || m_csp_to != csp_to || m_uv_only != uv_only)) {
        m_csp_from = csp_from;
        m_csp_to = csp_to;
        m_uv_only = uv_only;
        m_csp = get_convert_csp_func(csp_from, csp_to, uv_only, simd);
    }
    return m_csp;
}

int RGYConvertCSP::run(int interlaced, void **dst, const void **src, int width, int src_y_pitch_byte, int src_uv_pitch_byte, int dst_y_pitch_byte, int height, int dst_height, int *crop) {
    if (m_threads == 0) {
        const int div = (m_csp->simd == RGY_SIMD::NONE) ? 2 : 4;
        const int max = (m_csp->simd == RGY_SIMD::NONE) ? 8 : 4;
        m_threads = (dst_y_pitch_byte % 128 != 0) ? 1 : std::min(max, ((int)get_cpu_info().physical_cores + div) / div);
    }
    if (m_threads > 1 && m_th.size() == 0) {
        m_heFinCopy.clear();
        m_heStart.clear();
        m_heFin.clear();
        for (int ith = 0; ith < m_threads; ith++) {
            auto heStart = std::unique_ptr<void, handle_deleter>(CreateEvent(nullptr, false, false, nullptr), handle_deleter());
            auto heFin = std::unique_ptr<void, handle_deleter>(CreateEvent(nullptr, false, false, nullptr), handle_deleter());
            m_th.push_back(std::thread([heStart = heStart.get(), heFin = heFin.get(), ithId = ith, threadN = m_threads, threadParam = m_threadParam, prm = &m_prm, cspfunc = &m_csp]() {
                threadParam.apply(GetCurrentThread());
                WaitForSingleObject((HANDLE)heStart, INFINITE);
                while (!prm->abort) {
                    (*cspfunc)->func[prm->interlaced](prm->dst, prm->src,
                        prm->width, prm->src_y_pitch_byte, prm->src_uv_pitch_byte, prm->dst_y_pitch_byte,
                        prm->height, prm->dst_height, ithId, threadN, prm->crop);
                    SetEvent((HANDLE)heFin);
                    WaitForSingleObject((HANDLE)heStart, INFINITE);
                }
            }));
            m_heFinCopy.push_back(heFin.get());
            m_heStart.push_back(std::move(heStart));
            m_heFin.push_back(std::move(heFin));
        }
    }
    m_prm.abort = false;
    m_prm.interlaced = interlaced;
    m_prm.dst = dst;
    m_prm.src = src;
    m_prm.width = width;
    m_prm.src_y_pitch_byte = src_y_pitch_byte;
    m_prm.src_uv_pitch_byte = src_uv_pitch_byte;
    m_prm.dst_y_pitch_byte = dst_y_pitch_byte;
    m_prm.height = height;
    m_prm.dst_height = dst_height;
    m_prm.crop = crop;
    for (size_t i = 0; i < m_heStart.size(); i++) {
        SetEvent(m_heStart[i].get());
    }
    if (m_threads == 1) {
        m_csp->func[interlaced](dst, src,
            width, src_y_pitch_byte, src_uv_pitch_byte, dst_y_pitch_byte,
            height, dst_height, 0, 1, crop);
    }
    if (m_th.size() > 0) {
        WaitForMultipleObjects((uint32_t)m_heFinCopy.size(), m_heFinCopy.data(), TRUE, INFINITE);
    }
    return 0;
}
