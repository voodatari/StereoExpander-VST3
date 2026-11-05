// Stereo Expander - núcleo DSP (sin dependencias de JUCE, para poder testearlo aparte).
//
// Comportamiento, deducido del análisis del plugin VST2 original (ver docs/ANALISIS.md):
//
//  * Entrada estéreo  -> ensanchador mid/side. Ancho w = (p + 100) / 100, con p en [-100, +100] %.
//                        p = 0 deja la señal intacta, p = -100 la convierte en mono, p = +100 duplica el lado.
//  * Entrada mono     -> pseudo-estéreo: retardo de 7 ms -> pasa-banda Butterworth 1-3 kHz (orden 4)
//                        -> L = (x + w*f) * 0.9988149, R = (x - w*f) * 0.9988149.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace se
{
constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// Sección IIR de 4.º orden (forma directa, coeficientes normalizados a a0 = 1).
struct Biquad4Coeffs
{
    std::array<double, 5> b {};
    std::array<double, 5> a {};
};

namespace detail
{
    using Poly = std::array<double, 5>; // polinomio en x = z^-1, grado <= 4

    inline Poly mul (const Poly& p, const std::array<double, 2>& q) // p * (q0 + q1 x)
    {
        Poly r {};
        for (size_t i = 0; i < 5; ++i)
        {
            r[i] += p[i] * q[0];
            if (i + 1 < 5)
                r[i + 1] += p[i] * q[1];
        }
        return r;
    }

    // (1 - x)^i * (1 + x)^(4 - i)
    inline Poly basis (int i)
    {
        Poly p { 1, 0, 0, 0, 0 };
        for (int k = 0; k < i; ++k)       p = mul (p, { 1.0, -1.0 });
        for (int k = 0; k < 4 - i; ++k)   p = mul (p, { 1.0,  1.0 });
        return p;
    }
} // namespace detail

// Pasa-banda Butterworth de orden `order` (par): `order / 2` secciones de 4.º orden en cascada.
// Prototipo paso-bajo con polos s^2 + a s + 1, a = -2 cos((N-1+2k) pi / 2N); transformación
// paso-bajo -> paso-banda y bilineal con "prewarping" tan(pi f / fs).
inline std::vector<Biquad4Coeffs> designButterworthBandpass (int order, double f1, double f2, double fs)
{
    order = (std::max) (2, order + (order & 1));
    f2 = (std::min) (f2, 0.495 * fs);

    const double w1 = std::tan (kPi * f1 / fs);
    const double w2 = std::tan (kPi * f2 / fs);
    const double bw = w2 - w1;
    const double w0sq = w1 * w2;

    std::vector<Biquad4Coeffs> sections;

    for (int k = 1; k <= order / 2; ++k)
    {
        const double a = -2.0 * std::cos ((order - 1 + 2 * k) * kPi / (2.0 * order));

        // Denominador en s:  s^4 + a*B s^3 + (2 w0^2 + B^2) s^2 + a*B*w0^2 s + w0^4
        const std::array<double, 5> c { w0sq * w0sq,           // s^0
                                        a * bw * w0sq,         // s^1
                                        2.0 * w0sq + bw * bw,  // s^2
                                        a * bw,                // s^3
                                        1.0 };                 // s^4

        detail::Poly den {};
        for (int i = 0; i < 5; ++i)
        {
            const auto p = detail::basis (i);
            for (size_t j = 0; j < 5; ++j)
                den[j] += c[(size_t) i] * p[j];
        }

        // Numerador: B^2 s^2  ->  B^2 (1 - x^2)^2
        const detail::Poly num { bw * bw, 0.0, -2.0 * bw * bw, 0.0, bw * bw };

        Biquad4Coeffs s;
        for (size_t j = 0; j < 5; ++j)
        {
            s.b[j] = num[j] / den[0];
            s.a[j] = den[j] / den[0];
        }
        sections.push_back (s);
    }
    return sections;
}

//==============================================================================
class Biquad4
{
public:
    void setCoeffs (const Biquad4Coeffs& c) { co = c; }
    void reset() { z.fill (0.0); }

    double process (double x) noexcept
    {
        const double y = co.b[0] * x + z[0];
        z[0] = co.b[1] * x - co.a[1] * y + z[1];
        z[1] = co.b[2] * x - co.a[2] * y + z[2];
        z[2] = co.b[3] * x - co.a[3] * y + z[3];
        z[3] = co.b[4] * x - co.a[4] * y;

        for (auto& s : z)
            if (std::abs (s) < 1e-15)
                s = 0.0; // evita denormales
        return y;
    }

private:
    Biquad4Coeffs co;
    std::array<double, 4> z {};
};

//==============================================================================
class StereoExpanderDSP
{
public:
    static constexpr double delaySeconds = 0.007;
    static constexpr double bandLow = 1000.0;
    static constexpr double bandHigh = 3000.0;
    static constexpr int filterOrder = 4;
    static constexpr float monoGain = 0.9988149f;
    static constexpr float invSqrt2 = 0.70710678118654752f;

    // Convierte el parámetro en % (-100..+100) en el factor de ancho (0..2).
    static float widthFromPercent (float percent) noexcept { return (percent + 100.0f) * 0.01f; }

    void prepare (double sampleRate)
    {
        delayLen = (int) (delaySeconds * sampleRate); // truncado, igual que el original
        size_t n = 1;
        while ((int) n < delayLen + 2) n <<= 1;
        delayBuf.assign (n, 0.0f);
        mask = n - 1;

        const auto sections = designButterworthBandpass (filterOrder, bandLow, bandHigh, sampleRate);
        filters.resize (sections.size());
        for (size_t i = 0; i < sections.size(); ++i)
            filters[i].setCoeffs (sections[i]);
        reset();
    }

    void reset()
    {
        std::fill (delayBuf.begin(), delayBuf.end(), 0.0f);
        writePos = 0;
        for (auto& f : filters) f.reset();
    }

    // Entrada estéreo: mid/side.
    void processStereoSample (float& l, float& r, float width) const noexcept
    {
        const float mid  = (l + r) * invSqrt2;
        const float side = (l - r) * invSqrt2 * width;
        l = (mid + side) * invSqrt2;
        r = (mid - side) * invSqrt2;
    }

    // Entrada mono -> salida estéreo.
    void processMonoSample (float x, float width, float& outL, float& outR) noexcept
    {
        delayBuf[writePos & mask] = x;
        double d = (double) delayBuf[(writePos - (size_t) delayLen) & mask];
        ++writePos;

        for (auto& f : filters)
            d = f.process (d);

        const float side = (float) d * width;
        outL = (x + side) * monoGain;
        outR = (x - side) * monoGain;
    }

private:
    std::vector<float> delayBuf;
    size_t mask = 0, writePos = 0;
    int delayLen = 0;
    std::vector<Biquad4> filters;
};

} // namespace se
