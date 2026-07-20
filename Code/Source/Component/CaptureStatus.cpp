#include "Component/CaptureStatus.h"
#include "AppSettings.h"
#include "AppLocalisation.h"

namespace component
{

CaptureStatus::CaptureStatus(PluginAudioProcessor& audioProcessor):
    Component("capture-status"),
    _audioProcessor(audioProcessor)
{
    addAndMakeVisible(_label);
    addAndMakeVisible(_process);

    _label.setFontSize(nui::Theme::PARAGRAPH);
    _label.setJustificationType(juce::Justification::centredLeft);
    _process.setFontSize(nui::Theme::CAPTION);
    _process.setFontWeight(nui::Theme::BOLD);
    _process.setJustificationType(juce::Justification::centredLeft);

    AppLocalisation::getChangeBroadcaster().addChangeListener(this);
}

CaptureStatus::~CaptureStatus()
{
    AppLocalisation::getChangeBroadcaster().removeChangeListener(this);
}

void CaptureStatus::resized()
{
    const auto labelFont = nui::Theme::newFont(_label.getFontFamily(), _label.getFontWeight(), _label.getFontSize());
    const auto labelWidth = juce::GlyphArrangement::getStringWidthInt(labelFont, juce::String(_label.getText()));

    _label.setBounds(0, 0, labelWidth + 16, getHeight());
    _process.setBounds(labelWidth + 4, 0, getWidth() - labelWidth - 16, getHeight());
}

void CaptureStatus::refresh()
{
    if (_audioProcessor.isCapturing())
    {
        _label.setText(juce::translate("capture_status_capturing").toStdString());
        _process.setText(_audioProcessor.getLastProcessName());
    }
    else
    {
        _label.setText(juce::translate("capture_status_not_capturing").toStdString());
        _process.setText("");
    }

    resized(); // label width tracks its text, so the layout needs recomputing whenever it changes
}

void CaptureStatus::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    Component::changeListenerCallback(source);

    if (source != &AppLocalisation::getChangeBroadcaster())
        return;

    if (_audioProcessor.isCapturing())
        _label.setText(juce::translate("capture_status_capturing").toStdString());
    else
        _label.setText(juce::translate("capture_status_not_capturing").toStdString());

    resized();
    repaint();
}

}
