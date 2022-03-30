/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DSPModuleChorusAudioProcessor::DSPModuleChorusAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    treeState.addParameterListener("rate", this);
    treeState.addParameterListener("depth", this);
    treeState.addParameterListener("delay", this);
    treeState.addParameterListener("feedback", this);
    treeState.addParameterListener("mix", this);
}

DSPModuleChorusAudioProcessor::~DSPModuleChorusAudioProcessor()
{
    treeState.removeParameterListener("rate", this);
    treeState.removeParameterListener("depth", this);
    treeState.removeParameterListener("delay", this);
    treeState.removeParameterListener("feedback", this);
    treeState.removeParameterListener("mix", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout DSPModuleChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    auto pRate = std::make_unique<juce::AudioParameterFloat>("rate", "Rate", 0, 100.0, 0.0);
    auto pDepth = std::make_unique<juce::AudioParameterFloat>("depth", "Depth", 0, 1.0, 0.0);
    auto pDelay = std::make_unique<juce::AudioParameterFloat>("delay", "Delay", 1, 100.0, 1.0);
    auto pFeedback = std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback", -1, 1.0, 0.0);
    auto pMix = std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0, 1.0, 0.0);
    
    params.push_back(std::move(pRate));
    params.push_back(std::move(pDepth));
    params.push_back(std::move(pDelay));
    params.push_back(std::move(pFeedback));
    params.push_back(std::move(pMix));
    
    return { params.begin(), params.end() };
}

void DSPModuleChorusAudioProcessor::parameterChanged(const juce::String &parameterID, float newValue)
{
    if(parameterID == "rate")
        rate = newValue;
    if(parameterID == "depth")
        depth = newValue;
    if(parameterID == "delay")
        delay = newValue;
    if(parameterID == "feedback")
        feedback = newValue;
    if(parameterID == "mix")
        mix = newValue;
}

//==============================================================================
const juce::String DSPModuleChorusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DSPModuleChorusAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DSPModuleChorusAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DSPModuleChorusAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DSPModuleChorusAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DSPModuleChorusAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DSPModuleChorusAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DSPModuleChorusAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DSPModuleChorusAudioProcessor::getProgramName (int index)
{
    return {};
}

void DSPModuleChorusAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DSPModuleChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    
    rate  = *treeState.getRawParameterValue("rate");
    depth  = *treeState.getRawParameterValue("depth");
    delay  = *treeState.getRawParameterValue("delay");
    feedback  = *treeState.getRawParameterValue("feedback");
    mix  = *treeState.getRawParameterValue("mix");

    chorus.prepare (spec);
    chorus.reset();
}

void DSPModuleChorusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DSPModuleChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DSPModuleChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    chorus.setRate(rate);
    chorus.setDepth(depth);
    chorus.setCentreDelay(delay);
    chorus.setFeedback(feedback);
    chorus.setMix(mix);
    
    // My audio block object
    juce::dsp::AudioBlock<float> block (buffer);
    
    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
}

//==============================================================================
bool DSPModuleChorusAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DSPModuleChorusAudioProcessor::createEditor()
{
//    return new DSPModuleChorusAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void DSPModuleChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    treeState.state.writeToStream(stream);
}

void DSPModuleChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Recall params
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
        
    if(tree.isValid())
    {
        treeState.state = tree;
        rate  = *treeState.getRawParameterValue("rate");
        depth  = *treeState.getRawParameterValue("depth");
        delay  = *treeState.getRawParameterValue("delay");
        feedback  = *treeState.getRawParameterValue("feedback");
        mix  = *treeState.getRawParameterValue("mix");
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DSPModuleChorusAudioProcessor();
}
