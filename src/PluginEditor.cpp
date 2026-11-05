#include "PluginEditor.h"

namespace
{
    // Coordenadas en el "espacio de diseño" 301 x 153 (igual que el original).
    const juce::Rectangle<float> trackArea  { 27.0f, 72.0f, 247.0f, 26.0f };
    const juce::Rectangle<float> displayRect { 120.0f, 115.0f, 63.0f, 19.0f };
    const juce::Rectangle<float> titleArea  { 0.0f, 10.0f, 301.0f, 30.0f };
    constexpr float thumbW = 30.0f, thumbH = 20.0f;

    juce::Font uiFont (float h, bool bold = false)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }
}

//==============================================================================
class StereoExpanderEditor::PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override
    {
        const auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);

        // Barra negra con bisel
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (r, 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRect (r, 1.0f);
        g.setColour (juce::Colour (0xff2c2c2c));
        g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 2.0f, r.getRight() - 2.0f); // línea central

        // Deslizador de bandas verticales
        const float cx = juce::jlimit (r.getX() + thumbW * 0.5f, r.getRight() - thumbW * 0.5f, sliderPos);
        const auto t = juce::Rectangle<float> (thumbW, thumbH).withCentre ({ cx, r.getCentreY() });
        g.setColour (juce::Colour (0xff0b0b0b));
        g.fillRect (t);
        for (float sx = t.getX() + 1.0f; sx < t.getRight() - 1.0f; sx += 3.0f)
        {
            g.setColour (juce::Colour (0xffd8d8d8));
            g.fillRect (sx, t.getY() + 1.0f, 1.0f, t.getHeight() - 2.0f);
            g.setColour (juce::Colour (0xff6f6f6f));
            g.fillRect (sx + 1.0f, t.getY() + 1.0f, 1.0f, t.getHeight() - 2.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawRect (t, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRect (t.expanded (1.0f), 1.0f);
    }
};

//==============================================================================
StereoExpanderEditor::StereoExpanderEditor (StereoExpanderProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      lnf (std::make_unique<PanelLookAndFeel>()),
      widthAttachment (p.apvts, StereoExpanderProcessor::widthParamID, widthSlider)
{
    widthSlider.setLookAndFeel (lnf.get());
    widthSlider.setDoubleClickReturnValue (true, 0.0);
    widthSlider.setPopupDisplayEnabled (false, false, nullptr);
    widthSlider.setMouseDragSensitivity (250);
    widthSlider.addListener (this);
    addAndMakeVisible (widthSlider);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) designW / designH);
    setResizeLimits (designW, designH, designW * 4, designH * 4);
    setSize (designW * 2, designH * 2);
}

StereoExpanderEditor::~StereoExpanderEditor()
{
    widthSlider.removeListener (this);
    widthSlider.setLookAndFeel (nullptr);
}

juce::AffineTransform StereoExpanderEditor::designToScreen() const
{
    return juce::AffineTransform::scale ((float) getWidth() / designW, (float) getHeight() / designH);
}

juce::Rectangle<int> StereoExpanderEditor::displayBounds() const
{
    return displayRect.transformedBy (designToScreen()).getSmallestIntegerContainer();
}

void StereoExpanderEditor::resized()
{
    widthSlider.setBounds (trackArea.transformedBy (designToScreen()).toNearestInt());
    rebuildBackground();
}

void StereoExpanderEditor::mouseUp (const juce::MouseEvent& e)
{
    // Clic en el título (o en cualquier parte del panel de créditos) -> alterna "About".
    if (showAbout || titleArea.transformedBy (designToScreen()).contains (e.position))
    {
        showAbout = ! showAbout;
        widthSlider.setVisible (! showAbout);
        repaint();
    }
}

//==============================================================================
void StereoExpanderEditor::rebuildBackground()
{
    const float scale = (float) getWidth() / designW;
    const auto pixelScale = juce::Component::getApproximateScaleFactorForComponent (this);
    const int pw = juce::jmax (1, juce::roundToInt (getWidth() * pixelScale));
    const int ph = juce::jmax (1, juce::roundToInt (getHeight() * pixelScale));
    juce::ignoreUnused (scale);

    background = juce::Image (juce::Image::RGB, pw, ph, true);
    juce::Graphics g (background);

    // Metal cepillado: degradado suave + finas líneas horizontales pseudoaleatorias (semilla fija).
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb9b9b9), 0.0f, 0.0f,
                                             juce::Colour (0xff8d8d8d), 0.0f, (float) ph, false));
    g.fillAll();

    juce::Random rng (0x5E75);
    for (int y = 0; y < ph; ++y)
    {
        const float a = rng.nextFloat();
        g.setColour ((a > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + 0.10f * std::abs (a - 0.5f)));
        const int x0 = rng.nextInt (juce::jmax (1, pw / 3));
        g.drawHorizontalLine (y, (float) x0, (float) (pw - rng.nextInt (juce::jmax (1, pw / 3))));
    }

    // Bisel exterior
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawHorizontalLine (0, 0.0f, (float) pw);
    g.drawVerticalLine (0, 0.0f, (float) ph);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawHorizontalLine (ph - 1, 0.0f, (float) pw);
    g.drawVerticalLine (pw - 1, 0.0f, (float) ph);
}

void StereoExpanderEditor::paint (juce::Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());

    if (showAbout)
    {
        paintAbout (g);
        return;
    }

    g.addTransform (designToScreen());

    // ---- Título con sombra ----
    {
        auto f = uiFont (25.0f, true);
        f.setExtraKerningFactor (0.12f);
        g.setFont (f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawText ("Stereo Expander", titleArea.translated (1.5f, 1.5f), juce::Justification::centred, false);
        g.setColour (juce::Colours::white);
        g.drawText ("Stereo Expander", titleArea, juce::Justification::centred, false);
    }

    // ---- Escala -100 / 0 / +100 ----
    {
        g.setFont (uiFont (10.0f, true));
        const float xs[3] = { trackArea.getX() + 6.0f, trackArea.getCentreX(), trackArea.getRight() - 6.0f };
        const char* labels[3] = { "- 100", "0", "+ 100" };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawText (labels[i], juce::Rectangle<float> (46.0f, 12.0f).withCentre ({ xs[i], 59.0f }),
                        juce::Justification::centred, false);
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.fillRect (xs[i] - 0.5f, 65.0f, 1.0f, 6.0f);
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.fillRect (xs[i] + 0.5f, 65.0f, 1.0f, 6.0f);
        }
    }

    // ---- Visor azul con el valor ----
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff04041c), displayRect.getX(), displayRect.getY(),
                                                 juce::Colour (0xff1830a8), displayRect.getRight(), displayRect.getBottom(),
                                                 false));
        g.fillRect (displayRect);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRect (displayRect, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRect (displayRect.expanded (1.0f), 1.0f);

        g.setFont (uiFont (12.0f, true));
        g.setColour (juce::Colours::white);
        g.drawText (juce::String::formatted ("%2.2f", widthSlider.getValue()), displayRect,
                    juce::Justification::centred, false);
    }

    // ---- Firma en la esquina inferior derecha ----
    {
        const juce::Rectangle<float> sig { 196.0f, 128.0f, 99.0f, 18.0f };
        g.setFont (uiFont (11.0f, true));
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawText ("Recomp by VooD", sig.translated (0.7f, 0.7f), juce::Justification::bottomRight, false);
        g.setColour (juce::Colour (0xff1c2a6e));
        g.drawText ("Recomp by VooD", sig, juce::Justification::bottomRight, false);
    }
}

void StereoExpanderEditor::paintAbout (juce::Graphics& g)
{
    g.addTransform (designToScreen());

    auto f = uiFont (17.0f, true);
    f.setExtraKerningFactor (0.08f);
    g.setFont (f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawText ("Stereo Expander", juce::Rectangle<float> (1.5f, 11.5f, 301.0f, 24.0f), juce::Justification::centred, false);
    g.setColour (juce::Colours::white);
    g.drawText ("Stereo Expander", juce::Rectangle<float> (0.0f, 10.0f, 301.0f, 24.0f), juce::Justification::centred, false);

    g.setFont (uiFont (10.0f));
    g.setColour (juce::Colour (0xff101010));
    g.drawFittedText (juce::String::fromUTF8 (
                          "Algoritmo original: Wavelab StereoExpander 1.1\n"
                          "\xC2\xA9 2001 Spectral Design GmbH (Steinberg).\n\n"
                          "Reimplementaci\xC3\xB3n en JUCE para VST3 / AU,\n"
                          "Windows x64 y macOS Apple Silicon.\n"
                          "Contrastada muestra a muestra con el original."),
                      juce::Rectangle<int> (14, 40, 273, 70), juce::Justification::centred, 6);

    g.setFont (uiFont (11.0f, true));
    g.setColour (juce::Colour (0xff1c2a6e));
    g.drawText ("Recomp by VooD", juce::Rectangle<float> (196.0f, 128.0f, 99.0f, 18.0f),
                juce::Justification::bottomRight, false);
}
