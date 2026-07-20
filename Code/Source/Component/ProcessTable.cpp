#include "Component/ProcessTable.h"
#include "AppLocalisation.h"

#include <algorithm>

namespace component
{

ProcessTable::ProcessTable(const std::string& identifier)
    : Component(identifier)
{
    addAndMakeVisible(_table);
    _table.setModel(this);
    _table.setMultipleSelectionEnabled(false);
    _table.setRowHeight(28);
    _table.setHeaderHeight(28);
    _table.setOutlineThickness(0);

    auto& header = _table.getHeader();
    header.addColumn(juce::translate("process_table_name_column"), NameColumn, 260, 120, -1);
    header.addColumn(juce::translate("process_table_category_column"), CategoryColumn, 160, 100, -1);
    header.addColumn(juce::translate("process_table_pid_column"), PidColumn, 80, 60, 120);
    header.setSortColumnId(NameColumn, true);

    setTooltip(juce::translate("process_table_tooltip").toStdString());

    applyThemeColours();

    nui::Theme::getChangeBroadcaster().addChangeListener(this);
    AppLocalisation::getChangeBroadcaster().addChangeListener(this);

    refreshProcessList();
}

ProcessTable::~ProcessTable()
{
    nui::Theme::getChangeBroadcaster().removeChangeListener(this);
    AppLocalisation::getChangeBroadcaster().removeChangeListener(this);
}

void ProcessTable::paint(juce::Graphics& g)
{
    Component::paint(g);
}

void ProcessTable::resized()
{
    Component::resized();

    _table.setBounds(getLocalBounds());
}

void ProcessTable::visibilityChanged()
{
    if (isVisible())
    {
        refreshProcessList();
        startTimer(refreshIntervalMs);
    }
    else
    {
        stopTimer();
    }
}

void ProcessTable::setSearchText(const std::string& text)
{
    _filterOptions.searchText = text;
    recomputeFilteredRows();
}

void ProcessTable::setCategoryFilter(ProcessCategory category)
{
    _filterOptions.category = category;
    recomputeFilteredRows();
}

void ProcessTable::setHideBackgroundProcesses(bool hide)
{
    _filterOptions.hideBackgroundProcesses = hide;
    recomputeFilteredRows();
}

void ProcessTable::refreshProcessList()
{
    _allProcesses = audiocapture::ProcessList::filterProcesses([](const audiocapture::ProcessInfo& process) { return !process.name.empty(); });
    recomputeFilteredRows();
}

void ProcessTable::addOnProcessChosenListener(OnProcessChosenListener* listener)
{
    _listeners.push_back(listener);
}

void ProcessTable::removeListener(OnProcessChosenListener* listener)
{
    _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener), _listeners.end());
}

void ProcessTable::setHighlightedProcessID(int processID)
{
    _highlightedProcessID = processID;
    repaint();
}

int ProcessTable::getNumRows()
{
    return (int) _filteredProcesses.size();
}

void ProcessTable::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.setColour(nui::Theme::newColor(nui::Theme::ThemeColor::ACCENT).asJuce());
        g.fillRect(0, 0, width, height);
        return;
    }

    g.setColour(nui::Theme::newColor(rowNumber % 2 == 0
        ? nui::Theme::ThemeColor::BACKGROUND
        : nui::Theme::ThemeColor::SECONDARY_BACKGROUND).asJuce());
    g.fillRect(0, 0, width, height);
}

void ProcessTable::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int) _filteredProcesses.size())
        return;

    const auto& process = _filteredProcesses[(size_t) rowNumber];

    constexpr int textPadding = 8;
    constexpr float dotSize = 6.f;

    g.setFont(nui::Theme::newFont(nui::Theme::REGULAR, nui::Theme::PARAGRAPH));

    const auto primaryTextColour = rowIsSelected
        ? nui::Theme::newColor(nui::Theme::ThemeColor::INVERTED_TEXT).asJuce()
        : nui::Theme::newColor(nui::Theme::ThemeColor::TEXT).asJuce();
    const auto secondaryTextColour = rowIsSelected
        ? nui::Theme::newColor(nui::Theme::ThemeColor::INVERTED_TEXT).asJuce()
        : nui::Theme::newColor(nui::Theme::ThemeColor::DISABLED).asJuce();

    switch (columnId)
    {
        case NameColumn:
        {
            int textX = textPadding;

            if (process.processID == _highlightedProcessID)
            {
                g.setColour(nui::Theme::newColor(nui::Theme::ThemeColor::ACCENT).asJuce());
                g.fillEllipse((float) textX, (float) height / 2.f - dotSize / 2.f, dotSize, dotSize);
                textX += (int) dotSize + textPadding;
            }

            g.setColour(primaryTextColour);
            g.drawText(process.name, textX, 0, width - textX - textPadding, height, juce::Justification::centredLeft, true);
            break;
        }
        case CategoryColumn:
        {
            const auto category = ProcessCategoryMatcher::categorize(process);
            const auto text = category ? ProcessCategoryMatcher::getDisplayName(*category) : juce::String();

            g.setColour(secondaryTextColour);
            g.drawText(text, textPadding, 0, width - textPadding * 2, height, juce::Justification::centredLeft, true);
            break;
        }
        case PidColumn:
        {
            g.setColour(secondaryTextColour);
            g.drawText(juce::String(process.processID), textPadding, 0, width - textPadding * 2, height, juce::Justification::centredLeft, true);
            break;
        }
        default:
            break;
    }
}

void ProcessTable::cellDoubleClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    notifyProcessChosen(rowNumber);
}

void ProcessTable::returnKeyPressed(int lastRowSelected)
{
    notifyProcessChosen(lastRowSelected);
}

void ProcessTable::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    sortFilteredProcesses(newSortColumnId, isForwards);
    _table.updateContent();
}

juce::String ProcessTable::getCellTooltip(int rowNumber, int /*columnId*/)
{
    if (rowNumber < 0 || rowNumber >= (int) _filteredProcesses.size())
        return {};

    return juce::String(_filteredProcesses[(size_t) rowNumber].executablePath);
}

void ProcessTable::timerCallback()
{
    refreshProcessList();
}

void ProcessTable::recomputeFilteredRows()
{
    _filteredProcesses = ProcessFilter::apply(_allProcesses, _filterOptions);

    const auto& header = _table.getHeader();
    sortFilteredProcesses(header.getSortColumnId(), header.isSortedForwards());

    _table.updateContent();
    repaint();
}

void ProcessTable::sortFilteredProcesses(int sortColumnId, bool isForwards)
{
    auto compare = [sortColumnId](const audiocapture::ProcessInfo& a, const audiocapture::ProcessInfo& b) -> bool
    {
        switch (sortColumnId)
        {
            case PidColumn:
                return a.processID < b.processID;
            case CategoryColumn:
            {
                const auto categoryA = ProcessCategoryMatcher::categorize(a);
                const auto categoryB = ProcessCategoryMatcher::categorize(b);
                const auto nameA = categoryA ? ProcessCategoryMatcher::getDisplayName(*categoryA) : juce::String();
                const auto nameB = categoryB ? ProcessCategoryMatcher::getDisplayName(*categoryB) : juce::String();
                return nameA.compareIgnoreCase(nameB) < 0;
            }
            case NameColumn:
            default:
                return juce::String(a.name).compareIgnoreCase(juce::String(b.name)) < 0;
        }
    };

    std::stable_sort(_filteredProcesses.begin(), _filteredProcesses.end(), compare);
    if (!isForwards)
        std::reverse(_filteredProcesses.begin(), _filteredProcesses.end());
}

void ProcessTable::applyThemeColours()
{
    const auto background = nui::Theme::newColor(nui::Theme::ThemeColor::BACKGROUND).asJuce();
    const auto text = nui::Theme::newColor(nui::Theme::ThemeColor::TEXT).asJuce();
    const auto border = nui::Theme::newColor(nui::Theme::ThemeColor::BORDER).asJuce();
    const auto secondaryBackground = nui::Theme::newColor(nui::Theme::ThemeColor::SECONDARY_BACKGROUND).asJuce();
    const auto accent = nui::Theme::newColor(nui::Theme::ThemeColor::ACCENT).asJuce();

    _table.setColour(juce::ListBox::backgroundColourId, background);
    _table.setColour(juce::ListBox::outlineColourId, border);
    _table.setColour(juce::ListBox::textColourId, text);

    _table.getHeader().setColour(juce::TableHeaderComponent::backgroundColourId, secondaryBackground);
    _table.getHeader().setColour(juce::TableHeaderComponent::textColourId, text);
    _table.getHeader().setColour(juce::TableHeaderComponent::outlineColourId, border);
    _table.getHeader().setColour(juce::TableHeaderComponent::highlightColourId, accent.withAlpha(0.3f));
}

void ProcessTable::notifyProcessChosen(int row)
{
    if (row < 0 || row >= (int) _filteredProcesses.size())
        return;

    const auto process = _filteredProcesses[(size_t) row];
    for (auto* listener : _listeners)
        listener->onProcessChosen(process);
}

void ProcessTable::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    Component::changeListenerCallback(source);

    if (source == &nui::Theme::getChangeBroadcaster())
    {
        applyThemeColours();
        repaint();
    }

    if (source == &AppLocalisation::getChangeBroadcaster())
    {
        auto& header = _table.getHeader();
        header.setColumnName(NameColumn, juce::translate("process_table_name_column"));
        header.setColumnName(CategoryColumn, juce::translate("process_table_category_column"));
        header.setColumnName(PidColumn, juce::translate("process_table_pid_column"));
        setTooltip(juce::translate("process_table_tooltip").toStdString());
        repaint();
    }
}

}
