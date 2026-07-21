#include "Component/ProcessTable.h"
#include "AppLocalisation.h"
#include "AppSettings.h"

#include <algorithm>
#include <functional>

namespace component
{

namespace
{
    constexpr int kAutoCategoryItemId = 5;

    struct CategoryCellOption { int id; ProcessCategory category; };

    const std::vector<CategoryCellOption>& getCategoryCellOptions()
    {
        static const std::vector<CategoryCellOption> options = {
            { 1, ProcessCategory::MusicAndMedia },
            { 2, ProcessCategory::Browsers },
            { 3, ProcessCategory::CommunicationAndMeetings },
            { 4, ProcessCategory::CreativeAndDAW },
        };
        return options;
    }

    int categoryToCellItemId(ProcessCategory category)
    {
        for (const auto& option : getCategoryCellOptions())
            if (option.category == category)
                return option.id;
        return kAutoCategoryItemId;
    }

    std::optional<ProcessCategory> cellItemIdToCategory(int id)
    {
        for (const auto& option : getCategoryCellOptions())
            if (option.id == id)
                return option.category;
        return std::nullopt; // kAutoCategoryItemId, or anything unrecognized
    }

    class CategoryCell final : public juce::Component, private nelement::ComboBox::OnValueChangedListener
    {
    public:
        using OnCategoryChosen = std::function<void(const audiocapture::ProcessInfo&, std::optional<ProcessCategory>)>;

        explicit CategoryCell(OnCategoryChosen onCategoryChosen)
            : _onCategoryChosen(std::move(onCategoryChosen))
        {
            addAndMakeVisible(_comboBox);

            for (const auto& option : getCategoryCellOptions())
                _comboBox.addItem(ProcessCategoryMatcher::getDisplayName(option.category), option.id);
            _comboBox.addItem(juce::translate("category_cell_auto"), kAutoCategoryItemId);

            _comboBox.addOnValueChangedListener(this);

            _comboBox.setBackgroundColour(juce::Colours::transparentWhite);
            _comboBox.setBorderColour(juce::Colours::transparentWhite);
            _comboBox.setSelectedInvertedTextColor(true);
            _comboBox.setTextHeight(nui::Theme::PARAGRAPH);
        }

        ~CategoryCell() override { _comboBox.removeListener(this); }

        void resized() override { _comboBox.setBounds(getLocalBounds()); }

        void setProcess(const audiocapture::ProcessInfo& process)
        {
            _process = process;

            const auto pinned = AppSettings::getInstance().getCategoryPin(process.name, process.executablePath);
            const auto resolved = pinned ? pinned : ProcessCategoryMatcher::categorize(process);

            _comboBox.setSelectedId(resolved ? categoryToCellItemId(*resolved) : kAutoCategoryItemId, juce::dontSendNotification);
        }

    private:
        void onSelectionChanged(const std::string& /*componentID*/, int selectedId) override
        {
            _onCategoryChosen(_process, cellItemIdToCategory(selectedId));
        }

        nelement::ComboBox _comboBox { "process-table-category-cell" };
        audiocapture::ProcessInfo _process;
        OnCategoryChosen _onCategoryChosen;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CategoryCell)
    };
}

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

void ProcessTable::addOnProcessCaptureListener(OnProcessCaptureListener* listener)
{
    _processCaptureListeners.push_back(listener);
}

void ProcessTable::removeOnProcessCaptureListener(OnProcessCaptureListener* listener)
{
    _processCaptureListeners.erase(std::remove(_processCaptureListeners.begin(), _processCaptureListeners.end(), listener), _processCaptureListeners.end());
}

void ProcessTable::addOnProcessChosenListener(OnProcessChosenListener* listener)
{
    _processChosenListeners.push_back(listener);
}

void ProcessTable::removeOnProcessChosenListener(OnProcessChosenListener* listener)
{
    _processChosenListeners.erase(std::remove(_processChosenListeners.begin(), _processChosenListeners.end(), listener), _processChosenListeners.end());
}

void ProcessTable::setHighlightedProcessID(int processID)
{
    _highlightedProcessID = processID;
    repaint();
}

void ProcessTable::setSelectedProcessID(int processID)
{
    _selectedProcessID = processID;
    repaint();
}

std::optional<audiocapture::ProcessInfo> ProcessTable::getProcess(int processID)
{
    const auto it = std::find_if(_allProcesses.begin(), _allProcesses.end(),
        [processID](const audiocapture::ProcessInfo& process) { return process.processID == processID; });

    if (it != _allProcesses.end())
        return *it;

    return std::nullopt;
}

int ProcessTable::getNumRows()
{
    return (int) _filteredProcesses.size();
}

void ProcessTable::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.setColour(nui::Theme::newColor(nui::Theme::ThemeColor::ACCENT).asJuce().withAlpha(.1f));
        g.fillRect(0, 0, width, height);
        return;
    } else if (_filteredProcesses[(size_t) rowNumber].processID == _highlightedProcessID)
    {
        g.setColour(nui::Theme::newColor(nui::Theme::ThemeColor::ACCENT).asJuce().withAlpha(.2f));
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

    const auto primaryTextColour = nui::Theme::newColor(nui::Theme::ThemeColor::TEXT).asJuce();
    const auto secondaryTextColour = nui::Theme::newColor(nui::Theme::ThemeColor::DISABLED).asJuce();

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

juce::Component* ProcessTable::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, juce::Component* existingComponentToUpdate)
{
    if (columnId != CategoryColumn)
    {
        delete existingComponentToUpdate; // Name/PID stay static text via paintCell().
        return nullptr;
    }

    auto* cell = dynamic_cast<CategoryCell*>(existingComponentToUpdate);
    if (cell == nullptr)
    {
        delete existingComponentToUpdate;
        cell = new CategoryCell([this](const audiocapture::ProcessInfo& process, std::optional<ProcessCategory> category)
        {
            if (category)
                AppSettings::getInstance().setCategoryPin(process.name, process.executablePath, *category);
            else
                AppSettings::getInstance().clearCategoryPin(process.name, process.executablePath);

            juce::MessageManager::callAsync([safePointer = juce::Component::SafePointer<ProcessTable>(this)]()
            {
                if (safePointer != nullptr)
                    safePointer->recomputeFilteredRows();
            });
        });
    }

    if (rowNumber >= 0 && rowNumber < (int) _filteredProcesses.size())
        cell->setProcess(_filteredProcesses[(size_t) rowNumber]);

    return cell;
}

void ProcessTable::cellDoubleClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    notifyProcessCapture(rowNumber);
}

void ProcessTable::cellClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    notifyProcessChosen(rowNumber);
}

void ProcessTable::returnKeyPressed(int lastRowSelected)
{
    notifyProcessCapture(lastRowSelected);
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
    _selectedProcessID = process.processID;

    for (auto* listener : _processChosenListeners)
        listener->onProcessChosen(process);
}

void ProcessTable::notifyProcessCapture(int row)
{
    if (row < 0 || row >= (int) _filteredProcesses.size())
        return;

    const auto process = _filteredProcesses[(size_t) row];
    for (auto* listener : _processCaptureListeners)
        listener->onProcessCapture(process);
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
