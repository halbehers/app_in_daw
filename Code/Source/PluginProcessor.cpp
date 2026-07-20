#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "AppSettings.h"
#include "AppLocalisation.h"

PluginAudioProcessor::PluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), ndsp::ParameterManager(dynamic_cast<juce::AudioProcessor&>(*this), [this]() { return getParameterLayout(); })
#endif
{
    nui::Theme::configure({
        .mode = AppSettings::getInstance().getThemeMode(),
        .colorOverrides = {
            {
                nui::Theme::Mode::DARK,
                {
                    { nui::Theme::TEXT, juce::Colour(0xFFFFFFFF) },
                    { nui::Theme::INVERTED_TEXT, juce::Colour(0xFFFFFFFF) },
                    { nui::Theme::DISABLED, juce::Colour(0xFFA9A9A9) },
                    { nui::Theme::PRIMARY, juce::Colour(0xFF272727) },
                    { nui::Theme::ACCENT, juce::Colour(0xFF2D8CFF) },
                    { nui::Theme::BACKGROUND, juce::Colour(0xFF0F0F0F) },
                    { nui::Theme::SECONDARY_BACKGROUND, juce::Colour(0xFF191819) },
                    { nui::Theme::BORDER, juce::Colour(0xFF303030) },
                }
            },
            {
                nui::Theme::Mode::LIGHT,
                {
                    { nui::Theme::TEXT, juce::Colour(0xFF0E0E0E) },
                    { nui::Theme::INVERTED_TEXT, juce::Colour(0xFFFFFFFF) },
                    { nui::Theme::DISABLED, juce::Colour(0xFF616161) },
                    { nui::Theme::PRIMARY, juce::Colour(0xFFCAC8C7) },
                    { nui::Theme::ACCENT, juce::Colour(0xFF0A6FDB) },
                    { nui::Theme::BACKGROUND, juce::Colour(0xFFFFFFFF) },
                    { nui::Theme::SECONDARY_BACKGROUND, juce::Colour(0xFFF0EDEC) },
                    { nui::Theme::BORDER, juce::Colour(0xFFA5A5A5) },
                }
            }
        },
        .borderRadius = 8.f
    });

    _processCapture = std::make_unique<capture::SelectedProcessCapture>(audioCapture);

    setOutputTrimDb(AppSettings::getInstance().getOutputTrimDb());
    AppLocalisation::setLanguage(AppSettings::getInstance().getLanguage());

    startTimer(1000); // periodic latency logging, independent of whether an editor is open
}

PluginAudioProcessor::~PluginAudioProcessor()
{
    stopTimer();
    clearParameters();
}

void PluginAudioProcessor::selectProcess(int processID, const std::string& name, const std::string& executablePath)
{
    _lastProcessID = processID;
    _lastProcessName = name;
    _lastProcessExecutablePath = executablePath;

    _processCapture->setTargetProcessID(processID);
    _processCapture->startCapture();
    _isCapturing = true;
}

void PluginAudioProcessor::stopCapturing()
{
    _processCapture->stopCapture();
    _isCapturing = false;
}

void PluginAudioProcessor::tryReacquireLastProcess()
{
    if (_lastProcessExecutablePath.empty() && _lastProcessName.empty())
        return;

    const auto matches = audiocapture::ProcessList::filterProcesses(
        [this](const audiocapture::ProcessInfo& p)
        {
            if (!_lastProcessExecutablePath.empty())
                return p.executablePath == _lastProcessExecutablePath;
            return p.name == _lastProcessName;
        });

    if (matches.empty())
        return; // target app isn't running this session - leave state as "last known", not capturing

    selectProcess(matches.front().processID, matches.front().name, matches.front().executablePath);
}

void PluginAudioProcessor::timerCallback()
{
    nutils::AppLogger::info("Capture latency: " + std::to_string(getCurrentLatencyMs()) + " ms", "PluginAudioProcessor");
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginAudioProcessor::getParameterLayout()
{
    Parameters::registerAllSections(this);

    return buildParameterLayout();
}

const juce::String PluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PluginAudioProcessor::setCurrentProgram(int index)
{
    (void) index;
}

const juce::String PluginAudioProcessor::getProgramName(int index)
{
    (void) index;

    return {};
}

void PluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    (void) index;
    (void) newName;
}

//==============================================================================
void PluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getMainBusNumOutputChannels());

    // Resamples to the DAW's actual rate before writing to the fifo, since the capture source's
    // native rate is very unlikely to match it.
    audioCapture.prepare(sampleRate);
}

void PluginAudioProcessor::releaseResources()
{
    nutils::Logger::markShuttingDown();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void PluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    (void) midiMessages;

    audioCapture.process(buffer);
    buffer.applyGain(_outputTrimGain.load());
}

//==============================================================================
bool PluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginAudioProcessor::createEditor()
{
    return new PluginAudioProcessorEditor(*this);
}

//==============================================================================
void PluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree pluginState(Parameters::PLUGIN_STATE_ID);
    pluginState.setProperty(Parameters::PLUGIN_STATE_LAST_PROCESS_NAME_ID, juce::String(_lastProcessName), nullptr);
    pluginState.setProperty(Parameters::PLUGIN_STATE_LAST_PROCESS_PATH_ID, juce::String(_lastProcessExecutablePath), nullptr);
    pluginState.setProperty(Parameters::PLUGIN_STATE_LAST_PROCESS_PID_ID, _lastProcessID, nullptr);

    auto state = getState().state;
    state.removeChild(state.getChildWithName(Parameters::PLUGIN_STATE_ID), nullptr);
    state.appendChild(pluginState, nullptr);

    ndsp::ParameterManager::getStateInformation(destData);
}

void PluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ndsp::ParameterManager::setStateInformation(data, sizeInBytes);

    auto pluginState = getState().state.getChildWithName(Parameters::PLUGIN_STATE_ID);
    if (! pluginState.isValid())
        return;

    _lastProcessName = pluginState.getProperty(Parameters::PLUGIN_STATE_LAST_PROCESS_NAME_ID, "").toString().toStdString();
    _lastProcessExecutablePath = pluginState.getProperty(Parameters::PLUGIN_STATE_LAST_PROCESS_PATH_ID, "").toString().toStdString();
    _lastProcessID = (int) pluginState.getProperty(Parameters::PLUGIN_STATE_LAST_PROCESS_PID_ID, 0);

    tryReacquireLastProcess();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginAudioProcessor();
}
