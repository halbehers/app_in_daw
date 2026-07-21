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
    _captureStatus(audioProcessor),
    _captureButton("capture", nui::Icons::getStopInCircle()),
    _windowsManager(*this)
{
    ProcessCategoryConfig::ensureUserOverridesTemplateExists();

    _settings.setIconSize(24.f);
    _settings.addOnClickListener(this);
    _windowsManager.createWindow(std::make_unique<component::SettingsWindow>("settings", _windowsManager, audioProcessor));

    _searchBar.addOnValueChangedListener(this);
    _searchBar.setPlaceholder(juce::translate("search_placeholder").toStdString());
    _searchBar.setHeightType(nui::Theme::HeightType::LARGE);
    _searchBar.setRounded(true);

    populateCategoryFilter();
    _categoryFilter.setSelectedId(categoryToId(AppSettings::getInstance().getLastCategoryFilter()), juce::dontSendNotification);
    _categoryFilter.addOnValueChangedListener(this);
    _categoryFilter.setSelectedInvertedTextColor(true);
    _categoryFilter.setHeightType(nui::Theme::HeightType::LARGE);
    _categoryFilter.setSelectedInvertedTextColor(true);

    _hideBackgroundSwitch.setSelectedIndex(AppSettings::getInstance().getHideBackgroundProcesses() ? 1 : 0, juce::dontSendNotification);
    _hideBackgroundSwitch.addOnValueChangedListener(this);
    _hideBackgroundSwitch.setSelectedInvertedTextColor(true);
    _hideBackgroundSwitch.setHeightType(nui::Theme::HeightType::LARGE);
    _hideBackgroundSwitch.setRounded(true);
    
    _processTable.setCategoryFilter(idToCategory(_categoryFilter.getSelectedId()));
    _processTable.setHideBackgroundProcesses(_hideBackgroundSwitch.getSelectedIndex() == 1);
    _processTable.addOnProcessChosenListener(this);
    _processTable.addOnProcessCaptureListener(this);

    _captureButton.addOnClickListener(this);
    _captureButton.setIconSize(48.f);
    _captureButton.setClickableSurface(nui::helpers::ClickableSurface::ELEMENT_BOUNDARIES);

    AppLocalisation::getChangeBroadcaster().addChangeListener(this);

    if (_audioProcessor.isCapturing())
    {
        _processTable.setHighlightedProcessID(_audioProcessor.getLastProcessID());
    }
    else
    {
        _captureButton.setIconBinary(nui::Icons::getCaptureInCircle());
    }
    _captureButton.setEnabled(_audioProcessor.isCapturing() || _processTable.hasSelectedProcess());

    const auto selectedProcess = _processTable.getSelectedProcess();
    if (selectedProcess.has_value())
    {
        _captureStatus.setSelectedProcess(selectedProcess->name);
    }
    _captureStatus.refresh();

    getLayout().setGap(16.f);
    getLayout().setDisplayGrid(false);
    getLayout().setResizableLineConfiguration({ .displayLine = false });

    getLayout().setMargin(24.f, 24.f, 24.f, 24.f);
    getLayout().init({ 6, 48, 2, 4 }, { 2, 27, 4, 1, 15, 14 });

    getLayout().setFixedRowHeight(0, 60.f);
    getLayout().setFixedRowHeight(2, 20.f);
    getLayout().setFixedRowHeight(3, 40.f);

    getLayout().addComponent(_settings, 0, 0, 1, 1);
    getLayout().addComponent(_searchBar, 0, 1, 2, 1);
    getLayout().addComponent(_categoryFilter, 0, 3, 2, 1);
    getLayout().addComponent(_hideBackgroundSwitch, 0, 5, 1, 1);
    getLayout().addComponent(_processTable, 1, 0, 6, 1);
    getLayout().addComponent(_captureStatus, 3, 0, 2, 1);
    getLayout().addComponent(_captureButton, 2, 2, 2, 2);
    getLayout().addComponent(_performanceMonitor, 3, 4, 2, 1);
}

AppLayout::~AppLayout()
{
    _captureButton.removeListener(this);
    _processTable.removeOnProcessCaptureListener(this);
    _processTable.removeOnProcessChosenListener(this);
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

    _windowsManager.setBounds(getLocalBounds());
}

void AppLayout::onButtonClick(const std::string& componentID)
{
    if (componentID == _settings.getComponentID())
    {
        _windowsManager.showWindow("settings");
    }
    else if (componentID == _captureButton.getComponentID())
    {
        if (_audioProcessor.isCapturing()) {
            _audioProcessor.stopCapturing();
            _processTable.setHighlightedProcessID(0);
            _captureButton.setIconBinary(nui::Icons::getCaptureInCircle());
        }
        else
        {
            const auto process = _processTable.getProcess(_selectedProcessID);
            if (process.has_value())
            {
                _audioProcessor.selectProcess(process->processID, process->name, process->executablePath);
                _processTable.setHighlightedProcessID(process->processID);
                _captureButton.setIconBinary(nui::Icons::getStopInCircle());
            } else
            {
                _captureButton.setEnabled(false);
            }
        }
        _captureStatus.refresh();
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

void AppLayout::onProcessCapture(const audiocapture::ProcessInfo& process)
{
    _audioProcessor.selectProcess(process.processID, process.name, process.executablePath);
    _captureButton.setIconBinary(nui::Icons::getStopInCircle());
    _captureButton.setEnabled(true);
    _processTable.setHighlightedProcessID(process.processID);
    _captureStatus.refresh();
}

void AppLayout::onProcessChosen(const audiocapture::ProcessInfo& process)
{
    _selectedProcessID = process.processID;
    _captureButton.setEnabled(true);
    _captureStatus.setSelectedProcess(process.name);
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
    _captureStatus.refresh();
}
