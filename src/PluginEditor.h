#pragma once

#include "PluginProcessor.h"

class StereoExpanderEditor : public juce::AudioProcessorEditor
{
public:
    explicit StereoExpanderEditor (StereoExpanderProcessor&);
    ~StereoExpanderEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    StereoExpanderProcessor& processor;

    juce::Slider widthSlider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label title;
    juce::Label hint;
    juce::AudioProcessorValueTreeState::SliderAttachment widthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoExpanderEditor)
};
