// Tests del núcleo DSP (sin JUCE). Devuelve 0 si todo pasa.
#include "../src/StereoExpanderDSP.h"

#include <cstdio>
#include <cstdlib>
#include <random>

static int failures = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            ++failures;                                   \
            std::printf ("FALLO %s:%d  ", __FILE__, __LINE__); \
            std::printf (__VA_ARGS__);                    \
            std::printf ("\n");                           \
        }                                                 \
    } while (0)

// Ganancia en régimen estacionario del pasa-banda para una senoide de frecuencia f.
static double bandpassGain (double f, double fs)
{
    const auto sections = se::designButterworthBandpass (se::StereoExpanderDSP::filterOrder,
                                                         se::StereoExpanderDSP::bandLow,
                                                         se::StereoExpanderDSP::bandHigh, fs);
    std::vector<se::Biquad4> filt (sections.size());
    for (size_t i = 0; i < sections.size(); ++i) filt[i].setCoeffs (sections[i]);

    const int total = (int) (fs * 1.0), measureFrom = total / 2;
    double peak = 0.0;
    for (int n = 0; n < total; ++n)
    {
        double y = std::sin (2.0 * se::kPi * f * n / fs);
        for (auto& s : filt) y = s.process (y);
        if (n >= measureFrom) peak = std::max (peak, std::abs (y));
    }
    return peak;
}

int main()
{
    std::mt19937 rng (1234);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
    se::StereoExpanderDSP dsp;
    dsp.prepare (44100.0);

    // --- Mapeo del parámetro -------------------------------------------------
    CHECK (se::StereoExpanderDSP::widthFromPercent (-100.0f) == 0.0f, "p=-100 -> w=0");
    CHECK (se::StereoExpanderDSP::widthFromPercent (0.0f) == 1.0f,    "p=0 -> w=1");
    CHECK (se::StereoExpanderDSP::widthFromPercent (100.0f) == 2.0f,  "p=+100 -> w=2");

    // --- Estéreo -------------------------------------------------------------
    for (int i = 0; i < 1000; ++i)
    {
        const float l0 = dist (rng), r0 = dist (rng);

        float l = l0, r = r0;                       // w = 1 -> transparente
        dsp.processStereoSample (l, r, 1.0f);
        CHECK (std::abs (l - l0) < 1e-6f && std::abs (r - r0) < 1e-6f, "w=1 no es transparente");

        l = l0; r = r0;                             // w = 0 -> mono
        dsp.processStereoSample (l, r, 0.0f);
        const float m = 0.5f * (l0 + r0);
        CHECK (std::abs (l - m) < 1e-6f && std::abs (r - m) < 1e-6f, "w=0 no es mono");

        l = l0; r = r0;                             // w = 2
        dsp.processStereoSample (l, r, 2.0f);
        CHECK (std::abs (l - (0.5f * (l0 + r0) + (l0 - r0))) < 1e-5f, "w=2 L incorrecto");
        CHECK (std::abs (r - (0.5f * (l0 + r0) - (l0 - r0))) < 1e-5f, "w=2 R incorrecto");
    }

    // --- Filtro pasa-banda Butterworth 1-3 kHz, orden 4 ----------------------
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        const double centre = std::sqrt (1000.0 * 3000.0);
        const double gC = bandpassGain (centre, fs);
        const double gLo = bandpassGain (1000.0, fs);
        const double gHi = bandpassGain (3000.0, fs);
        CHECK (std::abs (gC - 1.0) < 2e-3,                 "fs=%g centro: %f (esperado 1)", fs, gC);
        CHECK (std::abs (gLo - 0.70710678) < 5e-3,         "fs=%g 1 kHz: %f (esperado -3 dB)", fs, gLo);
        CHECK (std::abs (gHi - 0.70710678) < 5e-3,         "fs=%g 3 kHz: %f (esperado -3 dB)", fs, gHi);
        CHECK (bandpassGain (100.0, fs) < 0.01,            "fs=%g 100 Hz no atenúa", fs);
        CHECK (bandpassGain (10000.0, fs) < 0.03,          "fs=%g 10 kHz no atenúa", fs);
        std::printf ("fs=%6.0f  centro=%.4f  1k=%.4f  3k=%.4f  100Hz=%.5f  10k=%.5f\n", fs, gC, gLo, gHi,
                     bandpassGain (100.0, fs), bandpassGain (10000.0, fs));
    }

    // --- Mono -> estéreo -----------------------------------------------------
    {
        se::StereoExpanderDSP mono;
        mono.prepare (44100.0);
        const int delay = (int) (0.007 * 44100.0);
        CHECK (delay == 308, "retardo = %d muestras (esperado 308)", delay);

        std::vector<float> in (4000);
        for (auto& v : in) v = dist (rng);
        for (int n = 0; n < (int) in.size(); ++n)
        {
            float l, r;
            mono.processMonoSample (in[(size_t) n], 1.0f, l, r);
            if (n < delay) // antes de que llegue la señal retardada, L = R = x * 0.9988
            {
                CHECK (std::abs (l - in[(size_t) n] * 0.9988149f) < 1e-6f, "n=%d L", n);
                CHECK (std::abs (r - in[(size_t) n] * 0.9988149f) < 1e-6f, "n=%d R", n);
            }
            // L + R = 2 x g  siempre (el lado se cancela en la suma)
            CHECK (std::abs ((l + r) - 2.0f * in[(size_t) n] * 0.9988149f) < 1e-5f, "n=%d L+R", n);
        }
    }

    if (failures == 0)
        std::printf ("OK: todos los tests pasan\n");
    return failures == 0 ? 0 : 1;
}
