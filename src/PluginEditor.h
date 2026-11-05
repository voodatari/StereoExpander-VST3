#pragma once

#include "PluginProcessor.h"

// Interfaz inspirada en la del original (panel metálico 301x153, escala -100/0/+100, barra negra
// con deslizador y visor azul). Todo se dibuja con vectores: no se usan los mapas de bits originales.
class StereoExpanderEditor : public juce::AudioProcessorEditor,
                             private juce::Slider::Listener
{
public:
    explicit StereoExpanderEditor (StereoExpanderProcessor&);
    ~StereoExpanderEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

    static constexpr int designW = 301, designH = 153;

private:
    class PanelLookAndFeel;

    void sliderValueChanged (juce::Slider*) override { repaint (displayBounds()); }
    juce::Rectangle<int> displayBounds() const;
    juce::AffineTransform designToScreen() const;
    void rebuildBackground();
    void paintAbout (juce::Graphics&);

    StereoExpanderProcessor& processor;

    std::unique_ptr<PanelLookAndFeel> lnf;
    juce::Slider widthSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    juce::AudioProcessorValueTreeState::SliderAttachment widthAttachment;

    juce::Image background;
    bool showAbout = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoExpanderEditor)
};
