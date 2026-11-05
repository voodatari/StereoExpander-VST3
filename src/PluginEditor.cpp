#include "PluginEditor.h"

StereoExpanderEditor::StereoExpanderEditor (StereoExpanderProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      widthAttachment (p.apvts, StereoExpanderProcessor::widthParamID, widthSlider)
{
    setSize (260, 280);

    title.setText ("Stereo Expander", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    addAndMakeVisible (title);

    widthSlider.setTextValueSuffix (" %");
    widthSlider.setNumDecimalPlacesToDisplay (2);
    widthSlider.setDoubleClickReturnValue (true, 0.0);
    widthSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 24);
    addAndMakeVisible (widthSlider);

    hint.setText ("-100 % = mono    0 % = sin cambios    +100 % = ancho x2",
                  juce::dontSendNotification);
    hint.setJustificationType (juce::Justification::centred);
    hint.setFont (juce::FontOptions (11.0f));
    hint.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (hint);
}

void StereoExpanderEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1f26));
}

void StereoExpanderEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    title.setBounds (area.removeFromTop (34));
    hint.setBounds (area.removeFromBottom (24));
    widthSlider.setBounds (area.reduced (20, 6));
}
