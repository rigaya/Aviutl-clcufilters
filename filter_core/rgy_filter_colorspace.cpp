﻿// -----------------------------------------------------------------------------------------
// RGY by rigaya
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

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <map>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include "rgy_filter_colorspace.h"
#include "rgy_filter_colorspace_func.h"
#include "rgy_resource.h"
#include "rgy_filesystem.h"

static const int COLORSPACE_BLOCK_X = 64;
static const int COLORSPACE_BLOCK_Y = 4;

using std::pair;
using std::make_pair;
using std::make_unique;

static const auto primXYList = make_array<std::pair<CspColorprim, mat3x3>>(
    make_pair(RGY_PRIM_BT470_M,   mat3x3(0.670, 0.330, 0.0,   0.210, 0.710, 0.0,   0.140, 0.080, 0.0) ),
    make_pair(RGY_PRIM_BT470_BG,  mat3x3(0.640, 0.330, 0.0,   0.290, 0.600, 0.0,   0.150, 0.060, 0.0) ),
    make_pair(RGY_PRIM_BT709,     mat3x3(0.640, 0.330, 0.0,   0.300, 0.600, 0.0,   0.150, 0.060, 0.0) ),
    make_pair(RGY_PRIM_FILM,      mat3x3(0.681, 0.319, 0.0,   0.243, 0.692, 0.0,   0.145, 0.049, 0.0) ),
    make_pair(RGY_PRIM_BT2020,    mat3x3(0.708, 0.292, 0.0,   0.170, 0.797, 0.0,   0.131, 0.046, 0.0) ),
    make_pair(RGY_PRIM_ST170_M,   mat3x3(0.630, 0.340, 0.0,   0.310, 0.595, 0.0,   0.155, 0.070, 0.0) ),
    make_pair(RGY_PRIM_ST240_M,   mat3x3(0.630, 0.340, 0.0,   0.310, 0.595, 0.0,   0.155, 0.070, 0.0) ),
    make_pair(RGY_PRIM_ST431_2,   mat3x3(0.680, 0.320, 0.0,   0.265, 0.690, 0.0,   0.150, 0.060, 0.0) ),
    make_pair(RGY_PRIM_ST432_1,   mat3x3(0.680, 0.320, 0.0,   0.265, 0.690, 0.0,   0.150, 0.060, 0.0) ), // RGY_PRIM_ST431_2 と同じ
    make_pair(RGY_PRIM_EBU3213_E, mat3x3(0.630, 0.340, 0.0,   0.295, 0.605, 0.0,   0.155, 0.077, 0.0) )
);

MAP_PAIR_0_1(primXY, prim, CspColorprim, mat, mat3x3, primXYList, RGY_PRIM_UNSPECIFIED, mat3x3());

typedef std::pair<double, double> primXYpair;
static const auto primIllumList = make_array<std::pair<CspColorprim, primXYpair>>(
    make_pair(RGY_PRIM_BT470_M, make_pair(0.31, 0.316)),
    make_pair(RGY_PRIM_FILM,    make_pair(0.31, 0.316)),
    make_pair(RGY_PRIM_ST431_2, make_pair(0.314, 0.351)),
    make_pair(RGY_PRIM_ST428,   make_pair(1.0 / 3.0, 1.0 / 3.0))
);
MAP_PAIR_0_1(primIllum, prim, CspColorprim, illum, primXYpair, primIllumList, RGY_PRIM_UNSPECIFIED, make_pair(0.3127, 0.3290));


mat3x3 genMatrix(const double r, const double b) {
    const double g = 1.0 - (r + b);
    const double u = 0.5 / (1.0 - b);
    const double v = 0.5 / (1.0 - r);
    return mat3x3(
                    r,      g,             b,
               -r * u, -g * u, (1.0 - b) * u,
        (1.0 - r) * v, -g * v,        -b * v
    );
}

mat3x3 genMatrix(std::pair<double, double> pair) {
    return genMatrix(pair.first, pair.second);
}

mat3x3 matrixRGB2YUV(CspMatrix matrix) {
    switch (matrix) {
    case RGY_MATRIX_YCGCO:
        return mat3x3(
         0.25, 0.5,  0.25,
        -0.25, 0.5, -0.25,
          0.5, 0.0,  -0.5);
    case RGY_MATRIX_2100_LMS:
        return mat3x3(
            1688.0 / 4096.0, 2146.0 / 4096.0,  262.0 / 4096.0,
             683.0 / 4096.0, 2951.0 / 4096.0,  462.0 / 4096.0,
              99.0 / 4096.0,  309.0 / 4096.0, 3688.0 / 4096.0);
    case RGY_MATRIX_RGB:       return genMatrix(0.0,       0.0);
    case RGY_MATRIX_BT709:     return genMatrix(0.2126, 0.0722);
    case RGY_MATRIX_FCC:       return genMatrix(0.3,      0.11);
    case RGY_MATRIX_BT470_BG:
    case RGY_MATRIX_ST170_M:   return genMatrix(0.299,   0.114);
    case RGY_MATRIX_ST240_M:   return genMatrix(0.212,   0.087);
    case RGY_MATRIX_BT2020_NCL:
    case RGY_MATRIX_BT2020_CL: return genMatrix(0.2627, 0.0593);
    default:                   return mat3x3();
    }
}

mat3x3 matrixYUV2RGB(CspMatrix matrix) {
    return matrixRGB2YUV(matrix).inv();
}

const vec3 xy2xyz(double x, double y) {
    return vec3(x / y, 1.0, (1.0 - (x + y)) / y);
};

vec3 getWhitePoint(CspColorprim prim) {
    return xy2xyz(primIllum_prim_to_illum(prim).first, primIllum_prim_to_illum(prim).second);
}

mat3x3 genMatrixfromPrim(CspColorprim prim) {
    const auto primXY = primXY_prim_to_mat(prim);
    const auto r = xy2xyz(primXY(0,0), primXY(0,1));
    const auto g = xy2xyz(primXY(1,0), primXY(1,1));
    const auto b = xy2xyz(primXY(2,0), primXY(2,1));
    const auto w = getWhitePoint(prim);
    const auto x = vec3(r(0), g(0), b(0));
    const auto y = vec3(r(1), g(1), b(1));
    const auto z = vec3(r(2), g(2), b(2));
    return genMatrix(
        w.dot(g.cross(b)) / x.dot(y.cross(z)),
        w.dot(r.cross(g)) / x.dot(y.cross(z)));
}

mat3x3 matrixRGB2YUVfromPrim(CspColorprim prim) {
    switch (prim) {
    case RGY_PRIM_BT709:  return matrixRGB2YUV(RGY_MATRIX_BT709);
    case RGY_PRIM_BT2020: return matrixRGB2YUV(RGY_MATRIX_BT2020_NCL);
    default: return genMatrixfromPrim(prim);
    }
}

mat3x3 matrixYUV2RGBfromPrim(CspColorprim prim) {
    return matrixRGB2YUVfromPrim(prim).inv();
}

mat3x3 getPrimXYZ(CspColorprim prim) {
    const auto primXY = primXY_prim_to_mat(prim);
    const auto r = xy2xyz(primXY(0, 0), primXY(0, 1));
    const auto g = xy2xyz(primXY(1, 0), primXY(1, 1));
    const auto b = xy2xyz(primXY(2, 0), primXY(2, 1));
    return mat3x3(r,g,b).trans();
}

mat3x3 matrixGamutRGB2XYZ(CspColorprim prim) {
    if (prim == RGY_PRIM_ST428)
        return mat3x3::identity();
    auto xyz = getPrimXYZ(prim);
    vec3 s = getPrimXYZ(prim).inv() * getWhitePoint(prim);
    const auto x = vec3(xyz(0,0), xyz(0,1), xyz(0,2));
    const auto y = vec3(xyz(1,0), xyz(1,1), xyz(1,2));
    const auto z = vec3(xyz(2,0), xyz(2,1), xyz(2,2));
    return mat3x3(x.amdal(s), y.amdal(s), z.amdal(s));
}

mat3x3 matrixGamutXYZ2RGB(CspColorprim prim) {
    if (prim == RGY_PRIM_ST428)
        return mat3x3::identity();
    return matrixGamutRGB2XYZ(prim).inv();
}

mat3x3 white_point_adaptation_matrix(CspColorprim in, CspColorprim out) {
    const vec3 wIn = getWhitePoint(in);
    const vec3 wOut = getWhitePoint(out);

    if (wIn == wOut)
        return mat3x3::identity();

    const mat3x3 bradford(
        0.8951, 0.2664, -0.1614,
        -0.7502, 1.7135, 0.0367,
        0.0389, -0.0685, 1.0296
    );

    const vec3 rgb_in = bradford * wIn;
    const vec3 rgb_out = bradford * wOut;

    mat3x3 m;
    m(0,0) = rgb_out(0) / rgb_in(0);
    m(1,1) = rgb_out(1) / rgb_in(1);
    m(2,2) = rgb_out(2) / rgb_in(2);
    return bradford.inv() * m * bradford;
}

bool transferEquivbt709(CspTransfer transfer) {
    const auto list = make_array<CspTransfer>(RGY_TRANSFER_BT709, RGY_TRANSFER_BT601, RGY_TRANSFER_BT2020_10, RGY_TRANSFER_BT2020_12);
    return std::find(list.begin(), list.end(), transfer) != list.end();
}

bool useDisplayReferredB67(const VideoVUIInfo &params, bool approx_gamma, bool scene_ref) {
    return params.colorprim != RGY_PRIM_UNSPECIFIED && !approx_gamma && !scene_ref;
}

typedef float (*gamma_func)(float);

struct TransferFunc {
    gamma_func to_linear;
    std::string to_linear_str;
    gamma_func to_gamma;
    std::string to_gamma_str;
    double to_linear_scale;
    double to_gamma_scale;
};

TransferFunc getTrasferFunc(CspTransfer transfer, double peak_luminance, bool scene_referred) {

    const double ST2084_PEAK_LUMINANCE = 10000.0; // Units of cd/m^2.

#define SET_FUNC(functype, funcname) { \
    func.functype = (funcname); \
    func.functype ## _str = #funcname; \
}

    TransferFunc func;

    func.to_linear_scale = 1.0;
    func.to_gamma_scale = 1.0;

    switch (transfer) {
    case RGY_TRANSFER_LOG_100:
        SET_FUNC(to_linear, log100_inverse_oetf);
        SET_FUNC(to_gamma, log100_oetf);
        break;
    case RGY_TRANSFER_LOG_316:
        SET_FUNC(to_linear, log316_inverse_oetf);
        SET_FUNC(to_gamma, log316_oetf);
        break;
    case RGY_TRANSFER_BT709:
        if (scene_referred) {
            SET_FUNC(to_linear, rec_709_inverse_oetf);
            SET_FUNC(to_gamma, rec_709_oetf);
        } else {
            SET_FUNC(to_linear, rec_1886_eotf);
            SET_FUNC(to_gamma, rec_1886_inverse_eotf);
        }
        break;
    case RGY_TRANSFER_BT470_M:
        SET_FUNC(to_linear, rec_470m_oetf);
        SET_FUNC(to_gamma, rec_470m_inverse_oetf);
        break;
    case RGY_TRANSFER_BT470_BG:
        SET_FUNC(to_linear, rec_470bg_oetf);
        SET_FUNC(to_gamma, rec_470bg_inverse_oetf);
        break;
    case RGY_TRANSFER_ST240_M:
        if (scene_referred) {
            SET_FUNC(to_linear, smpte_240m_inverse_oetf);
            SET_FUNC(to_gamma, smpte_240m_oetf);
        } else {
            SET_FUNC(to_linear, rec_1886_eotf);
            SET_FUNC(to_gamma, rec_1886_inverse_eotf);
        }
        break;
    case RGY_TRANSFER_IEC61966_2_4:
        if (scene_referred) {
            SET_FUNC(to_linear, xvycc_inverse_oetf);
            SET_FUNC(to_gamma, xvycc_oetf);
        } else {
            SET_FUNC(to_linear, xvycc_eotf);
            SET_FUNC(to_gamma, xvycc_inverse_eotf);
        }
        break;
    case RGY_TRANSFER_IEC61966_2_1:
        SET_FUNC(to_linear, srgb_eotf);
        SET_FUNC(to_gamma, srgb_inverse_eotf);
        break;
    case RGY_TRANSFER_ST2084:
        if (scene_referred) {
            SET_FUNC(to_linear, st_2084_inverse_oetf);
            SET_FUNC(to_gamma, st_2084_oetf);
        } else {
            SET_FUNC(to_linear, st_2084_eotf);
            SET_FUNC(to_gamma, st_2084_inverse_eotf);
        }
        func.to_linear_scale = ST2084_PEAK_LUMINANCE / peak_luminance;
        func.to_gamma_scale = peak_luminance / ST2084_PEAK_LUMINANCE;
        break;
    case RGY_TRANSFER_ARIB_B67:
        if (scene_referred) {
            SET_FUNC(to_linear, arib_b67_inverse_oetf);
            SET_FUNC(to_gamma, arib_b67_oetf);
        } else {
            SET_FUNC(to_linear, arib_b67_eotf);
            SET_FUNC(to_gamma, arib_b67_inverse_eotf);

        }
        func.to_linear_scale = scene_referred ? 12.0f : static_cast<float>(1000.0 / peak_luminance);
        func.to_gamma_scale = scene_referred ? 1.0f / 12.0f : static_cast<float>(peak_luminance / 1000.0);
        break;
    default:
        func.to_linear = nullptr;
        func.to_gamma = nullptr;
        break;
    }
#undef SET_FUNC
    return func;
}

class ColorspaceOpNone : public ColorspaceOp {
public:
    ColorspaceOpNone() { m_type = COLORSPACE_OP_TYPE_NONE; };
    virtual ~ColorspaceOpNone() {};
    virtual std::string print() { return ""; }
    virtual bool add(const ColorspaceOp* op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
};

class ColorspaceOpMatrix : public ColorspaceOp {
public:
    ColorspaceOpMatrix() {};
    ColorspaceOpMatrix(const mat3x3 &matrix) : m(matrix) { m_type = COLORSPACE_OP_TYPE_MATRIX; }
    virtual ~ColorspaceOpMatrix() {};
    virtual const mat3x3 &matrix() const {
        return m;
    }
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op);
protected:
    mat3x3 m;
};

class ColorspaceOpGammaFunc : public ColorspaceOp {
public:
    ColorspaceOpGammaFunc() {};
    ColorspaceOpGammaFunc(const TransferFunc &transferfunc) : func(transferfunc) { m_type = COLORSPACE_OP_TYPE_FUNC; };
    virtual ~ColorspaceOpGammaFunc() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    TransferFunc func;
};

class ColorspaceOpInvGammaFunc : public ColorspaceOp {
public:
    ColorspaceOpInvGammaFunc() {};
    ColorspaceOpInvGammaFunc(const TransferFunc &transferfunc) : func(transferfunc) { m_type = COLORSPACE_OP_TYPE_FUNC; };
    virtual ~ColorspaceOpInvGammaFunc() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    TransferFunc func;
};

class ColorspaceOpAribB67 : public ColorspaceOp {
public:
    ColorspaceOpAribB67() : m_kr(0.0), m_kg(0.0), m_kb(0.0), m_scale(0.0) {};
    ColorspaceOpAribB67(double kr, double kg, double kb, double scale) : m_kr(kr), m_kg(kg), m_kb(kb), m_scale(scale) { m_type = COLORSPACE_OP_TYPE_FUNC; };
    virtual ~ColorspaceOpAribB67() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_kr, m_kg, m_kb, m_scale;
};

class ColorspaceOpInvAribB67 : public ColorspaceOp {
public:
    ColorspaceOpInvAribB67() : m_kr(0.0), m_kg(0.0), m_kb(0.0), m_scale(0.0) {};
    ColorspaceOpInvAribB67(double kr, double kg, double kb, double scale) : m_kr(kr), m_kg(kg), m_kb(kb), m_scale(scale) { m_type = COLORSPACE_OP_TYPE_FUNC; };
    virtual ~ColorspaceOpInvAribB67() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_kr, m_kg, m_kb, m_scale;
};

class ColorspaceOpCL2RGB : public ColorspaceOp {
public:
    ColorspaceOpCL2RGB() : m_kr(0.0), m_kg(0.0), m_kb(0.0), m_scale(0.0), m_nb(0.0), m_pb(0.0), m_nr(0.0), m_pr(0.0), m_func() {};
    ColorspaceOpCL2RGB(double kr, double kg, double kb, double scale, const TransferFunc &transferfunc) : m_kr(kr), m_kg(kg), m_kb(kb), m_scale(scale), m_nb(0.0), m_pb(0.0), m_nr(0.0), m_pr(0.0), m_func(transferfunc) {
        m_type = COLORSPACE_OP_TYPE_FUNC;
        m_nb = m_func.to_gamma(1.0f - (float)kb);
        m_pb = 1.0f - m_func.to_gamma((float)kb);
        m_nr = m_func.to_gamma(1.0f - (float)kr);
        m_pr = 1.0f - m_func.to_gamma((float)kr);
    };
    virtual ~ColorspaceOpCL2RGB() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_kr, m_kg, m_kb, m_scale;
    float m_nb, m_pb, m_nr, m_pr;
    const TransferFunc m_func;
};

class ColorspaceOpCL2YUV : public ColorspaceOp {
public:
    ColorspaceOpCL2YUV() : m_kr(0.0), m_kg(0.0), m_kb(0.0), m_scale(0.0), m_nb(0.0), m_pb(0.0), m_nr(0.0), m_pr(0.0), m_func() {};
    ColorspaceOpCL2YUV(double kr, double kg, double kb, double scale, const TransferFunc &transferfunc) : m_kr(kr), m_kg(kg), m_kb(kb), m_scale(scale), m_nb(0.0), m_pb(0.0), m_nr(0.0), m_pr(0.0), m_func(transferfunc) {
        m_type = COLORSPACE_OP_TYPE_FUNC;
        m_nb = m_func.to_gamma(1.0f - (float)kb);
        m_pb = 1.0f - m_func.to_gamma((float)kb);
        m_nr = m_func.to_gamma(1.0f - (float)kr);
        m_pr = 1.0f - m_func.to_gamma((float)kr);
    };
    virtual ~ColorspaceOpCL2YUV() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_kr, m_kg, m_kb, m_scale;
    float m_nb, m_pb, m_nr, m_pr;
    const TransferFunc m_func;
};

class ColorspaceOpHDR2SDR : public ColorspaceOp {
public:
    ColorspaceOpHDR2SDR() : ColorspaceOpHDR2SDR(HDR2SDR_DISABLED) {};
    ColorspaceOpHDR2SDR(HDR2SDRToneMap mode) : m_mode(mode),
        m_source_peak(FILTER_DEFAULT_COLORSPACE_HDR_SOURCE_PEAK),
        m_ldr_nits(FILTER_DEFAULT_COLORSPACE_LDRNITS),
        m_desat_base(FILTER_DEFAULT_HDR2SDR_DESAT_BASE),
        m_desat_strength(FILTER_DEFAULT_HDR2SDR_DESAT_STRENGTH),
        m_desat_exp(FILTER_DEFAULT_HDR2SDR_DESAT_EXP) {};
    ColorspaceOpHDR2SDR(HDR2SDRToneMap mode, double source_peak, double ldr_nits,
        double desat_base, double desat_strength, double desat_exp) :
        m_mode(mode),
        m_source_peak(source_peak), m_ldr_nits(ldr_nits),
        m_desat_base(desat_base), m_desat_strength(desat_strength), m_desat_exp(desat_exp) {
        m_type = COLORSPACE_OP_TYPE_HDR2SDR;
    };
    virtual ~ColorspaceOpHDR2SDR() {};
    virtual std::string printDesat(double desat_scale);
    virtual std::string printDesatInfo();
    virtual bool add(const ColorspaceOp *op) override { UNREFERENCED_PARAMETER(op); return false; }
    double source_peak() const { return m_source_peak; }
    double ldr_nits() const { return m_ldr_nits; }
    double desat_base() const { return m_desat_base; }
    double desat_strength() const { return m_desat_strength; }
    double desat_exp() const { return m_desat_exp; }
protected:
    HDR2SDRToneMap m_mode;
    double m_source_peak, m_ldr_nits;
    double m_desat_base, m_desat_strength, m_desat_exp;
};

class ColorspaceOpHDR2SDRHable : public ColorspaceOpHDR2SDR {
public:
    ColorspaceOpHDR2SDRHable() : ColorspaceOpHDR2SDR(HDR2SDR_HABLE),
        m_A(FILTER_DEFAULT_HDR2SDR_HABLE_A),
        m_B(FILTER_DEFAULT_HDR2SDR_HABLE_B),
        m_C(FILTER_DEFAULT_HDR2SDR_HABLE_C),
        m_D(FILTER_DEFAULT_HDR2SDR_HABLE_D),
        m_E(FILTER_DEFAULT_HDR2SDR_HABLE_E),
        m_F(FILTER_DEFAULT_HDR2SDR_HABLE_F) {};
    ColorspaceOpHDR2SDRHable(double source_peak, double ldr_nits,
        double desat_base, double desat_strength, double desat_exp,
        double A, double B, double C, double D, double E, double F) :
        ColorspaceOpHDR2SDR(HDR2SDR_HABLE, source_peak, ldr_nits, desat_base, desat_strength, desat_exp),
        m_A(A), m_B(B), m_C(C), m_D(D), m_E(E), m_F(F) {
        m_type = COLORSPACE_OP_TYPE_HDR2SDR;
    };
    virtual ~ColorspaceOpHDR2SDRHable() {};
    virtual std::string print() override;
    virtual std::string printInfo() override;
    virtual bool add(const ColorspaceOp *op) override { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_A, m_B, m_C, m_D, m_E, m_F;
};

class ColorspaceOpHDR2SDRMobius : public ColorspaceOpHDR2SDR {
public:
    ColorspaceOpHDR2SDRMobius() : ColorspaceOpHDR2SDR(HDR2SDR_MOBIUS),
        m_transition(FILTER_DEFAULT_HDR2SDR_MOBIUS_TRANSITION),
        m_peak(FILTER_DEFAULT_HDR2SDR_MOBIUS_PEAK) {};
    ColorspaceOpHDR2SDRMobius(double source_peak, double ldr_nits,
        double desat_base, double desat_strength, double desat_exp,
        double transition, double peak) :
        ColorspaceOpHDR2SDR(HDR2SDR_MOBIUS, source_peak, ldr_nits, desat_base, desat_strength, desat_exp),
        m_transition(transition), m_peak(peak) {
        m_type = COLORSPACE_OP_TYPE_HDR2SDR;
    };
    virtual ~ColorspaceOpHDR2SDRMobius() {};
    virtual std::string print() override;
    virtual std::string printInfo() override;
    virtual bool add(const ColorspaceOp *op) override { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_transition, m_peak;
};

class ColorspaceOpHDR2SDRReinhard : public ColorspaceOpHDR2SDR {
public:
    ColorspaceOpHDR2SDRReinhard() : ColorspaceOpHDR2SDR(HDR2SDR_REINHARD),
        m_contrast(FILTER_DEFAULT_HDR2SDR_REINHARD_CONTRAST),
        m_peak(FILTER_DEFAULT_HDR2SDR_REINHARD_PEAK) {};
    ColorspaceOpHDR2SDRReinhard(double source_peak, double ldr_nits,
        double desat_base, double desat_strength, double desat_exp,
        double contrast, double peak) :
        ColorspaceOpHDR2SDR(HDR2SDR_REINHARD, source_peak, ldr_nits, desat_base, desat_strength, desat_exp),
        m_contrast(contrast), m_peak(peak) {
        m_type = COLORSPACE_OP_TYPE_HDR2SDR;
    };
    virtual ~ColorspaceOpHDR2SDRReinhard() {};
    virtual std::string print() override;
    virtual std::string printInfo() override;
    virtual bool add(const ColorspaceOp *op) override { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_contrast, m_peak;
};

class ColorspaceOpHDR2SDRBT2390 : public ColorspaceOpHDR2SDR {
public:
    ColorspaceOpHDR2SDRBT2390() : ColorspaceOpHDR2SDR(HDR2SDR_BT2390) {};
    ColorspaceOpHDR2SDRBT2390(double source_peak, double ldr_nits,
        double desat_base, double desat_strength, double desat_exp) :
        ColorspaceOpHDR2SDR(HDR2SDR_BT2390, source_peak, ldr_nits, desat_base, desat_strength, desat_exp) {
        m_type = COLORSPACE_OP_TYPE_HDR2SDR;
    };
    virtual ~ColorspaceOpHDR2SDRBT2390() {};
    virtual std::string print() override;
    virtual std::string printInfo() override;
    virtual bool add(const ColorspaceOp *op) override { UNREFERENCED_PARAMETER(op); return false; }
protected:
};

class ColorspaceOpRange : public ColorspaceOp {
public:
    ColorspaceOpRange() : m_scale_y(0), m_offset_y(0), m_scale_uv(0), m_offset_uv(0), m_int2float(false) {};
    ColorspaceOpRange(const VideoVUIInfo& info, int bit_depth, bool int2float) :
        m_scale_y(0), m_offset_y(0), m_scale_uv(0), m_offset_uv(0), m_int2float(int2float) {
        double range_y, range_uv, offset_y, offset_uv;
        if (info.matrix == RGY_MATRIX_RGB) {
            range_y   = (1<<bit_depth)-1;
            range_uv  = (1<<bit_depth)-1;
            offset_y  = 0.0;
            offset_uv = 0.0;
        } else {
            range_y   = (info.colorrange == RGY_COLORRANGE_FULL) ? ((1<<bit_depth)-1) : 219 << (bit_depth - 8);
            range_uv  = (info.colorrange == RGY_COLORRANGE_FULL) ? ((1<<bit_depth)-1) : 224 << (bit_depth - 8);
            offset_y  = (info.colorrange == RGY_COLORRANGE_FULL) ? 0 : 16 << (bit_depth - 8);
            offset_uv = (double)(1 << (bit_depth - 1));
        }
        if (int2float) {
            m_scale_y = 1.0 / range_y;
            m_offset_y = -offset_y * (1.0 / range_y);
            m_scale_uv = 1.0 / range_uv;
            m_offset_uv = -offset_uv * (1.0 / range_uv);
            m_type = COLORSPACE_OP_TYPE_I2F;
        } else {
            m_scale_y = range_y;
            m_offset_y = offset_y;
            m_scale_uv = range_uv;
            m_offset_uv = offset_uv;
            m_type = COLORSPACE_OP_TYPE_F2I;
        }
    };
    virtual ~ColorspaceOpRange() {};
    virtual std::string print();
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
protected:
    double m_scale_y, m_offset_y;
    double m_scale_uv, m_offset_uv;
    bool m_int2float;
};

enum class LUT3DIDX : int {
    r,g,b
};

struct ColorspaceOpLUT3DPreLUT {
    int size;
    vec3f min;
    vec3f max;
    vec3f scale;
};

class ColorspaceOpLUT3D : public ColorspaceOp {
public:
    ColorspaceOpLUT3D() : m_table_file(), m_interp(LUT3DInterp::Nearest), m_rgbscale(), m_tableSize0(0), m_tableSize01(0), m_preLUT() { m_type = COLORSPACE_OP_TYPE_LUT3D; };
    ColorspaceOpLUT3D(const tstring& table_file, LUT3DInterp interp, std::shared_ptr<RGYLog> log) : m_table_file(table_file), m_interp(interp), m_rgbscale(), m_tableSize0(0), m_tableSize01(0), m_preLUT(), m_log(log) {
        m_type = COLORSPACE_OP_TYPE_LUT3D;
    };
    virtual ~ColorspaceOpLUT3D() {};
    virtual std::string print() override;
    virtual std::string printInfo() override;
    virtual bool add(const ColorspaceOp *op) { UNREFERENCED_PARAMETER(op); return false; }
    virtual RGY_ERR init(std::vector<uint8_t>& devParams);
protected:
    void setAdditionalParams(std::vector<uint8_t>& additionalParams, const std::vector<LUTVEC>& luttable);
    RGY_ERR parseTable(std::vector<uint8_t>& additionalParams);
    RGY_ERR parseCube(std::vector<uint8_t>& additionalParams);
    void clearTable();
    tstring m_table_file;
    LUT3DInterp m_interp;
    vec3f m_rgbscale;
    int m_tableSize0;
    int m_tableSize01;
    ColorspaceOpLUT3DPreLUT m_preLUT;
    std::shared_ptr<RGYLog> m_log;
};

RGY_ERR ColorspaceOpLUT3D::init(std::vector<uint8_t>& additionalParams) {
    RGY_ERR err = parseTable(additionalParams);
    if (err != RGY_ERR_NONE) return err;

    return RGY_ERR_NONE;
}

std::string ColorspaceOpLUT3D::print() {
    const vec3f scale_to_table = m_rgbscale * (float)(m_tableSize0 - 1);
    auto str = strsprintf(R"(
    { // lut3d
    )");
    if (m_preLUT.size > 0) {
        str += strsprintf(R"(
        //prelut
        const int size = %d;
        const float prelutmin[3]   = { %.16ef, %.16ef, %.16ef };
        const float prelutscale[3] = { %.16ef, %.16ef, %.16ef };
        x = lut3d_prelut3(x, size, prelutmin, prelutscale, getDevParamsPrelutC(params));
        )",
        m_preLUT.size,
        m_preLUT.min(0), m_preLUT.min(1), m_preLUT.min(2),
        m_preLUT.scale(0), m_preLUT.scale(1), m_preLUT.scale(2));
    }
    str += strsprintf(R"(
        //lut3d main
        const int lutSize0  = %d;
        const int lutSize01 = %d;
        const float lut_max_idx = (float)(lutSize0 - 1) + 1e-6f; 
        x.x = clamp(x.x * %.16ef, 0.0f, lut_max_idx);
        x.y = clamp(x.y * %.16ef, 0.0f, lut_max_idx);
        x.z = clamp(x.z * %.16ef, 0.0f, lut_max_idx);
        x = lut3d_interp_%s(x, getDevParamsLutC(params), lutSize0, lutSize01);
    })",
        m_tableSize0, m_tableSize01,
        scale_to_table((int)LUT3DIDX::r), scale_to_table((int)LUT3DIDX::b), scale_to_table((int)LUT3DIDX::b),
        tchar_to_string(get_cx_desc(list_vpp_colorspace_lut3d_interp, (int)m_interp)).c_str());
    return str;
}

std::string ColorspaceOpLUT3D::printInfo() {
    return strsprintf("lut3d: table=%s\n"
        "                                  size=%d, interp=%s",
        tchar_to_string(m_table_file).c_str(),
        m_tableSize0, tchar_to_string(get_cx_desc(list_vpp_colorspace_lut3d_interp, (int)m_interp)).c_str());
}

void ColorspaceOpLUT3D::clearTable() {
    m_preLUT.size = 0;
    m_preLUT.max = vec3f(0.0f, 0.0f, 0.0f);
    m_preLUT.min = vec3f(0.0f, 0.0f, 0.0f);
    m_preLUT.scale = vec3f(0.0f, 0.0f, 0.0f);
    m_tableSize0 = 0;
    m_tableSize01 = 0;
    m_rgbscale = vec3f(0.0f, 0.0f, 0.0f);
}

RGY_ERR ColorspaceOpLUT3D::parseTable(std::vector<uint8_t>& additionalParams) {
    if (check_ext(m_table_file.c_str(), { ".cube" })) {
        return parseCube(additionalParams);
    }
    m_log->write(RGY_LOG_ERROR, RGY_LOGT_VPP, _T("Unsupported lut3d file type: %s\n"), m_table_file.c_str());
    return RGY_ERR_UNSUPPORTED;
}

RGY_ERR ColorspaceOpLUT3D::parseCube(std::vector<uint8_t>& additionalParams) {
    clearTable();

    FILE *fptmp = nullptr;
    if (_tfopen_s(&fptmp, m_table_file.c_str(), _T("r")) != 0) {
        m_log->write(RGY_LOG_ERROR, RGY_LOGT_VPP, _T("Failed to open lut3d cube file: %s\n"), m_table_file.c_str());
        return RGY_ERR_FILE_OPEN;
    }
    m_log->write(RGY_LOG_DEBUG, RGY_LOGT_VPP, _T("Opened lut3d cube file: %s\n"), m_table_file.c_str());
    std::unique_ptr<FILE, fp_deleter> fp(fptmp, fp_deleter());
    char buffer[512];
    float lutmin[3] = { 0.0f, 0.0f, 0.0f };
    float lutmax[3] = { 1.0f, 1.0f, 1.0f };
    LUTVEC value;
    memset(&value, 0, sizeof(value));

    std::vector<LUTVEC> luttable;
    size_t tableidx = 0;
    while (fgets(buffer, _countof(buffer), fp.get()) != nullptr) {
        if (buffer[0] == '#') continue;
        if (buffer[0] == '\n') continue;
        if (m_tableSize01 > 0
            && sscanf_s(buffer, "%f %f %f", &value.x, &value.y, &value.z) == 3) {
            const auto b = tableidx / m_tableSize01;
            const auto g = (tableidx - m_tableSize01 * b) / m_tableSize0;
            const auto r = tableidx - m_tableSize01 * b - m_tableSize0 * g;
            if (b >= (size_t)m_tableSize0 || g >= (size_t)m_tableSize0 || r >= (size_t)m_tableSize0) {
                return RGY_ERR_INVALID_DATA_TYPE;
            }
            luttable[r * m_tableSize01 + g * m_tableSize0 + b] = value;
            tableidx++;
        } else if (sscanf_s(buffer, "LUT_3D_SIZE %d", &m_tableSize0) == 1) {
            m_tableSize01 = m_tableSize0 * m_tableSize0;
            if (m_tableSize01 <= 0) {
                m_log->write(RGY_LOG_ERROR, RGY_LOGT_VPP, _T("Invalid lut3d cube file: size %d\n"), m_tableSize01);
                return RGY_ERR_INVALID_DATA_TYPE;
            }
            m_log->write(RGY_LOG_DEBUG, RGY_LOGT_VPP, _T("lut3d cube file: size %d\n"), m_tableSize01);
            luttable.resize(m_tableSize0 * m_tableSize0 * m_tableSize0);
        } else if (sscanf_s(buffer, "DOMAIN_MIN %f %f %f", &lutmin[0], &lutmin[1], &lutmin[2]) == 3
                || sscanf_s(buffer, "DOMAIN_MAX %f %f %f", &lutmax[0], &lutmax[1], &lutmax[2]) == 3) {
            // なにもしない
        } 
    }
    if (tableidx != luttable.size()) {
        m_log->write(RGY_LOG_ERROR, RGY_LOGT_VPP, _T("Invalid lut3d cube file: found %llu entry, which should be %llu\n"), (uint64_t)tableidx, (uint64_t)luttable.size());
        return RGY_ERR_INVALID_DATA_TYPE;
    }
    for (int i = 0; i < 3; i++) {
        m_rgbscale(i) = clamp(1.0f / (lutmax[i] - lutmin[i]), 0.0f, 1.0f);
        m_log->write(RGY_LOG_DEBUG, RGY_LOGT_VPP, _T("lut3d cube file: rgbscale(%d): %f\n"), i, m_rgbscale(i));
    }
    setAdditionalParams(additionalParams, luttable);
    return RGY_ERR_NONE;
}

void ColorspaceOpLUT3D::setAdditionalParams(std::vector<uint8_t>& additionalParams, const std::vector<LUTVEC>& luttable) {
    RGYColorspaceDevParams *addPrmPtr = (RGYColorspaceDevParams *)additionalParams.data();
    // LUTVECのアライメントをとるため、最初の位置を調整する
    addPrmPtr->lut_offset = (int)rgy_ceil_int(additionalParams.size(), sizeof(LUTVEC));
    m_log->write(RGY_LOG_DEBUG, RGY_LOGT_VPP, _T("lut3d table: lut_offset: %d, luttable size %llu\n"), addPrmPtr->lut_offset, sizeof(luttable[0]) * luttable.size());
    additionalParams.resize(addPrmPtr->lut_offset + sizeof(luttable[0]) * luttable.size());
    addPrmPtr = nullptr; // resizeで失効
    memcpy(getDevParamsLut(additionalParams.data()), luttable.data(), sizeof(luttable[0]) * luttable.size());
}

bool ColorspaceOpMatrix::add(const ColorspaceOp *op) {
    if (op->getType() != m_type) return false;
    const auto opMatrix = dynamic_cast<const ColorspaceOpMatrix *>(op);
    m = opMatrix->matrix() * m; //左からかける
    return true;
}

std::string ColorspaceOpMatrix::print() {
    return strsprintf(R"(
    {
        float m[3][3] = {
            { %.16ef, %.16ef, %.16ef },
            { %.16ef, %.16ef, %.16ef },
            { %.16ef, %.16ef, %.16ef }
        };
        x = matrix_mul(m, x);
    })",
    m(0, 0), m(0, 1), m(0, 2),
    m(1, 0), m(1, 1), m(1, 2),
    m(2, 0), m(2, 1), m(2, 2));
}

std::string ColorspaceOpGammaFunc::print() {
    const auto pre_scaler = func.to_gamma_scale;
    const auto post_scaler = 1.0;
    return strsprintf(R"(
    { //linear->gamma
        const float pre_scaler  = %.16ef;
        const float post_scaler = %.16ef;
        x.x = post_scaler * %s( x.x * pre_scaler );
        x.y = post_scaler * %s( x.y * pre_scaler );
        x.z = post_scaler * %s( x.z * pre_scaler );
    })",
        pre_scaler, post_scaler,
        func.to_gamma_str.c_str(), func.to_gamma_str.c_str(), func.to_gamma_str.c_str());
}

std::string ColorspaceOpInvGammaFunc::print() {
    const auto pre_scaler = 1.0;
    const auto post_scaler = func.to_linear_scale;
    return strsprintf(R"(
    { //gamma->linear
        const float pre_scaler  = %.16ef;
        const float post_scaler = %.16ef;
        x.x = post_scaler * %s( x.x * pre_scaler );
        x.y = post_scaler * %s( x.y * pre_scaler );
        x.z = post_scaler * %s( x.z * pre_scaler );
    })",
        pre_scaler, post_scaler,
        func.to_linear_str.c_str(), func.to_linear_str.c_str(), func.to_linear_str.c_str());
}

std::string ColorspaceOpAribB67::print() {
    return strsprintf("    x = aribB67Ops( x, %.16ef, %.16ef, %.16ef, %.16ef );\n", m_kr, m_kg, m_kb, m_scale);
}

std::string ColorspaceOpInvAribB67::print() {
    return strsprintf("    x = aribB67InvOps( x, %.16ef, %.16ef, %.16ef, %.16ef );\n", m_kr, m_kg, m_kb, m_scale);
}

std::string ColorspaceOpCL2RGB::print() {
    return strsprintf(R"(
    { //CL2RGB
        const float nb = %.16ef;
        const float pb = %.16ef;
        const float nr = %.16ef;
        const float pr = %.16ef;
        float y = x.x;
        float u = x.y;
        float v = x.z;

        const float b_minus_y = u * 2.0f * ((u < 0) ? nb : pb);
        const float r_minus_y = v * 2.0f * ((v < 0) ? nr : pr);

        float b = %s(b_minus_y + y);
        float r = %s(r_minus_y + y);

        y = %s(y);

        const float kr = %.16ef;
        const float kb = %.16ef;
        const float kg = %.16ef;
        const float g = (y - kr * r - kb * b) / kg;

        const float scale = %.16ef;
        x.x = r * scale;
        x.y = g * scale;
        x.z = b * scale;
    })",
        m_nb, m_pb, m_nr, m_pr,
        m_func.to_linear_str.c_str(), m_func.to_linear_str.c_str(), m_func.to_linear_str.c_str(),
        m_kr, m_kb, m_kg,
        m_scale);
}

std::string ColorspaceOpCL2YUV::print() {
    return strsprintf(R"(
    { //CL2YUV
        const float scale = %.16ef;
        float r = x.x * scale;
        float g = x.y * scale;
        float b = x.z * scale;

        const float kr = %.16ef;
        const float kb = %.16ef;
        const float kg = %.16ef;
        const float y = %s(kr * r + kg * g + kb * b);
        b = %s(b);
        r = %s(r);

        const float nb = %.16ef;
        const float pb = %.16ef;
        const float nr = %.16ef;
        const float pr = %.16ef;
        const float u = (b - y) / (2.0f * ((b - y < 0.0f) ? nb : pb));
        const float v = (r - y) / (2.0f * ((r - y < 0.0f) ? nr : pr));

        x.x = y;
        x.y = u;
        x.z = v;
    })",
        m_scale,
        m_kr, m_kb, m_kg,
        m_func.to_gamma_str.c_str(), m_func.to_gamma_str.c_str(), m_func.to_gamma_str.c_str(),
        m_nb, m_pb, m_nr, m_pr);
}

// https://mpv.io/manual/master/#options-tone-mapping-desaturate あたりがベース
// https://github.com/mpv-player/mpv/blob/master/video/out/gpu/video_shaders.c あたりを参考にして実装しなおしたもの
std::string ColorspaceOpHDR2SDR::printDesat(double desat_scale) {
    return strsprintf(R"(
        const float in_max  = fmaxf( fmaxf(x.x, x.y), fmaxf(x.z, 1e-6f) );
        const float out_max = fmaxf( fmaxf(y.x, y.y), fmaxf(y.z, 1e-6f) );
        const float mul = out_max / in_max;

        const float desat_scale = %.16ef;
        const float desat_base = %.16ef;
        const float desat_strength = %.16ef;
        const float desat_exp = %.16ef;
        // in coeff calculation, "out_max" should be in normalized scale
        const float coeff = fmaxf(out_max * desat_scale - desat_base, 1e-6f) / fmaxf(out_max * desat_scale, 1.0f);
        const float mixcoeff = desat_strength * powf(coeff, desat_exp);
        x.x = colorspace_mix(x.x * mul, y.x, mixcoeff);
        x.y = colorspace_mix(x.y * mul, y.y, mixcoeff);
        x.z = colorspace_mix(x.z * mul, y.z, mixcoeff);
    )",
        desat_scale, m_desat_base, m_desat_strength, m_desat_exp);
}

std::string ColorspaceOpHDR2SDRHable::print() {
    auto str = strsprintf(R"(
    { //hdr2sdr hable
        const float source_peak = %.16ef;
        const float ldr_nits = %.16ef;
        const float A = %.16ef;
        const float B = %.16ef;
        const float C = %.16ef;
        const float D = %.16ef;
        const float E = %.16ef;
        const float F = %.16ef;

        float3 y;
        y.x = hdr2sdr_hable( x.x, source_peak, ldr_nits, A, B, C, D, E, F );
        y.y = hdr2sdr_hable( x.y, source_peak, ldr_nits, A, B, C, D, E, F );
        y.z = hdr2sdr_hable( x.z, source_peak, ldr_nits, A, B, C, D, E, F );
    )", m_source_peak, m_ldr_nits, m_A, m_B, m_C, m_D, m_E, m_F);
    str += printDesat(1.0f);
    str += "}";
    return str;
}

std::string ColorspaceOpHDR2SDRMobius::print() {
    auto str = strsprintf(R"(
    { //hdr2sdr mobius
        const float source_peak = %.16ef;
        const float ldr_nits = %.16ef;
        const float transition = %.16ef;
        const float peak = %.16ef;
        const float in = fmaxf( fmaxf(x.x, x.y), fmaxf(x.z, 1e-6f) );

        float3 y;
        y.x = hdr2sdr_mobius( x.x, source_peak, ldr_nits, transition, peak );
        y.y = hdr2sdr_mobius( x.y, source_peak, ldr_nits, transition, peak );
        y.z = hdr2sdr_mobius( x.z, source_peak, ldr_nits, transition, peak );
    )", m_source_peak, m_ldr_nits, m_transition, m_peak);
    str += printDesat(1.0f);
    str += "}";
    return str;
}

std::string ColorspaceOpHDR2SDRReinhard::print() {
    auto str = strsprintf(R"(
    { //hdr2sdr reinhard
        const float source_peak = %.16ef;
        const float ldr_nits = %.16ef;
        const float contrast = %.16ef;
        const float peak = %.16ef;
        const float offset = (1.0f - contrast) / contrast;
        float3 y;
        y.x = hdr2sdr_reinhard( x.x, source_peak, ldr_nits, offset, peak );
        y.y = hdr2sdr_reinhard( x.y, source_peak, ldr_nits, offset, peak );
        y.z = hdr2sdr_reinhard( x.z, source_peak, ldr_nits, offset, peak );
    )", m_source_peak, m_ldr_nits, m_contrast, m_peak);
    str += printDesat(1.0f);
    str += "}";
    return str;
}

// https://mpv.io/manual/master/#options-tone-mapping ベースの実装
// https://github.com/mpv-player/mpv/blob/master/video/out/gpu/video_shaders.c あたりを参考にして実装しなおしたもの
std::string ColorspaceOpHDR2SDRBT2390::print() {
    auto str = strsprintf(R"(
    { //hdr2sdr bt.2390
        float sig_peak = %.16ef;
        const float dst_peak = %.16ef;
        const float inv_dst_peak = 1.0f / dst_peak;

        // use non-normalized value
        x.x *= dst_peak;
        x.y *= dst_peak;
        x.z *= dst_peak;

        const float sig_peak_pq = linear_to_pq_space(sig_peak);
        const float scale = 1.0 / sig_peak_pq;

        float3 y;
        y.x = linear_to_pq_space(x.x) * scale;
        y.y = linear_to_pq_space(x.y) * scale;
        y.z = linear_to_pq_space(x.z) * scale;
        const float maxLum = linear_to_pq_space(dst_peak) * scale;

        y.x = apply_bt2390(y.x, maxLum) * sig_peak_pq;
        y.y = apply_bt2390(y.y, maxLum) * sig_peak_pq;
        y.z = apply_bt2390(y.z, maxLum) * sig_peak_pq;

        y.x = pq_space_to_linear(y.x);
        y.y = pq_space_to_linear(y.y);
        y.z = pq_space_to_linear(y.z);
    )", m_source_peak, m_ldr_nits);
    str += printDesat(1.0f / m_ldr_nits);
    str += R"(
        // back to normalized value
        x.x *= inv_dst_peak;
        x.y *= inv_dst_peak;
        x.z *= inv_dst_peak;
    })";
    return str;
}

std::string ColorspaceOpHDR2SDR::printDesatInfo() {
    return strsprintf(""
        "                             desat base %.2f, strength %.2f, exp %.2f",
        m_desat_base, m_desat_strength, m_desat_exp);
}

std::string ColorspaceOpHDR2SDRHable::printInfo() {
    return strsprintf("hdr2sdr(hable): source_peak=%.2f ldr_nits=%.2f\n"
        "                             A %.2f, B %.2f, C %.2f, D %.2f\n"
        "                             E %.2f, F %.2f\n",
        m_source_peak, m_ldr_nits,
        m_A, m_B, m_C, m_D, m_E, m_F)
        + printDesatInfo();
}

std::string ColorspaceOpHDR2SDRMobius::printInfo() {
    return strsprintf("hdr2sdr(mobius): source_peak=%.2f ldr_nits=%.2f\n"
        "                             transition %.2f, peak %.2f\n",
        m_source_peak, m_ldr_nits,
        m_transition, m_peak)
        + printDesatInfo();
}

std::string ColorspaceOpHDR2SDRReinhard::printInfo() {
    return strsprintf("hdr2sdr(reinhard): source_peak=%.2f ldr_nits=%.2f\n"
        "                             contrast %.2f, peak %.2f\n",
        m_source_peak, m_ldr_nits,
        m_contrast, m_peak)
        + printDesatInfo();
}

std::string ColorspaceOpHDR2SDRBT2390::printInfo() {
    return strsprintf("hdr2sdr(bt2390): source_peak=%.2f ldr_nits=%.2f\n",
        m_source_peak, m_ldr_nits)
        + printDesatInfo();
}

std::string ColorspaceOpRange::print() {
    return strsprintf(R"(
    { //range %s
        const float range_y   = %.16ef;
        const float offset_y  = %.16ef;
        const float range_uv  = %.16ef;
        const float offset_uv = %.16ef;
        x.x = x.x * range_y  + offset_y;
        x.y = x.y * range_uv + offset_uv;
        x.z = x.z * range_uv + offset_uv;
    })",
        m_int2float ? "int->float" : "float->int",
        m_scale_y,  m_offset_y,
        m_scale_uv, m_offset_uv);
}

void ColorspaceOpCtrl::addOperation(ColorspaceOpInfo& op) {
    if (operations.size() == 0
        || !operations.back().ops->add(op.ops.get())) {
        operations.push_back(std::move(op));
    } else {
        operations.back().to = op.to;
    }
}

std::string ColorspaceOpCtrl::printOpAll() const {
    std::string str;
    for (const auto &op : operations) {
        str += op.ops->print() + "\n";
    }
    return str;
}

tstring ColorspaceOpCtrl::printInfoAll() const {
    tstring str;
    for (const auto &op : operations) {
        const bool print_maxtrix = op.from.matrix != op.to.matrix;
        const bool print_prim = op.from.colorprim != op.to.colorprim;
        const bool print_transfer = op.from.transfer != op.to.transfer;
        const bool print_range = op.from.colorrange != op.to.colorrange;
        tstring op_str;
        if (print_maxtrix || print_prim || print_transfer || print_range) {
            if (print_maxtrix)  op_str += tstring(_T(",matrix:"))   + get_cx_desc(list_colormatrix, op.from.matrix)    + _T("->") + get_cx_desc(list_colormatrix, op.to.matrix);
            if (print_prim)     op_str += tstring(_T(",prim:"))     + get_cx_desc(list_colorprim, op.from.colorprim)   + _T("->") + get_cx_desc(list_colorprim, op.to.colorprim);
            if (print_transfer) op_str += tstring(_T(",transfer:")) + get_cx_desc(list_transfer, op.from.transfer)     + _T("->") + get_cx_desc(list_transfer, op.to.transfer);
            if (print_range)    op_str += tstring(_T(",range:"))    + get_cx_desc(list_colorrange, op.from.colorrange) + _T("->") + get_cx_desc(list_colorrange, op.to.colorrange);

            if (str.length() > 0) {
                str += _T("\n                           ");
            }
            str += op_str.substr(1);
        } else if (op.ops->getType() == COLORSPACE_OP_TYPE_HDR2SDR
                || op.ops->getType() == COLORSPACE_OP_TYPE_LUT3D) {
            if (str.length() > 0) {
                str += _T("\n                           ");
            }
            str += char_to_tstring(op.ops->printInfo());
        } else if (op.ops->getType() == COLORSPACE_OP_TYPE_NONE) {
            if (str.length() > 0) {
                str += _T("\n                           ");
            }
        }
    }
    return str;
}

VideoVUIInfo ColorspaceOpCtrl::VuiOut() const {
    return operations.back().to;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpNclYUV2RGB(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to) {
    if (from.transfer != to.transfer) {
        AddMessage(RGY_LOG_ERROR, _T("transfer mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.colorprim != to.colorprim) {
        AddMessage(RGY_LOG_ERROR, _T("colorprim mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.matrix == RGY_MATRIX_RGB || to.matrix != RGY_MATRIX_RGB || from.matrix == RGY_MATRIX_BT2020_CL) {
        AddMessage(RGY_LOG_ERROR, _T("invalid conversion\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    mat3x3 mat;
    if (from.matrix == RGY_MATRIX_DERIVED_CL || from.matrix == RGY_MATRIX_DERIVED_NCL) {
        mat = matrixYUV2RGBfromPrim(from.colorprim);
    } else {
        mat = matrixYUV2RGB(from.matrix);
    }
    ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpMatrix>(mat)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpNclRGB2YUV(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to) {
    if (from.transfer != to.transfer) {
        AddMessage(RGY_LOG_ERROR, _T("transfer mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.colorprim != to.colorprim) {
        AddMessage(RGY_LOG_ERROR, _T("colorprim mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.matrix != RGY_MATRIX_RGB || to.matrix == RGY_MATRIX_RGB || from.matrix == RGY_MATRIX_BT2020_CL) {
        AddMessage(RGY_LOG_ERROR, _T("invalid conversion\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    mat3x3 mat;
    if (to.matrix == RGY_MATRIX_DERIVED_CL || to.matrix == RGY_MATRIX_DERIVED_NCL) {
        mat = matrixRGB2YUVfromPrim(to.colorprim);
    } else {
        mat = matrixRGB2YUV(to.matrix);
    }
    ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpMatrix>(mat)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpGamma2Linear(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak, bool approx_gamma, bool scene_ref) {
    if (from.colorprim != to.colorprim) {
        AddMessage(RGY_LOG_ERROR, _T("colorprim mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.matrix != RGY_MATRIX_RGB || to.matrix != RGY_MATRIX_RGB) {
        AddMessage(RGY_LOG_ERROR, _T("RGB to RGB only\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.transfer == RGY_TRANSFER_LINEAR || to.transfer != RGY_TRANSFER_LINEAR) {
        AddMessage(RGY_LOG_ERROR, _T("invalid conversion\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.transfer == RGY_TRANSFER_ARIB_B67 && useDisplayReferredB67(from, approx_gamma, scene_ref)) {
        const mat3x3 mat = matrixRGB2YUVfromPrim(from.colorprim);
        const auto func = getTrasferFunc(RGY_TRANSFER_ARIB_B67, source_peak, false);
        ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpInvAribB67>(mat(0, 0), mat(0, 1), mat(0, 2), func.to_linear_scale)));
    } else {
        ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpInvGammaFunc>(getTrasferFunc(from.transfer, source_peak, scene_ref))));
    }
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpLinear2Gamma(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak, bool approx_gamma, bool scene_ref) {
    if (from.colorprim != to.colorprim) {
        AddMessage(RGY_LOG_ERROR, _T("colorprim mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.matrix != RGY_MATRIX_RGB || to.matrix != RGY_MATRIX_RGB) {
        AddMessage(RGY_LOG_ERROR, _T("RGB to RGB only\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.transfer != RGY_TRANSFER_LINEAR || to.transfer == RGY_TRANSFER_LINEAR) {
        AddMessage(RGY_LOG_ERROR, _T("invalid conversion\n"));
        return RGY_ERR_INVALID_PARAM;
    }

    if (from.transfer == RGY_TRANSFER_ARIB_B67 && useDisplayReferredB67(to, approx_gamma, scene_ref)) {
        const mat3x3 mat = matrixRGB2YUVfromPrim(to.colorprim);
        const auto func = getTrasferFunc(RGY_TRANSFER_ARIB_B67, source_peak, false);
        ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpAribB67>(mat(0, 0), mat(0, 1), mat(0, 2), func.to_gamma_scale)));
    } else {
        ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpGammaFunc>(getTrasferFunc(to.transfer, source_peak, scene_ref))));
    }
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpGamut(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to) {
    if (from.matrix != RGY_MATRIX_RGB || from.transfer != RGY_TRANSFER_LINEAR
        || to.matrix != RGY_MATRIX_RGB || to.transfer != RGY_TRANSFER_LINEAR) {
        AddMessage(RGY_LOG_ERROR, _T("int/out must be linear RGB\n"));
        return RGY_ERR_INVALID_PARAM;
    }

    mat3x3 mat = matrixGamutXYZ2RGB(to.colorprim) * white_point_adaptation_matrix(from.colorprim, to.colorprim) * matrixGamutRGB2XYZ(from.colorprim);
    ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpMatrix>(mat)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpClYUV2RGB(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak) {
    if (from.colorprim != to.colorprim) {
        AddMessage(RGY_LOG_ERROR, _T("colorprim mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (to.matrix != RGY_MATRIX_RGB || to.transfer != RGY_TRANSFER_LINEAR) {
        AddMessage(RGY_LOG_ERROR, _T("output should be linear RGB\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.matrix != RGY_MATRIX_DERIVED_CL && (from.matrix != RGY_MATRIX_BT2020_CL || !transferEquivbt709(from.transfer))) {
        AddMessage(RGY_LOG_ERROR, _T("input should be 2020\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    auto func = getTrasferFunc(from.transfer, source_peak, true);
    if (func.to_linear) {
        auto mat = from.matrix == RGY_MATRIX_DERIVED_CL ? matrixYUV2RGBfromPrim(from.colorprim) : matrixYUV2RGB(from.matrix);
        ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpCL2RGB>(mat(0, 0), mat(0, 1), mat(0, 2), func.to_linear_scale, func)));
    }
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpClRGB2YUV(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const VideoVUIInfo &to, double source_peak) {
    if (from.colorprim != to.colorprim) {
        AddMessage(RGY_LOG_ERROR, _T("colorprim mismatch\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (from.matrix != RGY_MATRIX_RGB || from.transfer != RGY_TRANSFER_LINEAR) {
        AddMessage(RGY_LOG_ERROR, _T("input should be linear RGB\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (to.matrix != RGY_MATRIX_DERIVED_CL && (to.matrix != RGY_MATRIX_BT2020_CL || !transferEquivbt709(to.transfer))) {
        AddMessage(RGY_LOG_ERROR, _T("output should be 2020\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    auto func = getTrasferFunc(to.transfer, source_peak, true);
    if (func.to_gamma) {
        auto mat = to.matrix == RGY_MATRIX_DERIVED_CL ? matrixRGB2YUVfromPrim(to.colorprim) : matrixRGB2YUV(to.matrix);
        ops.push_back(ColorspaceOpInfo(from, to, make_unique<ColorspaceOpCL2YUV>(mat(0, 0), mat(0, 1), mat(0, 2), func.to_gamma_scale, func)));
    }
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpHDR2SDRHable(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm) {
    ops.push_back(ColorspaceOpInfo(from, from, make_unique<ColorspaceOpHDR2SDRHable>(
        prm.hdr_source_peak, prm.ldr_nits, prm.desat_base, prm.desat_strength, prm.desat_exp,
        prm.hable.a, prm.hable.b, prm.hable.c, prm.hable.d, prm.hable.e, prm.hable.f)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpHDR2SDRMobius(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm) {
    ops.push_back(ColorspaceOpInfo(from, from, make_unique<ColorspaceOpHDR2SDRMobius>(
        prm.hdr_source_peak, prm.ldr_nits, prm.desat_base, prm.desat_strength, prm.desat_exp,
        prm.mobius.transition, prm.mobius.peak)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpHDR2SDRReinhard(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm) {
    ops.push_back(ColorspaceOpInfo(from, from, make_unique<ColorspaceOpHDR2SDRReinhard>(
        prm.hdr_source_peak, prm.ldr_nits, prm.desat_base, prm.desat_strength, prm.desat_exp,
        prm.reinhard.contrast, prm.reinhard.peak)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpHDR2SDRBT2390(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const HDR2SDRParams &prm) {
    ops.push_back(ColorspaceOpInfo(from, from, make_unique<ColorspaceOpHDR2SDRBT2390>(
        prm.hdr_source_peak, prm.ldr_nits, prm.desat_base, prm.desat_strength, prm.desat_exp)));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::addColorspaceOpLUT3D(vector<ColorspaceOpInfo> &ops, const VideoVUIInfo &from, const LUT3DParams &prm, std::vector<uint8_t>& additionalParams) {
    auto op = make_unique<ColorspaceOpLUT3D>(prm.table_file, prm.interp, m_log);
    if (auto ret = op->init(additionalParams); ret != RGY_ERR_NONE) {
        return ret;
    }
    ops.push_back(ColorspaceOpInfo(from, from, std::move(op)));
    return RGY_ERR_NONE;
}

bool is_valid_2020cl(const VideoVUIInfo &csp) {
    return csp.matrix == RGY_MATRIX_BT2020_CL && transferEquivbt709(csp.transfer);
}

bool is_valid_ictcp(const VideoVUIInfo &csp) {
    return csp.matrix == RGY_MATRIX_ICTCP &&
        (csp.transfer == RGY_TRANSFER_ST2084 || csp.transfer == RGY_TRANSFER_ARIB_B67) &&
        csp.colorprim == RGY_PRIM_BT2020;
}

bool is_valid_lms(const VideoVUIInfo &csp) {
    return csp.matrix == RGY_MATRIX_2100_LMS &&
        (csp.transfer == RGY_TRANSFER_LINEAR || csp.transfer == RGY_TRANSFER_ST2084 || csp.transfer == RGY_TRANSFER_ARIB_B67) &&
        csp.colorprim == RGY_PRIM_BT2020;
}

bool is_valid_csp(const VideoVUIInfo &csp) {
    // 1. Require matrix to be set if transfer is set.
    // 2. Require transfer to be set if colorprim is set.
    // 3. Check requirements for Rec.2020 CL.
    // 4. Check requirements for chromaticity-derived NCL matrix.
    // 5. Check requirements for chromaticity-derived CL matrix.
    // 6. Check requirements for Rec.2100 ICtCp.
    // 7. Check requirements for Rec.2100 LMS.
    return !(csp.matrix == RGY_MATRIX_UNSPECIFIED && csp.transfer != RGY_TRANSFER_UNSPECIFIED) &&
        !(csp.transfer == RGY_TRANSFER_UNSPECIFIED && csp.colorprim != RGY_PRIM_UNSPECIFIED) &&
        !(csp.matrix == RGY_MATRIX_BT2020_CL && !is_valid_2020cl(csp)) &&
        !(csp.matrix == RGY_MATRIX_DERIVED_NCL && csp.colorprim == RGY_PRIM_UNSPECIFIED) &&
        !(csp.matrix == RGY_MATRIX_DERIVED_CL && csp.colorprim == RGY_PRIM_UNSPECIFIED) &&
        !(csp.matrix == RGY_MATRIX_ICTCP && !is_valid_ictcp(csp)) &&
        !(csp.matrix == RGY_MATRIX_2100_LMS && !is_valid_lms(csp));
}

RGY_ERR ColorspaceOpCtrl::getNeighboringColorspaces(vector<ColorspaceOpInfo>& ops, const VideoVUIInfo &csp, double source_peak, bool approx_gamma, bool scene_ref) {
#define CHECK(x) { RGY_ERR err = (x); if (err != RGY_ERR_NONE) return err; };
    if (csp.matrix == RGY_MATRIX_RGB) {
        const auto special_matrices = make_array<CspMatrix>(
            RGY_MATRIX_UNSPECIFIED,
            RGY_MATRIX_RGB,
            RGY_MATRIX_BT2020_CL,
            RGY_MATRIX_DERIVED_NCL,
            RGY_MATRIX_DERIVED_CL,
            RGY_MATRIX_2100_LMS,
            RGY_MATRIX_ICTCP
        );

        // RGB can be converted to conventional YUV.
        for (auto matrix : CspMatrixList) {
            if (std::find(special_matrices.begin(), special_matrices.end(), matrix) == special_matrices.end()) {
                CHECK(addColorspaceOpNclRGB2YUV(ops, csp, csp.to(matrix)));
            }
        }
        if (csp.colorprim != RGY_PRIM_UNSPECIFIED) {
            CHECK(addColorspaceOpNclRGB2YUV(ops, csp, csp.to(RGY_MATRIX_DERIVED_NCL)));
        }

        // Linear RGB can be converted to other transfer functions and colorprim; also to combined matrix-transfer systems.
        if (csp.transfer == RGY_TRANSFER_LINEAR) {
            for (auto transfer : CspTransferList) {
                if (transfer != csp.transfer && transfer != RGY_TRANSFER_UNSPECIFIED) {
                    CHECK(addColorspaceOpLinear2Gamma(ops, csp, csp.to(transfer), source_peak, approx_gamma, scene_ref));
                    if (csp.colorprim != RGY_PRIM_UNSPECIFIED) {
                        CHECK(addColorspaceOpClRGB2YUV(ops, csp, csp.to(transfer).to(RGY_MATRIX_DERIVED_CL), source_peak));
                    }

                }
            }
            for (auto colorprim : CspColorprimList) {
                if (colorprim != csp.colorprim && colorprim != RGY_PRIM_UNSPECIFIED) {
                    CHECK(addColorspaceOpGamut(ops, csp, csp.to(colorprim)));
                }
            }

            CHECK(addColorspaceOpClRGB2YUV(ops, csp, csp.to(RGY_MATRIX_BT2020_CL).to(RGY_TRANSFER_BT709), source_peak));

            if (csp.colorprim == RGY_PRIM_BT2020) {
                CHECK(addColorspaceOpNclRGB2YUV(ops, csp, csp.to(RGY_MATRIX_2100_LMS)));
            }
        } else if (csp.transfer != RGY_TRANSFER_UNSPECIFIED) {
            // Gamma RGB can be converted to linear RGB.
            CHECK(addColorspaceOpGamma2Linear(ops, csp, csp.to(RGY_TRANSFER_LINEAR), source_peak, approx_gamma, scene_ref));
        }
    } else if (csp.matrix == RGY_MATRIX_BT2020_CL || csp.matrix == RGY_MATRIX_DERIVED_CL) {
        CHECK(addColorspaceOpClYUV2RGB(ops, csp, csp.to(RGY_MATRIX_RGB).to(RGY_TRANSFER_LINEAR), source_peak));
    } else if (csp.matrix == RGY_MATRIX_2100_LMS) {
        // LMS with ST_2084 or ARIB_B67 transfer functions can be converted to ICtCp and also to linear transfer function
        if (csp.transfer == RGY_TRANSFER_ST2084 || csp.transfer == RGY_TRANSFER_ARIB_B67) {
            //add_edge(ops, csp, csp.to(RGY_MATRIX_ICTCP), create_lms_to_ictcp_operation);
            CHECK(addColorspaceOpGamma2Linear(ops, csp, csp.to(RGY_TRANSFER_LINEAR), source_peak, approx_gamma, scene_ref));
        }
        // LMS with linear transfer function can be converted to RGB matrix and to ARIB_B67 and ST_2084 transfer functions
        if (csp.transfer == RGY_TRANSFER_LINEAR) {
            CHECK(addColorspaceOpNclYUV2RGB(ops, csp, csp.to(RGY_MATRIX_RGB)));
            //CHECK(addColorspaceOpLinear2Gamma(ops, csp, csp.to(RGY_TRANSFER_ST2084), source_peak, approx_gamma, scene_ref));
            //CHECK(addColorspaceOpLinear2Gamma(ops, csp, csp.to(RGY_TRANSFER_ARIB_B67), source_peak, approx_gamma, scene_ref));
        }
    } else if (csp.matrix == RGY_MATRIX_ICTCP) {
        // ICtCp with ST_2084 or ARIB_B67 transfer functions can be converted to LMS
        if (csp.transfer == RGY_TRANSFER_ST2084 || csp.transfer == RGY_TRANSFER_ARIB_B67) {
            return RGY_ERR_UNSUPPORTED;
            //add_edge(ops, csp, csp.to(RGY_MATRIX_2100_LMS), create_ictcp_to_lms_operation);
        }
    } else if (csp.matrix != RGY_MATRIX_UNSPECIFIED) {
        // YUV can be converted to RGB.
        CHECK(addColorspaceOpNclYUV2RGB(ops, csp, csp.to(RGY_MATRIX_RGB)));
    }
    return RGY_ERR_NONE;
}

struct ColorspaceHash {
    bool operator()(const VideoVUIInfo &csp) const {
        return std::hash<uint32_t>{}(
            ((uint32_t)(csp.matrix) << 16) |
            ((uint32_t)(csp.transfer) << 8) |
            ((uint32_t)(csp.colorprim)));
    }
};

RGY_ERR ColorspaceOpCtrl::setLUT3D(const VideoVUIInfo &in, const VideoVUIInfo &out, double sdr_source_peak, bool approx_gamma, bool scene_ref, const LUT3DParams& prm, std::vector<uint8_t>& additionalParams, int height) {
    auto csp_from1 = in;
    if (csp_from1.matrix == RGY_MATRIX_UNSPECIFIED) {
        csp_from1 = csp_from1.to(RGY_MATRIX_BT709);
    }
    if (csp_from1.transfer == RGY_TRANSFER_UNSPECIFIED) {
        csp_from1 = csp_from1.to(RGY_TRANSFER_BT709);
    }
    const auto csp_to1 = csp_from1.to(RGY_MATRIX_RGB);
    CHECK(setPath(csp_from1, csp_to1, sdr_source_peak, approx_gamma, scene_ref, height));

    CHECK(addColorspaceOpLUT3D(m_path, csp_to1, prm, additionalParams));

    auto csp_to2 = out;
    csp_to2.apply_auto(csp_from1, height);
    if (csp_to2.matrix == RGY_MATRIX_UNSPECIFIED) {
        csp_to2 = csp_to2.to(csp_from1.matrix);
    }
    if (csp_to2.transfer == RGY_TRANSFER_UNSPECIFIED) {
        csp_to2 = csp_to2.to(csp_from1.transfer);
    }
    if (csp_to2.colorprim == RGY_PRIM_UNSPECIFIED) {
        csp_to2 = csp_to2.to(csp_from1.colorprim);
    }
    CHECK(setPath(csp_to1, csp_to2, sdr_source_peak, approx_gamma, scene_ref, height));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::setHDR2SDR(const VideoVUIInfo &in, const VideoVUIInfo &out, double sdr_source_peak, bool approx_gamma, bool scene_ref, const HDR2SDRParams& prm, int height) {
    auto csp_from1 = in;
    if (csp_from1.matrix == RGY_MATRIX_UNSPECIFIED) {
        csp_from1 = csp_from1.to(RGY_MATRIX_BT2020_NCL);
    }
    if (csp_from1.transfer == RGY_TRANSFER_UNSPECIFIED) {
        csp_from1 = csp_from1.to(RGY_TRANSFER_ST2084);
    }
    if (csp_from1.transfer == RGY_TRANSFER_ARIB_B67 && csp_from1.colorprim == RGY_PRIM_UNSPECIFIED) {
        csp_from1 = csp_from1.to(RGY_PRIM_BT2020);
    }
    const auto csp_to1 = csp_from1.to(RGY_MATRIX_RGB).to(RGY_TRANSFER_LINEAR);
    CHECK(setPath(csp_from1, csp_to1, sdr_source_peak, approx_gamma, scene_ref, height));
    switch (prm.tonemap) {
    case HDR2SDR_HABLE:
        CHECK(addColorspaceOpHDR2SDRHable(m_path, csp_to1, prm));
        break;
    case HDR2SDR_MOBIUS:
        CHECK(addColorspaceOpHDR2SDRMobius(m_path, csp_to1, prm));
        break;
    case HDR2SDR_REINHARD:
        CHECK(addColorspaceOpHDR2SDRReinhard(m_path, csp_to1, prm));
        break;
    case HDR2SDR_BT2390:
        CHECK(addColorspaceOpHDR2SDRBT2390(m_path, csp_to1, prm));
        break;
    default:
        m_log->write(RGY_LOG_ERROR, RGY_LOGT_VPP, _T("Invalid tonemap type for hdr2sdr.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    auto csp_to2 = out;
    csp_to2.apply_auto(csp_from1, height);
    if (csp_to2.matrix == RGY_MATRIX_UNSPECIFIED) {
        csp_to2 = csp_to2.to(RGY_MATRIX_BT709).to(RGY_TRANSFER_BT709).to(RGY_PRIM_BT709);
    }
    auto csp_from2 = csp_to1;
    if (csp_from2.matrix == RGY_MATRIX_UNSPECIFIED) {
        csp_from2 = csp_from2.to(RGY_MATRIX_RGB);
    }
    if (csp_from2.transfer == RGY_TRANSFER_UNSPECIFIED) {
        csp_from2 = csp_from2.to(RGY_TRANSFER_LINEAR);
    }
    if (csp_from2.colorprim == RGY_PRIM_UNSPECIFIED) {
        csp_from2 = csp_from2.to(RGY_PRIM_BT2020);
    }
    CHECK(setPath(csp_from2, csp_to2, sdr_source_peak, approx_gamma, scene_ref, height));
    return RGY_ERR_NONE;
}

RGY_ERR ColorspaceOpCtrl::setPath(const VideoVUIInfo &in, const VideoVUIInfo &out, double source_peak, bool approx_gamma, bool scene_ref, int height) {
    if (!is_valid_csp(in)) {
        AddMessage(RGY_LOG_ERROR, _T("invalid input colorspace definition\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (!is_valid_csp(out)) {
        AddMessage(RGY_LOG_ERROR, _T("invalid output colorspace definition\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (   in.matrix == out.matrix
        && in.colorprim == out.colorprim
        && in.transfer == out.transfer) {
        //やることはない
        m_path.push_back(ColorspaceOpInfo(in, out, std::make_unique<ColorspaceOpNone>()));
        return RGY_ERR_NONE;
    }
    auto out_target = out;
    out_target.apply_auto(in, height);
    AddMessage(RGY_LOG_DEBUG, _T("Search path from %s -> %s"), in.print_main().c_str(), out_target.print_main().c_str());

    std::deque<VideoVUIInfo> queue;
    std::unordered_set<VideoVUIInfo, ColorspaceHash> visited;
    std::unordered_map<VideoVUIInfo, ColorspaceOpInfo, ColorspaceHash> parents;

    VideoVUIInfo vertex;

    visited.insert(in);
    queue.push_back(in);

    while (!queue.empty()) {
        vertex = queue.front();
        queue.pop_front();

        if (vertex == out_target)
            break;

        vector<ColorspaceOpInfo> nodes;
        CHECK(getNeighboringColorspaces(nodes, vertex, source_peak, approx_gamma, scene_ref));
        for (auto &&edge : nodes) {
            if (visited.find(edge.to) != visited.end())
                continue;

            visited.insert(edge.to);
            queue.push_back(edge.to);
            parents[edge.to] = std::move(edge);
        }
    }
    if (vertex != out_target) {
        AddMessage(RGY_LOG_ERROR, _T("no path between colorspaces\n"));
        return RGY_ERR_INVALID_PARAM;
    }

    std::vector<ColorspaceOpInfo> path;
    while (vertex != in) {
        auto it = parents.find(vertex);

        auto node = std::move(it->second);
        vertex = node.from;
        path.push_back(std::move(node));
    }
    for (auto it = path.rbegin(); it != path.rend(); it++) {
        m_path.push_back(std::move(*it));
    }
    return RGY_ERR_NONE;
}
#undef CHECK

RGY_ERR ColorspaceOpCtrl::setOperation(RGY_CSP csp_in, RGY_CSP csp_out) {
    if (m_path.size() > 0) {
        const auto from = m_path.begin()->from;
        const auto to = m_path.back().to;

        //先頭にfloatでの規格化式を追加する
        auto begin = ColorspaceOpInfo(from, from, make_unique<ColorspaceOpRange>(from, RGY_CSP_BIT_DEPTH[csp_in], true));
        addOperation(begin);

        AddMessage(RGY_LOG_DEBUG, _T("Set path...\n"));
        AddMessage(RGY_LOG_DEBUG, _T("  node: %s\n"), m_path.begin()->from.print_main().c_str());
        for (auto& node : m_path) {
            AddMessage(RGY_LOG_DEBUG, _T("  node: %s\n"), node.to.print_main().c_str());
            addOperation(node);
        }

        //最後にfloat->intの式を追加する
        auto end = ColorspaceOpInfo(to, to, make_unique<ColorspaceOpRange>(to, RGY_CSP_BIT_DEPTH[csp_out], false));
        addOperation(end);
    } else if (false) { //for debug
        VideoVUIInfo dummy = VideoVUIInfo().to(RGY_MATRIX_BT709);
        //先頭にfloatでの規格化式を追加する
        auto begin = ColorspaceOpInfo(dummy, dummy, make_unique<ColorspaceOpRange>(dummy, RGY_CSP_BIT_DEPTH[csp_in], true));
        addOperation(begin);
        //最後にfloat->intの式を追加する
        auto end = ColorspaceOpInfo(dummy, dummy, make_unique<ColorspaceOpRange>(dummy, RGY_CSP_BIT_DEPTH[csp_out], false));
        addOperation(end);
    }
    return RGY_ERR_NONE;
}

const char *kernel_base1 = R"(
float3 convert_colorspace_custom(float3 x, const __global RGYColorspaceDevParams *__restrict__ params) {
)";


const char *kernel_base2 = R"(
    return x;
}

#define PIX_PER_THREAD (4)

#ifndef clamp
#define clamp(x, low, high) (((x) <= (high)) ? (((x) >= (low)) ? (x) : (low)) : (high))
#endif

#define toPix(x) (TYPE)clamp((x) + 0.5f, 0.0f, (1<<(sizeof(TYPE)*8)) - 0.5f)

__kernel void kernel_colorspace(
    __global uchar *pDstY, __global uchar *pDstU, __global uchar *pDstV,
    const int dstPitch, const int dstWidth, const int dstHeight,
    const __global uchar *__restrict__ pSrcY, const __global uchar *pSrcU, const __global uchar *pSrcV,
    const int srcPitch, const int srcWidth, const int srcHeight,
    const __global RGYColorspaceDevParams *__restrict__ params) {
    const int ix = get_global_id(0) * PIX_PER_THREAD;
    const int iy = get_global_id(1);
    if (ix + PIX_PER_THREAD - 1 < dstWidth && iy < dstHeight) {

        TYPE4 srcY = *(__global TYPE4 *)(pSrcY + iy * srcPitch + ix * sizeof(TYPE));
        TYPE4 srcU = *(__global TYPE4 *)(pSrcU + iy * srcPitch + ix * sizeof(TYPE));
        TYPE4 srcV = *(__global TYPE4 *)(pSrcV + iy * srcPitch + ix * sizeof(TYPE));

        float3 pix0 = make_float3((float)srcY.x, (float)srcU.x, (float)srcV.x);
        float3 pix1 = make_float3((float)srcY.y, (float)srcU.y, (float)srcV.y);
        float3 pix2 = make_float3((float)srcY.z, (float)srcU.z, (float)srcV.z);
        float3 pix3 = make_float3((float)srcY.w, (float)srcU.w, (float)srcV.w);

        pix0 = convert_colorspace_custom(pix0, params);
        pix1 = convert_colorspace_custom(pix1, params);
        pix2 = convert_colorspace_custom(pix2, params);
        pix3 = convert_colorspace_custom(pix3, params);

        TYPE4 dstY, dstU, dstV;
        dstY.x = toPix(pix0.x); dstU.x = toPix(pix0.y); dstV.x = toPix(pix0.z);
        dstY.y = toPix(pix1.x); dstU.y = toPix(pix1.y); dstV.y = toPix(pix1.z);
        dstY.z = toPix(pix2.x); dstU.z = toPix(pix2.y); dstV.z = toPix(pix2.z);
        dstY.w = toPix(pix3.x); dstU.w = toPix(pix3.y); dstV.w = toPix(pix3.z);

        __global TYPE4 *ptrDstY = (__global TYPE4 *)(pDstY + iy * dstPitch + ix * sizeof(TYPE));
        __global TYPE4 *ptrDstU = (__global TYPE4 *)(pDstU + iy * dstPitch + ix * sizeof(TYPE));
        __global TYPE4 *ptrDstV = (__global TYPE4 *)(pDstV + iy * dstPitch + ix * sizeof(TYPE));

        ptrDstY[0] = dstY;
        ptrDstU[0] = dstU;
        ptrDstV[0] = dstV;
    }
};
)";


RGY_ERR RGYFilterColorspace::procFrame(RGYFrameInfo *pOutputFrame, const RGYFrameInfo *pInputFrame, RGYOpenCLQueue &queue, const std::vector<RGYOpenCLEvent> &wait_events, RGYOpenCLEvent *event) {
    auto prm = std::dynamic_pointer_cast<RGYFilterParamColorspace>(m_param);
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    const auto planeInputY = getPlane(pInputFrame, RGY_PLANE_Y);
    const auto planeInputU = getPlane(pInputFrame, RGY_PLANE_U);
    const auto planeInputV = getPlane(pInputFrame, RGY_PLANE_V);
    auto planeOutputY = getPlane(pOutputFrame, RGY_PLANE_Y);
    auto planeOutputU = getPlane(pOutputFrame, RGY_PLANE_U);
    auto planeOutputV = getPlane(pOutputFrame, RGY_PLANE_V);
    if (   planeInputY.pitch[0] != planeInputU.pitch[0]
        || planeInputY.pitch[0] != planeInputV.pitch[0]
        || planeOutputY.pitch[0] != planeOutputU.pitch[0]
        || planeOutputY.pitch[0] != planeOutputV.pitch[0]) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid pitch!\n"));
        return RGY_ERR_UNKNOWN;
    }

    const char *kernel_name = "kernel_colorspace";
    RGYWorkSize local(COLORSPACE_BLOCK_X, COLORSPACE_BLOCK_Y);
    RGYWorkSize global(pOutputFrame->width, pOutputFrame->height);
    auto err = m_colorspace.get()->kernel(kernel_name).config(queue, local, global, wait_events, event).launch(
        (cl_mem)planeOutputY.ptr[0], (cl_mem)planeOutputU.ptr[0], (cl_mem)planeOutputV.ptr[0],
        planeOutputY.pitch[0], planeOutputY.width, planeOutputY.height,
        (cl_mem)planeInputY.ptr[0], (cl_mem)planeInputU.ptr[0], (cl_mem)planeInputV.ptr[0],
        planeInputY.pitch[0], planeInputY.width, planeInputY.height,
        (additionalParamsDev) ? additionalParamsDev->mem() : nullptr);
    if (err != RGY_ERR_NONE) {
        AddMessage(RGY_LOG_ERROR, _T("error at %s (procFrame(%s)): %s.\n"),
            char_to_tstring(kernel_name).c_str(), RGY_CSP_NAMES[planeOutputY.csp], get_err_mes(err));
        return err;
    }
    return RGY_ERR_NONE;
}

std::string RGYFilterColorspace::genKernelCode() {
    const auto colorspace_func_h_cl = getEmbeddedResourceStr(_T("RGY_FILTER_COLORSPACE_CL"), _T("EXE_DATA"), m_cl->getModuleHandle());

    std::string kernel;
    kernel += colorspace_func_h_cl;
    kernel += kernel_base1;
    kernel += opCtrl->printOpAll();
    kernel += kernel_base2;
    return kernel;
}

RGYFilterColorspace::RGYFilterColorspace(shared_ptr<RGYOpenCLContext> context) : RGYFilter(context), crop(), opCtrl(), m_colorspace(), additionalParams(), additionalParamsDev() {
    m_name = _T("colorspace");
}

RGYFilterColorspace::~RGYFilterColorspace() {
    close();
}
RGY_ERR RGYFilterColorspace::check_param(shared_ptr<RGYFilterParamColorspace> prm) {
    if (prm->frameOut.height <= 0 || prm->frameOut.width <= 0) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->colorspace.hdr2sdr.ldr_nits <= 0.0) {
        AddMessage(RGY_LOG_ERROR, _T("ldr_nits must be positive value.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->colorspace.hdr2sdr.hdr_source_peak <= 0.0) {
        AddMessage(RGY_LOG_ERROR, _T("source_peak must be positive value.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    return RGY_ERR_NONE;
}

RGY_ERR RGYFilterColorspace::init(shared_ptr<RGYFilterParam> pParam, shared_ptr<RGYLog> pPrintMes) {
    RGY_ERR sts = RGY_ERR_NONE;
    m_pLog = pPrintMes;
    auto prm = std::dynamic_pointer_cast<RGYFilterParamColorspace>(pParam);
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }

    //パラメータチェック
    if (check_param(prm) != RGY_ERR_NONE) {
        return RGY_ERR_INVALID_PARAM;
    }

    prm->frameOut = pParam->frameIn;
    if (!crop || cmpFrameInfoCspResolution(&crop->GetFilterParam()->frameIn, &pParam->frameIn)) {
        crop.reset();
        if (pParam->frameIn.csp != RGY_CSP_YUV444_16
            && pParam->frameIn.csp != RGY_CSP_YUV444
            && RGY_CSP_CHROMA_FORMAT[pParam->frameIn.csp] != RGY_CHROMAFMT_RGB) {
            unique_ptr<RGYFilterCspCrop> filterCrop(new RGYFilterCspCrop(m_cl));
            shared_ptr<RGYFilterParamCrop> paramCrop(new RGYFilterParamCrop());
            paramCrop->frameIn = pParam->frameIn;
            paramCrop->frameOut = pParam->frameIn;
            paramCrop->frameOut.csp = (std::max(RGY_CSP_BIT_DEPTH[paramCrop->frameIn.csp], RGY_CSP_BIT_DEPTH[prm->encCsp]) > 8) ? RGY_CSP_YUV444_16 : RGY_CSP_YUV444;
            paramCrop->baseFps = pParam->baseFps;
            paramCrop->frameOut.mem_type = RGY_MEM_TYPE_GPU;
            paramCrop->bOutOverwrite = false;
            sts = filterCrop->init(paramCrop, m_pLog);
            if (sts != RGY_ERR_NONE) {
                return sts;
            }
            crop = std::move(filterCrop);
            prm->frameOut = paramCrop->frameOut;
        }
    }

    //入力ファイルのVUIが取得されていれば、これを使用する
    auto &firstVUI = prm->colorspace.convs.begin()->from;
    firstVUI.apply_auto(prm->VuiIn, prm->frameIn.height);

    auto prmPrev = std::dynamic_pointer_cast<RGYFilterParamColorspace>(m_param);
    if (!prmPrev || prmPrev->colorspace != prm->colorspace) {
        additionalParams.resize(sizeof(RGYColorspaceDevParams));
        RGYColorspaceDevParams* addPrmPtr = (RGYColorspaceDevParams*)additionalParams.data();
        addPrmPtr->lut_offset = 0;
        addPrmPtr->prelut_offset = 0;

        const auto filterInCsp = prm->frameOut.csp;
        if (RGY_CSP_CHROMA_FORMAT[filterInCsp] == RGY_CHROMAFMT_RGB) {
            if (prm->colorspace.convs.begin()->from.matrix != RGY_MATRIX_RGB) {
                AddMessage(RGY_LOG_ERROR, _T("source matrix must be \"GBR\" when input is in RGB format.\n"));
                return RGY_ERR_INVALID_PARAM;
            }
            if (prm->colorspace.convs.back().to.matrix == RGY_MATRIX_RGB) {
                AddMessage(RGY_LOG_ERROR, _T("output matrix to \"GBR\" is not supported.\n"));
                return RGY_ERR_INVALID_PARAM;
            }
            prm->frameOut.csp = RGY_CSP_YUV444;
        }
        opCtrl = std::make_unique<ColorspaceOpCtrl>(pPrintMes);
        if (prm->colorspace.lut3d.table_file.length() > 0) {
            if (prm->colorspace.hdr2sdr.tonemap != HDR2SDR_DISABLED) {
                AddMessage(RGY_LOG_ERROR, _T("lut3d and hdr2sdr cannot be used at the same time.\n"));
                return RGY_ERR_UNSUPPORTED;
            }
            const auto &convbegin = prm->colorspace.convs.begin();
            const auto from = convbegin->from;
            const auto source_peak = convbegin->sdr_source_peak;
            const auto approx_gamma = convbegin->approx_gamma;
            const auto scene_ref = convbegin->scene_ref;
            const auto& to = prm->colorspace.convs.back().to;
            if ((sts = opCtrl->setLUT3D(from, to, source_peak, approx_gamma, scene_ref, prm->colorspace.lut3d, additionalParams, prm->frameIn.height)) != RGY_ERR_NONE) {
                return sts;
            }
        } else if (prm->colorspace.hdr2sdr.tonemap != HDR2SDR_DISABLED) {
            if (prm->colorspace.lut3d.table_file.length() > 0) {
                AddMessage(RGY_LOG_ERROR, _T("lut3d and hdr2sdr cannot be used at the same time.\n"));
                return RGY_ERR_UNSUPPORTED;
            }
            const auto &convbegin = prm->colorspace.convs.begin();
            const auto from = convbegin->from;
            const auto source_peak = convbegin->sdr_source_peak;
            const auto approx_gamma = convbegin->approx_gamma;
            const auto scene_ref = convbegin->scene_ref;
            const auto to = prm->colorspace.convs.back().to;
            if ((sts = opCtrl->setHDR2SDR(from, to, source_peak, approx_gamma, scene_ref, prm->colorspace.hdr2sdr, prm->frameIn.height)) != RGY_ERR_NONE) {
                return sts;
            }
        } else {
            for (const auto &conv : prm->colorspace.convs) {
                if ((sts = opCtrl->setPath(conv.from, conv.to, conv.sdr_source_peak, conv.approx_gamma, conv.scene_ref, prm->frameIn.height)) != RGY_ERR_NONE) {
                    return sts;
                }
            }
        }
        opCtrl->setOperation(filterInCsp, filterInCsp);
        if (additionalParams.size() > 0) {
            AddMessage(RGY_LOG_DEBUG, _T("additional param size: %llu.\n"), (uint64_t)additionalParams.size());
            additionalParamsDev = m_cl->copyDataToBuffer(additionalParams.data(), additionalParams.size(), CL_MEM_READ_ONLY, m_cl->queue().get());
            if (!additionalParamsDev) {
                AddMessage(RGY_LOG_ERROR, _T("Failed to allocate memory for additional params.\n"));
                return RGY_ERR_NULL_PTR;
            }
        }
        const auto options = strsprintf("-D TYPE=%s -D TYPE4=%s -D bit_depth=%d -D IS_OPENCL=1",
            RGY_CSP_BIT_DEPTH[prm->frameOut.csp] > 8 ? "ushort" : "uchar",
            RGY_CSP_BIT_DEPTH[prm->frameOut.csp] > 8 ? "ushort4" : "uchar4",
            RGY_CSP_BIT_DEPTH[prm->frameOut.csp]);
        const auto kernel = genKernelCode();
        if (m_pLog->getLogLevel(RGY_LOGT_VPP_BUILD) <= RGY_LOG_DEBUG) {
            const auto sep = _T("--------------------------------------------------------------------------\n");
            const auto mes = tstring(sep) + _T("Generated colorspace kernel code...\n") + sep + char_to_tstring(kernel) + sep;
            AddMessage(RGY_LOG_DEBUG, mes);
        }
        m_colorspace.set(m_cl->buildAsync(kernel, options.c_str()));
    }

    auto err = AllocFrameBuf(prm->frameOut, 1);
    if (err != RGY_ERR_NONE) {
        AddMessage(RGY_LOG_ERROR, _T("failed to allocate memory: %s.\n"), get_err_mes(err));
        return RGY_ERR_MEMORY_ALLOC;
    }
    for (int i = 0; i < RGY_CSP_PLANES[m_frameBuf[0]->frame.csp]; i++) {
        prm->frameOut.pitch[i] = m_frameBuf[0]->frame.pitch[i];
    }
    AddMessage(RGY_LOG_DEBUG, _T("allocated output buffer: %dx%d, picth %d, %s.\n"),
        pParam->frameOut.width, pParam->frameOut.height, pParam->frameOut.pitch, RGY_CSP_NAMES[pParam->frameOut.csp]);

    tstring filterInfo = _T("colorspace: ");
    if (crop) {
        filterInfo += crop->GetInputMessage() + _T("\n                           ");
    }
    filterInfo += opCtrl->printInfoAll();
    setFilterInfo(filterInfo);
    m_param = prm;
    return sts;
}

VideoVUIInfo RGYFilterColorspace::VuiOut() const {
    return opCtrl->VuiOut();
}

tstring RGYFilterParamColorspace::print() const {
    return _T("");
}

RGY_ERR RGYFilterColorspace::run_filter(const RGYFrameInfo *pInputFrame, RGYFrameInfo **ppOutputFrames, int *pOutputFrameNum, RGYOpenCLQueue &queue, const std::vector<RGYOpenCLEvent> &wait_events, RGYOpenCLEvent *event) {
    RGY_ERR sts = RGY_ERR_NONE;
    if (pInputFrame->ptr[0] == nullptr) {
        *pOutputFrameNum = 0;
        return sts;
    }

    *pOutputFrameNum = 1;
    if (ppOutputFrames[0] == nullptr) {
        auto pOutFrame = m_frameBuf[0].get();
        ppOutputFrames[0] = &pOutFrame->frame;
    }
    ppOutputFrames[0]->picstruct = pInputFrame->picstruct;

    if (!m_colorspace.get()) {
        AddMessage(RGY_LOG_ERROR, _T("failed to load RGY_FILTER_COLORSPACE_CL(m_colorspace)\n"));
        return RGY_ERR_OPENCL_CRUSH;
    }
    const auto memcpyKind = getMemcpyKind(pInputFrame->mem_type, ppOutputFrames[0]->mem_type);
    if (memcpyKind != RGYCLMemcpyD2D) {
        AddMessage(RGY_LOG_ERROR, _T("only supported on device memory.\n"));
        return RGY_ERR_UNSUPPORTED;
    }
    //YUV444への変換
    if (crop) {
        int cropFilterOutputNum = 0;
        RGYFrameInfo *pCropFilterOutput[1] = { nullptr };
        RGYFrameInfo cropInput = *pInputFrame;
        auto sts_filter = crop->filter(&cropInput, (RGYFrameInfo **)&pCropFilterOutput, &cropFilterOutputNum, queue, wait_events, nullptr);
        if (pCropFilterOutput[0] == nullptr || cropFilterOutputNum != 1) {
            AddMessage(RGY_LOG_ERROR, _T("Unknown behavior \"%s\".\n"), crop->name().c_str());
            return sts_filter;
        }
        if (sts_filter != RGY_ERR_NONE || cropFilterOutputNum != 1) {
            AddMessage(RGY_LOG_ERROR, _T("Error while running filter \"%s\".\n"), crop->name().c_str());
            return sts_filter;
        }
        pInputFrame = pCropFilterOutput[0];
    }

    sts = procFrame(ppOutputFrames[0], pInputFrame, queue, wait_events, event);
    if (sts != RGY_ERR_NONE) {
        AddMessage(RGY_LOG_ERROR, _T("error at procFrame (%s): %s.\n"),
            RGY_CSP_NAMES[pInputFrame->csp], get_err_mes(sts));
        return sts;
    }

    return sts;
}

void RGYFilterColorspace::close() {
    m_frameBuf.clear();
    additionalParamsDev.reset();
    opCtrl.reset();
    crop.reset();
    m_colorspace.clear();
    m_cl.reset();
    m_bInterlacedWarn = false;
    AddMessage(RGY_LOG_DEBUG, _T("closed colorspace filter.\n"));
}
