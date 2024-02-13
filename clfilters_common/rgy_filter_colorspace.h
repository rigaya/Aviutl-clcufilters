﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
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

#include "rgy_filter_cl.h"
#include "rgy_prm.h"
#include <array>

enum ColorspaceOpType {
    COLORSPACE_OP_TYPE_UNKNOWN,
    COLORSPACE_OP_TYPE_MATRIX,
    COLORSPACE_OP_TYPE_FUNC,
    COLORSPACE_OP_TYPE_F2I,
    COLORSPACE_OP_TYPE_I2F,
    COLORSPACE_OP_TYPE_HDR2SDR,
    COLORSPACE_OP_TYPE_LUT3D,
    COLORSPACE_OP_TYPE_NONE,
};

class ColorspaceOp {
public:
    ColorspaceOp() : m_type(COLORSPACE_OP_TYPE_UNKNOWN) {};
    virtual ~ColorspaceOp() {};
    virtual ColorspaceOpType getType() const { return m_type; };
    virtual std::string print() = 0;
    virtual std::string printInfo() { return ""; }
    virtual bool add(const ColorspaceOp *op) = 0;
protected:
    ColorspaceOpType m_type;
};

struct ColorspaceOpInfo {
    VideoVUIInfo from, to;
    unique_ptr<ColorspaceOp> ops;
    ColorspaceOpInfo() : from(), to(), ops() {};
    ColorspaceOpInfo(VideoVUIInfo csp_from, VideoVUIInfo csp_to, unique_ptr<ColorspaceOp> csp_ops) :
        from(csp_from), to(csp_to), ops(std::move(csp_ops)) {
    };
};

class ColorspaceOpCtrl {
public:
    ColorspaceOpCtrl(shared_ptr<RGYLog> log) : operations(), m_log(log), m_path() {};
    ~ColorspaceOpCtrl() {};

    void addOperation(ColorspaceOpInfo &op);
    void clearOperation() {
        operations.clear();
    }
    RGY_ERR setLUT3D(const VideoVUIInfo& in, const VideoVUIInfo& out, double sdr_source_peak, bool approx_gamma, bool scene_ref, const LUT3DParams& prm, std::vector<uint8_t>& additionalParams, int height);
    RGY_ERR setHDR2SDR(const VideoVUIInfo &in, const VideoVUIInfo &out, double source_peak, bool approx_gamma, bool scene_ref, const HDR2SDRParams &prm, int height);
    RGY_ERR setPath(const VideoVUIInfo &in, const VideoVUIInfo &out, double source_peak, bool approx_gamma, bool scene_ref, int height);
    RGY_ERR setOperation(RGY_CSP csp_in, RGY_CSP csp_out);
    std::string printOpAll() const;
    tstring printInfoAll() const;
    VideoVUIInfo VuiOut() const;
private:
    RGY_ERR addColorspaceOpHDR2SDRHable(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm);
    RGY_ERR addColorspaceOpHDR2SDRMobius(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm);
    RGY_ERR addColorspaceOpHDR2SDRReinhard(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm);
    RGY_ERR addColorspaceOpHDR2SDRBT2390(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm);
    RGY_ERR addColorspaceOpLUT3D(vector<ColorspaceOpInfo>& ops, const VideoVUIInfo& from, const LUT3DParams& prm, std::vector<uint8_t>& additionalParams);
    RGY_ERR addColorspaceOpNclYUV2RGB(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to);
    RGY_ERR addColorspaceOpNclRGB2YUV(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to);
    RGY_ERR addColorspaceOpClYUV2RGB(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak);
    RGY_ERR addColorspaceOpClRGB2YUV(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak);
    RGY_ERR addColorspaceOpGamma2Linear(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak, bool approx_gamma, bool scene_ref);
    RGY_ERR addColorspaceOpLinear2Gamma(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak, bool approx_gamma, bool scene_ref);
    RGY_ERR addColorspaceOpGamut(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to);
    RGY_ERR getNeighboringColorspaces(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &csp, double source_peak, bool approx_gamma, bool scene_ref);
    void AddMessage(RGYLogLevel log_level, const TCHAR *format, ...) {
        if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_VPP)) {
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
    void AddMessage(RGYLogLevel log_level, const tstring &str) {
        if (m_log == nullptr || log_level < m_log->getLogLevel(RGY_LOGT_VPP)) {
            return;
        }
        auto lines = split(str, _T("\n"));
        for (const auto &line : lines) {
            if (line[0] != _T('\0')) {
                m_log->write(log_level, RGY_LOGT_VPP, (_T("ColorspaceOpCtrl: ") + line + _T("\n")).c_str());
            }
        }
    }
    vector<ColorspaceOpInfo> operations;
    shared_ptr<RGYLog> m_log;  //ログ出力
    vector<ColorspaceOpInfo> m_path;
};

class RGYFilterParamColorspace : public RGYFilterParam {
public:
    VppColorspace colorspace;
    RGY_CSP encCsp;
    VideoVUIInfo VuiIn;

    RGYFilterParamColorspace() : colorspace(), encCsp(), VuiIn() {};
    virtual ~RGYFilterParamColorspace() {};
    virtual tstring print() const override;
};

class RGYFilterColorspace : public RGYFilter {
public:
    RGYFilterColorspace(shared_ptr<RGYOpenCLContext> context);
    virtual ~RGYFilterColorspace();
    virtual RGY_ERR init(shared_ptr<RGYFilterParam> pParam, shared_ptr<RGYLog> pPrintMes) override;
    virtual std::string genKernelCode();
    VideoVUIInfo VuiOut() const;
protected:
    virtual RGY_ERR run_filter(const RGYFrameInfo *pInputFrame, RGYFrameInfo **ppOutputFrames, int *pOutputFrameNum, RGYOpenCLQueue &queue, const std::vector<RGYOpenCLEvent> &wait_events, RGYOpenCLEvent *event) override;
    virtual void close() override;
    RGY_ERR check_param(shared_ptr<RGYFilterParamColorspace> prm);
    virtual RGY_ERR procFrame(RGYFrameInfo *pFrame, const RGYFrameInfo *pInputFrame, RGYOpenCLQueue &queue, const std::vector<RGYOpenCLEvent> &wait_events, RGYOpenCLEvent *event);

    bool m_bInterlacedWarn;
    unique_ptr<RGYFilterCspCrop> crop;
    unique_ptr<ColorspaceOpCtrl> opCtrl;
    RGYOpenCLProgramAsync m_colorspace;
    std::vector<uint8_t> additionalParams;
    std::unique_ptr<RGYCLBuf> additionalParamsDev;
};
