#include "AppLayout.h"
#include "AppSettings.h"
#include "AppLocalisation.h"

#include <vector>

namespace
{
    struct CategoryOption { int id; ProcessCategory category; };

    const std::vector<CategoryOption>& getCategoryOptions()
    {
        static const std::vector<CategoryOption> options = {
            { 1, ProcessCategory::All },
            { 2, ProcessCategory::MusicAndMedia },
            { 3, ProcessCategory::Browsers },
            { 4, ProcessCategory::CommunicationAndMeetings },
            { 5, ProcessCategory::CreativeAndDAW },
        };
        return options;
    }

    int categoryToId(ProcessCategory category)
    {
        for (const auto& option : getCategoryOptions())
            if (option.category == category)
                return option.id;
        return 1; // All
    }

    ProcessCategory idToCategory(int id)
    {
        for (const auto& option : getCategoryOptions())
            if (option.id == id)
                return option.category;
        return ProcessCategory::All;
    }
}

AppLayout::AppLayout(ndsp::ParameterManager& parameterManager, PluginAudioProcessor& audioProcessor):
    nlayout::AppLayout(parameterManager),
    _audioProcessor(audioProcessor),
    _settings("settings", nui::Icons::getGear()),
    _searchBar("search-bar"),
    _categoryFilter("category-filter"),
    _hideBackgroundSwitch("hide-background-switch", juce::translate("hide_background_switch_all").toStdString(), juce::translate("hide_background_switch_main_only").toStdString()),
    _processTable("process-table"),
    _performanceMonitor(audioProcessor),
    _captureStatusLabel("capture-status-label", "", juce::translate("capture_status_not_capturing").toStdString()),
    _stopCaptureButton("stop-capture-button", juce::translate("stop_capture_button").toStdString()),
    _windowsManager(*this)
{
    // Best-effort: give the user a real, valid template file to copy from on first real launch.
    // Not done inside ProcessCategoryMatcher::categorize()/getCategoryTable() - see
    // ProcessCategory.cpp - so headless unit tests (which construct PluginAudioProcessor directly
    // but never AppLayout) never write into the real ~/Library folder.
    ProcessCategoryConfig::ensureUserOverridesTemplateExists();

    _settings.setIconSize(24.f);
    _settings.addOnClickListener(this);
    _windowsManager.createWindow(std::make_unique<component::SettingsWindow>("settings", _windowsManager, audioProcessor));

    _searchBar.addOnValueChangedListener(this);
    _searchBar.setPlaceholder(juce::translate("search_placeholder").toStdString());
    _searchBar.setBorderRadius(20.f);
    _searchBar.setHeightType(nui::Theme::HeightType::LARGE);

    populateCategoryFilter();
    _categoryFilter.setSelectedId(categoryToId(AppSettings::getInstance().getLastCategoryFilter()), juce::dontSendNotification);
    _categoryFilter.addOnValueChangedListener(this);
    _categoryFilter.setSelectedInvertedTextColor(true);
    _categoryFilter.setHeightType(nui::Theme::HeightType::LARGE);

    _processTable.setCategoryFilter(idToCategory(_categoryFilter.getSelectedId()));

    _hideBackgroundSwitch.setSelectedIndex(AppSettings::getInstance().getHideBackgroundProcesses() ? 1 : 0, juce::dontSendNotification);
    _hideBackgroundSwitch.addOnValueChangedListener(this);
    _hideBackgroundSwitch.setSelectedInvertedTextColor(true);
    _hideBackgroundSwitch.setHeightType(nui::Theme::HeightType::LARGE);
    _processTable.setHideBackgroundProcesses(_hideBackgroundSwitch.getSelectedIndex() == 1);

    _processTable.addOnProcessChosenListener(this);

    _stopCaptureButton.addOnClickListener(this);
    _stopCaptureButton.setColour(juce::TextButton::ColourIds::buttonColourId, nui::Theme::newColor(nui::Theme::ThemeColor::SECONDARY_BACKGROUND).asJuce());

    AppLocalisation::getChangeBroadcaster().addChangeListener(this);

    addAndMakeVisible(_performanceMonitor);
    addAndMakeVisible(_captureStatusLabel);
    addAndMakeVisible(_stopCaptureButton);

    if (_audioProcessor.isCapturing())
        _processTable.setHighlightedProcessID(_audioProcessor.getLastProcessID());
    refreshCaptureStatusLabel();

    getLayout().setGap(16.f);
    getLayout().setDisplayGrid(false);
    getLayout().setResizableLineConfiguration({ .displayLine = false });

    getLayout().setMargin(24.f, 24.f, 24.f, 12.f + 20.f + 16.f);
    getLayout().init({ 6, 54 }, { 2, 30, 16, 14 });

    getLayout().addComponent(_settings, 0, 0, 1, 1);
    getLayout().addComponent(_searchBar, 0, 1, 1, 1);
    getLayout().addComponent(_categoryFilter, 0, 2, 1, 1);
    getLayout().addComponent(_hideBackgroundSwitch, 0, 3, 1, 1);
    getLayout().addComponent(_processTable, 1, 0, 4, 1);
}

AppLayout::~AppLayout()
{
    _stopCaptureButton.removeListener(this);
    _processTable.removeListener(this);
    _hideBackgroundSwitch.removeListener(this);
    _categoryFilter.removeListener(this);
    _searchBar.removeOnValueChangedListener(this);
    _settings.removeListener(this);
    AppLocalisation::getChangeBroadcaster().removeChangeListener(this);
}

void AppLayout::populateCategoryFilter()
{
    _categoryFilter.addItem(ProcessCategoryMatcher::getDisplayName(ProcessCategory::All), categoryToId(ProcessCategory::All));
    _categoryFilter.addItem(ProcessCategoryMatcher::getDisplayName(ProcessCategory::MusicAndMedia), categoryToId(ProcessCategory::MusicAndMedia));
    _categoryFilter.addItem(ProcessCategoryMatcher::getDisplayName(ProcessCategory::Browsers), categoryToId(ProcessCategory::Browsers));
    _categoryFilter.addItem(ProcessCategoryMatcher::getDisplayName(ProcessCategory::CommunicationAndMeetings), categoryToId(ProcessCategory::CommunicationAndMeetings));
    _categoryFilter.addItem(ProcessCategoryMatcher::getDisplayName(ProcessCategory::CreativeAndDAW), categoryToId(ProcessCategory::CreativeAndDAW));
}

void AppLayout::resized()
{
    nlayout::AppLayout::resized();

    // ComboBox/TwoWaySwitch look wrong stretched to an arbitrary grid cell (native combo-box
    // height convention, switch's pill shape) - fixed-size within their cell, same pattern used
    // throughout the settings sections. SVGButton/TextInput are fine auto-filling their cell
    // (icon/text stay fixed-size internally regardless of bounds).
    constexpr int categoryFilterWidth = 200;
    const auto categoryFilterHeight = static_cast<int>(nui::Theme::getLargeHeight());
    const auto categoryCellBounds = getLayout().getBounds(_categoryFilter.getComponentID().toStdString());
    _categoryFilter.setBounds(juce::Rectangle<float>((float) categoryFilterWidth, (float) categoryFilterHeight)
        .withCentre(categoryCellBounds.getCentre()).toNearestInt());

    constexpr int hideBackgroundSwitchWidth = 200;
    const auto hideBackgroundSwitchHeight = static_cast<int>(nui::Theme::getLargeHeight());
    const auto hideBackgroundCellBounds = getLayout().getBounds(_hideBackgroundSwitch.getComponentID().toStdString());
    _hideBackgroundSwitch.setBounds(juce::Rectangle<float>((float) hideBackgroundSwitchWidth, (float) hideBackgroundSwitchHeight)
        .withPosition(hideBackgroundCellBounds.getRight() - (float) hideBackgroundSwitchWidth,
                       hideBackgroundCellBounds.getCentreY() - (float) hideBackgroundSwitchHeight / 2.f).toNearestInt());

    const auto strip = getLayout().getRectangleAtBottom(20.f, 16.f);

    // Latency monitor is LEFT-aligned here - opposite of youtube_in_daw's right-aligned placement.
    _performanceMonitor.setBounds(juce::Rectangle<float>(
        strip.getX(), strip.getY(), (float) component::PerformanceMonitor::WIDTH, strip.getHeight()).toNearestInt());

    constexpr int gap = 16;
    constexpr int stopButtonWidth = 90;

    const auto statusX = strip.getX() + (float) component::PerformanceMonitor::WIDTH + (float) gap;
    const auto statusWidth = juce::jmax(0.f, strip.getRight() - (float) stopButtonWidth - (float) gap - statusX);

    _captureStatusLabel.setBounds(juce::Rectangle<float>(statusX, strip.getY(), statusWidth, strip.getHeight()).toNearestInt());

    _stopCaptureButton.setBounds(juce::Rectangle<float>(
        strip.getRight() - (float) stopButtonWidth, strip.getY(), (float) stopButtonWidth, strip.getHeight()).toNearestInt());

    _windowsManager.setBounds(getLocalBounds());
}

void AppLayout::onButtonClick(const std::string& componentID)
{
    if (componentID == _settings.getComponentID())
    {
        _windowsManager.showWindow("settings");
    }
    else if (componentID == _stopCaptureButton.getComponentID())
    {
        _audioProcessor.stopCapturing();
        _processTable.setHighlightedProcessID(0);
        refreshCaptureStatusLabel();
    }
}

void AppLayout::onValueChanged(const std::string& componentID, const std::string& newValue)
{
    if (componentID == _searchBar.getComponentID())
        _processTable.setSearchText(newValue);
}

void AppLayout::onSelectionChanged(const std::string& componentID, int selectedId)
{
    if (componentID == _categoryFilter.getComponentID())
    {
        const auto category = idToCategory(selectedId);
        _processTable.setCategoryFilter(category);
        AppSettings::getInstance().setLastCategoryFilter(category);
    }
    else if (componentID == _hideBackgroundSwitch.getComponentID())
    {
        const auto hide = selectedId == 1;
        _processTable.setHideBackgroundProcesses(hide);
        AppSettings::getInstance().setHideBackgroundProcesses(hide);
    }
}

void AppLayout::onProcessChosen(const audiocapture::ProcessInfo& process)
{
    _audioProcessor.selectProcess(process.processID, process.name, process.executablePath);
    _processTable.setHighlightedProcessID(process.processID);
    refreshCaptureStatusLabel();
}

void AppLayout::refreshCaptureStatusLabel()
{
    if (_audioProcessor.isCapturing())
        _captureStatusLabel.setText(juce::translate("capture_status_capturing").toStdString()
            .append(_audioProcessor.getLastProcessName()));
    else
        _captureStatusLabel.setText(juce::translate("capture_status_not_capturing").toStdString());
}

void AppLayout::bypassComponents(bool isBypassed)
{
    _processTable.setEnabled(!isBypassed);
}

void AppLayout::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    Component::changeListenerCallback(source);

    if (source != &AppLocalisation::getChangeBroadcaster())
        return;

    _searchBar.setPlaceholder(juce::translate("search_placeholder").toStdString());
    _hideBackgroundSwitch.setLabels(juce::translate("hide_background_switch_all").toStdString(), juce::translate("hide_background_switch_main_only").toStdString());
    _stopCaptureButton.setButtonText(juce::translate("stop_capture_button").toStdString());
    refreshCaptureStatusLabel();
    repaint();
}
