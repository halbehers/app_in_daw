#include "Component/PerformanceMonitor.h"
#include "AppSettings.h"
#include "AppLocalisation.h"

namespace component
{

PerformanceMonitor::PerformanceMonitor(PluginAudioProcessor& audioProcessor):
    Component("performance-monitor"),
    _audioProcessor(audioProcessor)
{
    if (!ENABLED)
    {
        setVisible(false);
        return;
    }

    addAndMakeVisible(_label);
    addAndMakeVisible(_latencyValue);

    _label.setFontSize(nui::Theme::PARAGRAPH);
    _label.setJustificationType(juce::Justification::centredRight);
    _latencyValue.setFontSize(nui::Theme::PARAGRAPH);

    AppLocalisation::getChangeBroadcaster().addChangeListener(this);

    startTimerHz(15);
}

PerformanceMonitor::~PerformanceMonitor()
{
    stopTimer();
    AppLocalisation::getChangeBroadcaster().removeChangeListener(this);
}

void PerformanceMonitor::resized()
{
    if (!ENABLED)
        return;

    _label.setBounds(getWidth() - LABEL_WIDTH - VALUE_WIDTH, 0, LABEL_WIDTH, getHeight());
    _latencyValue.setBounds(getWidth() - VALUE_WIDTH, 0, VALUE_WIDTH, getHeight());
}

void PerformanceMonitor::timerCallback()
{
    setVisible(AppSettings::getInstance().getShowLatencyMonitor());
    if (!isVisible())
        return;

    _latencyValue.setValue(nui::Formatter::formatDouble(_audioProcessor.getCurrentLatencyMs(), 1));
    _latencyValue.repaint();
}

void PerformanceMonitor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    Component::changeListenerCallback(source);

    if (source != &AppLocalisation::getChangeBroadcaster())
        return;

    _label.setText(juce::translate("performance_monitor_latency_label").toStdString());
    _latencyValue.setUnit(juce::translate("performance_monitor_latency_unit").toStdString());
    repaint();
}

}
