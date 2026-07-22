#pragma once

#include <nierika_dsp/nierika_dsp.h>
#include <audio_capture_dsp/audio_capture_dsp.h>

#include <memory>
#include <vector>

#include "ProcessFilter.h"

namespace component
{

class ProcessTable : public nui::Component, private juce::TableListBoxModel, private juce::Timer
{
public:
    struct OnProcessChosenListener
    {
        virtual ~OnProcessChosenListener() = default;
        virtual void onProcessChosen(const audiocapture::ProcessInfo& process) = 0;
    };
    struct OnProcessCaptureListener
    {
        virtual ~OnProcessCaptureListener() = default;
        virtual void onProcessCapture(const audiocapture::ProcessInfo& process) = 0;
    };

    explicit ProcessTable(const std::string& identifier);
    ~ProcessTable() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    void setSearchText(const std::string& text);
    void setCategoryFilter(ProcessCategory category);
    void setHideBackgroundProcesses(bool hide);

    void refreshProcessList();

    void addOnProcessCaptureListener(OnProcessCaptureListener* listener);
    void removeOnProcessCaptureListener(OnProcessCaptureListener* listener);
    void addOnProcessChosenListener(OnProcessChosenListener* listener);
    void removeOnProcessChosenListener(OnProcessChosenListener* listener);

    void setHighlightedProcessID(int processID);
    void setSelectedProcessID(int processID);
    int getSelectedProcessID() { return _selectedProcessID; }
    bool hasSelectedProcess() { return _selectedProcessID != 0; }
    std::optional<audiocapture::ProcessInfo> getSelectedProcess() { return hasSelectedProcess() ? getProcess(_selectedProcessID) : std::nullopt; }

    std::optional<audiocapture::ProcessInfo> getProcess(int processID);

private:
    enum ColumnId { NameColumn = 1, CategoryColumn, PidColumn, StatusColumn };

    static constexpr int refreshIntervalMs = 5000;
    static constexpr int rowHeight = 32;

    int getNumRows() override;
    void paintRowBackground(juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    void returnKeyPressed(int lastRowSelected) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    juce::String getCellTooltip(int rowNumber, int columnId) override;
    void listWasScrolled() override;

    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void recomputeFilteredRows();
    void sortFilteredProcesses(int sortColumnId, bool isForwards);
    void applyThemeColours();
    void notifyProcessChosen(int row);
    void notifyProcessCapture(int row);
    void updateCaptureIconPosition();

    std::vector<audiocapture::ProcessInfo> _allProcesses;
    std::vector<audiocapture::ProcessInfo> _filteredProcesses;
    ProcessFilterOptions _filterOptions;
    int _highlightedProcessID = 0;
    int _selectedProcessID = 0;

    nelement::AnimatedSVG _captureIcon { "animated-capture-icon", nui::AnimatedIcons::getCapture() };

    juce::TableListBox _table { "process-table-list", this };
    std::unique_ptr<juce::LookAndFeel> _headerLookAndFeel;
    std::vector<OnProcessChosenListener*> _processChosenListeners;
    std::vector<OnProcessCaptureListener*> _processCaptureListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessTable)
};

}
