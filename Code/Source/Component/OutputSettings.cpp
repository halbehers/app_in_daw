#include "Component/OutputSettings.h"
#include "AppSettings.h"
#include "AppLocalisation.h"

#include <algorithm>

namespace component
{

OutputSettings::OutputSettings(const std::string& identifier, PluginAudioProcessor& audioProcessor):
    Component(identifier),
    _audioProcessor(audioProcessor)
{
    _title.setFontSize(nui::Theme::SMALL);
    _title.setColor(nui::Theme::ThemeColor::DISABLED);
    _title.setJustificationType(juce::Justification::centredLeft);

    const auto persistedDb = AppSettings::getInstance().getOutputTrimDb();
    const auto* match = std::find(std::begin(TRIM_VALUES_DB), std::end(TRIM_VALUES_DB), persistedDb);
    const auto initialIndex = match != std::end(TRIM_VALUES_DB) ? (int) (match - std::begin(TRIM_VALUES_DB)) : 0;
    _trimSwitch.setSelectedIndex(initialIndex, juce::dontSendNotification);
    _trimSwitch.addOnValueChangedListener(this);
    _trimSwitch.setSelectedInvertedTextColor(true);
    _trimSwitch.setHeightType(nui::Theme::HeightType::THIN);
    _trimSwitch.setRounded(true);

    AppLocalisation::getChangeBroadcaster().addChangeListener(this);

    _layout.setGap(8.f);
    _layout.setDisplayGrid(false);
    _layout.init({ 1, 1, 1 }, { 1 });

    _layout.addComponent(_title, 0, 0, 1, 1);
    _layout.addComponent(_trimSwitch, 1, 0, 1, 1);

    _layout.setBottomBorder(_title.getComponentID().toStdString(), nui::Theme::newColor(nui::Theme::ThemeColor::BORDER).asJuce());
}

OutputSettings::~OutputSettings()
{
    _trimSwitch.removeListener(this);
    AppLocalisation::getChangeBroadcaster().removeChangeListener(this);
}

void OutputSettings::paint(juce::Graphics& g)
{
    Component::paint(g);

    _layout.paint(g);
}

void OutputSettings::resized()
{
    Component::resized();

    _layout.resized();

    constexpr int switchWidth = 220;
    const auto switchCellBounds = _layout.getBounds(_trimSwitch.getID());
    _trimSwitch.setBounds(juce::Rectangle<float>((float) switchWidth, (float) _trimSwitch.getHeight())
        .withPosition(getControlX(switchCellBounds, (float) switchWidth), switchCellBounds.getCentreY() - (float) _trimSwitch.getHeight() / 2.f).toNearestInt());
}

float OutputSettings::getControlX(const juce::Rectangle<float>& cell, float controlWidth)
{
    return CONTROL_ALIGNMENT == ControlAlignment::Right ? cell.getRight() - controlWidth : cell.getX();
}

void OutputSettings::onSelectionChanged(const std::string& componentID, int selectedIndex)
{
    if (componentID != _trimSwitch.getComponentID())
        return;

    const auto db = TRIM_VALUES_DB[selectedIndex];
    AppSettings::getInstance().setOutputTrimDb(db);
    _audioProcessor.setOutputTrimDb(db);
}

void OutputSettings::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    Component::changeListenerCallback(source);

    if (source == &nui::Theme::getChangeBroadcaster())
    {
        _layout.setBottomBorder(_title.getComponentID().toStdString(), nui::Theme::newColor(nui::Theme::ThemeColor::BORDER).asJuce());
        repaint();
    }

    if (source != &AppLocalisation::getChangeBroadcaster())
        return;

    _title.setText(juce::translate("output_trim_title").toStdString());
    _trimSwitch.setLabels(juce::translate("output_trim_0db").toStdString(), juce::translate("output_trim_neg6db").toStdString(), juce::translate("output_trim_neg12db").toStdString());

    repaint();
}

}
