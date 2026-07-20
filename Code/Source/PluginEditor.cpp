#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginAudioProcessorEditor::PluginAudioProcessorEditor(PluginAudioProcessor& p):
    AudioProcessorEditor(&p),
    audioProcessor(p),
    _layout(p, p)
{
    addAndMakeVisible(_layout, 10);

    setResizable(true, true);
    setResizeLimits(720, 480, 1920, 1200);
    setSize(960, 640);
}

void PluginAudioProcessorEditor::setBypass(bool isBypassed)
{
    _layout.setBypass(isBypassed);
}

void PluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setColour(nui::Theme::newColor(nui::Theme::BACKGROUND).asJuce());
    g.fillAll();
}

void PluginAudioProcessorEditor::resized()
{
    _layout.setBounds(getLocalBounds());
}
