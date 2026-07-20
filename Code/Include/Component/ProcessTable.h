#pragma once

#include <nierika_dsp/nierika_dsp.h>
#include <audio_capture_dsp/audio_capture_dsp.h>

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

    explicit ProcessTable(const std::string& identifier);
    ~ProcessTable() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    void setSearchText(const std::string& text);
    void setCategoryFilter(ProcessCategory category);
    void setHideBackgroundProcesses(bool hide);

    void refreshProcessList();

    void addOnProcessChosenListener(OnProcessChosenListener* listener);
    void removeListener(OnProcessChosenListener* listener);

    void setHighlightedProcessID(int processID);

private:
    enum ColumnId { NameColumn = 1, CategoryColumn, PidColumn };

    static constexpr int refreshIntervalMs = 5000;

    int getNumRows() override;
    void paintRowBackground(juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    void returnKeyPressed(int lastRowSelected) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    juce::String getCellTooltip(int rowNumber, int columnId) override;

    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void recomputeFilteredRows();
    void sortFilteredProcesses(int sortColumnId, bool isForwards);
    void applyThemeColours();
    void notifyProcessChosen(int row);

    std::vector<audiocapture::ProcessInfo> _allProcesses;
    std::vector<audiocapture::ProcessInfo> _filteredProcesses;
    ProcessFilterOptions _filterOptions;
    int _highlightedProcessID = 0;

    juce::TableListBox _table { "process-table-list", this };
    std::vector<OnProcessChosenListener*> _listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessTable)
};

}
