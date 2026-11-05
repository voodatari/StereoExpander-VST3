#include "PluginProcessor.h"
#include "PluginEditor.h"

StereoExpanderProcessor::StereoExpanderProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    widthParam = apvts.getRawParameterValue (widthParamID);
}

juce::AudioProcessorValueTreeState::ParameterLayout StereoExpanderProcessor::createLayout()
{
    // Rango y valor por defecto idénticos al original: -100..+100 %, por defecto 0 %.
    auto attrs = juce::AudioParameterFloatAttributes().withLabel ("%");
    return { std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { widthParamID, 1 }, "Width",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f, attrs) };
}

bool StereoExpanderProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Salida siempre estéreo; entrada estéreo (mid/side) o mono (pseudo-estéreo).
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void StereoExpanderProcessor::prepareToPlay (double sampleRate, int)
{
    dsp.prepare (sampleRate);
    widthSmoothed.reset (sampleRate, 0.010); // 10 ms: evita "zipper noise" al mover el control
    widthSmoothed.setCurrentAndTargetValue (se::StereoExpanderDSP::widthFromPercent (widthParam->load()));
}

void StereoExpanderProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();

    for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
        if (ch >= 1)
            buffer.clear (ch, 0, numSamples);

    widthSmoothed.setTargetValue (se::StereoExpanderDSP::widthFromPercent (widthParam->load()));

    float* left = buffer.getWritePointer (0);

    if (numIn >= 2 && buffer.getNumChannels() >= 2)
    {
        float* right = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
            dsp.processStereoSample (left[i], right[i], widthSmoothed.getNextValue());
    }
    else if (buffer.getNumChannels() >= 2)
    {
        float* right = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            float l, r;
            dsp.processMonoSample (left[i], widthSmoothed.getNextValue(), l, r);
            left[i] = l;
            right[i] = r;
        }
    }
}

juce::AudioProcessorEditor* StereoExpanderProcessor::createEditor()
{
    return new StereoExpanderEditor (*this);
}

void StereoExpanderProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void StereoExpanderProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoExpanderProcessor();
}
