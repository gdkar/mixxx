/**
* @file dlgprefcontroller.cpp
* @author Sean M. Pappalardo  spappalardo@mixxx.org
* @date Mon May 2 2011
* @brief Configuration dialog for a DJ controller
*/

#include <QtDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDesktopServices>
#include <QtAlgorithms>

#include "controllers/dlgprefcontroller.h"
#include "controllers/controllerlearningeventfilter.h"
#include "controllers/controller.h"
#include "controllers/controllermanager.h"
#include "controllers/defs_controllers.h"
#include "preferences/usersettings.h"
#include "util/version.h"

DlgPrefController::DlgPrefController(QWidget* parent, Controller* controller,
                                     ControllerManager* controllerManager,
                                     UserSettingsPointer pConfig)
        : DlgPreferencePage(parent),
          m_pConfig(pConfig),
          m_pControllerManager(controllerManager),
          m_pController(controller),
          m_pDlgControllerLearning(nullptr),
          m_pInputTableModel(nullptr),
          m_pInputProxyModel(nullptr),
          m_pOutputTableModel(nullptr),
          m_pOutputProxyModel(nullptr),
          m_bDirty(false) {
    m_ui.setupUi(this);

    initTableView(m_ui.m_pInputMappingTableView);
    initTableView(m_ui.m_pOutputMappingTableView);
    initTableView(m_ui.m_pScriptsTableWidget);

    connect(m_pController, SIGNAL(presetLoaded(ControllerPresetPointer)),
            this, SLOT(slotPresetLoaded(ControllerPresetPointer)));
    // TODO(rryan): Eh, this really isn't thread safe but it's the way it's been
    // since 1.11.0. We shouldn't be calling Controller methods because it lives
    // in a different thread. Booleans (like isOpen()) are fine but a complex
    // object like a preset involves QHash's and other data structures that
    // really don't like concurrent access.
    auto pPreset = m_pController->getPreset();
    slotPresetLoaded(pPreset);
    m_ui.labelDeviceName->setText(m_pController->getName());
    auto category = m_pController->getCategory();
    if (!category.isEmpty()) {
        m_ui.labelDeviceCategory->setText(category);
    } else {
        m_ui.labelDeviceCategory->hide();
    }

    // When the user picks a preset, load it.
    connect(m_ui.comboBoxPreset, SIGNAL(activated(int)),
            this, SLOT(slotLoadPreset(int)));

    // When the user toggles the Enabled checkbox, toggle.
    connect(m_ui.chkEnabledDevice, SIGNAL(clicked(bool)),
            this, SLOT(slotEnableDevice(bool)));

    // Connect our signals to controller manager.
    connect(this, SIGNAL(openController(Controller*)),
            m_pControllerManager, SLOT(openController(Controller*)));
    connect(this, SIGNAL(closeController(Controller*)),
            m_pControllerManager, SLOT(closeController(Controller*)));
    connect(this, SIGNAL(loadPreset(Controller*, ControllerPresetPointer)),
            m_pControllerManager, SLOT(loadPreset(Controller*, ControllerPresetPointer)));

    // Input mappings
    connect(m_ui.btnAddInputMapping, SIGNAL(clicked()),
            this, SLOT(addInputMapping()));
    connect(m_ui.btnRemoveInputMappings, SIGNAL(clicked()),
            this, SLOT(removeInputMappings()));
    connect(m_ui.btnLearningWizard, SIGNAL(clicked()),
            this, SLOT(showLearningWizard()));
    connect(m_ui.btnClearAllInputMappings, SIGNAL(clicked()),
            this, SLOT(clearAllInputMappings()));

    // Output mappings
    connect(m_ui.btnAddOutputMapping, SIGNAL(clicked()),
            this, SLOT(addOutputMapping()));
    connect(m_ui.btnRemoveOutputMappings, SIGNAL(clicked()),
            this, SLOT(removeOutputMappings()));
    connect(m_ui.btnClearAllOutputMappings, SIGNAL(clicked()),
            this, SLOT(clearAllOutputMappings()));

    // Scripts
    connect(m_ui.m_pScriptsTableWidget, SIGNAL(cellChanged(int, int)),
            this, SLOT(slotDirty()));
    connect(m_ui.btnAddScript, SIGNAL(clicked()),
            this, SLOT(addScript()));
    connect(m_ui.btnRemoveScript, SIGNAL(clicked()),
            this, SLOT(removeScript()));
    connect(m_ui.btnOpenScript, SIGNAL(clicked()),
            this, SLOT(openScript()));
    slotUpdate();
}
DlgPrefController::~DlgPrefController() = default;

void DlgPrefController::showLearningWizard() {
    // If the user has checked the "Enabled" checkbox but they haven't hit OK to
    // apply it yet, prompt them to apply the settings before we open the
    // learning dialog. If we don't apply the settings first and open the
    // device, the dialog won't react to controller messages.
    if (m_ui.chkEnabledDevice->isChecked() && !m_pController->isOpen()) {
        auto result = QMessageBox::question(
            this,
            tr("Apply device settings?"),
            tr("Your settings must be applied before starting the learning wizard.\n"
               "Apply settings and continue?"),
            QMessageBox::Ok | QMessageBox::Cancel,  // Buttons to be displayed
            QMessageBox::Ok);  // Default button
        // Stop if the user has not pressed the Ok button,
        // which could be the Cancel or the Close Button.
        if (result != QMessageBox::Ok)
            return;
    }
    slotApply();

    // After this point we consider the mapping wizard as dirtying the preset.
    slotDirty();
    // Note that DlgControllerLearning is set to delete itself on close using
    // the Qt::WA_DeleteOnClose attribute (so this "new" doesn't leak memory)
    m_pDlgControllerLearning = new DlgControllerLearning(this, m_pController);
    m_pDlgControllerLearning->show();
    auto pControllerLearning = m_pControllerManager->getControllerLearningEventFilter();
    pControllerLearning->startListening();
    connect(pControllerLearning, SIGNAL(controlClicked(ControlObject*)),
            m_pDlgControllerLearning, SLOT(controlClicked(ControlObject*)));
    connect(m_pDlgControllerLearning, SIGNAL(listenForClicks()),
            pControllerLearning, SLOT(startListening()));
    connect(m_pDlgControllerLearning, SIGNAL(stopListeningForClicks()),
            pControllerLearning, SLOT(stopListening()));
    connect(m_pDlgControllerLearning, SIGNAL(stopLearning()),
            this, SLOT(show()));
    connect(m_pDlgControllerLearning, SIGNAL(inputMappingsLearned(MidiInputMappings)),
            this, SLOT(midiInputMappingsLearned(MidiInputMappings)));

    emit(mappingStarted());
    connect(m_pDlgControllerLearning, SIGNAL(stopLearning()),this, SIGNAL(mappingEnded()));
}
void DlgPrefController::midiInputMappingsLearned(const MidiInputMappings& mappings) {
    // This is just a shortcut since doing a round-trip from Learning ->
    // Controller -> slotPresetLoaded -> setPreset is too heavyweight.
    if (m_pInputTableModel)
        m_pInputTableModel->addMappings(mappings);
}
QString DlgPrefController::presetShortName(const ControllerPresetPointer pPreset) const {
    auto presetName = tr("None");
    if (pPreset) {
        auto name = pPreset->name();
        auto author = pPreset->author();
        if (name.length() > 0 && author.length() > 0) {
            presetName = tr("%1 by %2").arg(pPreset->name(), pPreset->author());
        } else if (name.length() > 0) {
            presetName = name;
        } else if (pPreset->filePath().length() > 0) {
            QFileInfo file(pPreset->filePath());
            presetName = file.baseName();
        }
    }
    return presetName;
}
QString DlgPrefController::presetName(const ControllerPresetPointer pPreset) const {
    if (pPreset) {
        auto name = pPreset->name();
        if (name.length() > 0)
            return name;
    }
    return tr("No Name");
}
QString DlgPrefController::presetDescription(const ControllerPresetPointer pPreset) const {
    if (pPreset) {
        QString description = pPreset->description();
        if (description.length() > 0)
            return description;
    }
    return tr("No Description");
}
QString DlgPrefController::presetAuthor(const ControllerPresetPointer pPreset) const {
    if (pPreset) {
        QString author = pPreset->author();
        if (author.length() > 0)
            return author;
    }
    return tr("No Author");
}
QString DlgPrefController::presetForumLink(const ControllerPresetPointer pPreset) const {
    QString url;
    if (pPreset) {
        QString link = pPreset->forumlink();
        if (link.length() > 0)
            url = "<a href=\"" + link + "\">Mixxx Forums</a>";
    }
    return url;
}
QString DlgPrefController::presetWikiLink(const ControllerPresetPointer pPreset) const {
    QString url;
    if (pPreset) {
        auto link = pPreset->wikilink();
        if (link.length() > 0)
            url = "<a href=\"" + link + "\">Mixxx Wiki</a>";
    }
    return url;
}
void DlgPrefController::slotDirty()
{
    m_bDirty = true;
}
QString nameForPreset(const PresetInfo& preset)
{
    auto name = preset.getName();
    if (name.length() == 0) {
        QFileInfo file(preset.getPath());
        name = file.baseName();
    }
    return name;
}

void DlgPrefController::enumeratePresets() {
    m_ui.comboBoxPreset->clear();
    // qDebug() << "Enumerating presets for controller" << m_pController->getName();
    // Insert a dummy "..." item at the top to try to make it less confusing.
    // (We don't want the first found file showing up as the default item when a
    // user has their controller plugged in)
    m_ui.comboBoxPreset->addItem("...");
    m_ui.comboBoxPreset->setInsertPolicy(QComboBox::InsertAlphabetically);
    // Ask the controller manager for a list of applicable presets
    auto pie =  m_pControllerManager->getMainThreadPresetEnumerator();
    auto presets = pie->getPresets(m_pController->presetExtension());
    PresetInfo match;
    for(const auto & preset: presets) {
        m_ui.comboBoxPreset->addItem(nameForPreset(preset), preset.getPath());
        if (m_pController->matchPreset(preset)) {
            match = preset;
        }
    }

    // Jump to matching device in list if it was found.
    if (match.isValid()) {
        auto index = m_ui.comboBoxPreset->findText(nameForPreset(match));
        if (index != -1)
            m_ui.comboBoxPreset->setCurrentIndex(index);
    }
}
void DlgPrefController::slotUpdate() {
    enumeratePresets();
    // Check if the controller is open.
    auto deviceOpen = m_pController->isOpen();
    // Check/uncheck the "Enabled" box
    m_ui.chkEnabledDevice->setChecked(deviceOpen);

    // If the controller is not mappable, disable the input and output mapping
    // sections and the learning wizard button.
    auto isMappable = m_pController->isMappable();
    m_ui.btnLearningWizard->setEnabled(isMappable);
    m_ui.inputMappingsTab->setEnabled(isMappable);
    m_ui.outputMappingsTab->setEnabled(isMappable);
}

void DlgPrefController::slotCancel() {
    if (m_pInputTableModel)
        m_pInputTableModel->cancel();
    if (m_pOutputTableModel)
        m_pOutputTableModel->cancel();
}

void DlgPrefController::slotApply() {
    if (m_bDirty) {
        // Apply the presets and load the resulting preset.
        if (m_pInputTableModel)
            m_pInputTableModel->apply();
        if (m_pOutputTableModel)
            m_pOutputTableModel->apply();
        // Load script info from the script table.
        m_pPreset->scripts.clear();
        for (auto i = 0; i < m_ui.m_pScriptsTableWidget->rowCount(); ++i) {
            auto scriptFile = m_ui.m_pScriptsTableWidget->item(i, 0)->text();
            // Skip empty rows.
            if (scriptFile.isEmpty())
                continue;
            auto scriptPrefix = m_ui.m_pScriptsTableWidget->item(i, 1)->text();
            auto builtin = m_ui.m_pScriptsTableWidget->item(i, 2)->checkState() == Qt::Checked;
            ControllerPreset::ScriptFileInfo info;
            info.name = scriptFile;
            info.functionPrefix = scriptPrefix;
            info.builtin = builtin;
            m_pPreset->scripts.append(info);
        }

        // Load the resulting preset (which has been mutated by the input/output
        // table models). The controller clones the preset so we aren't touching
        // the same preset.
        emit(loadPreset(m_pController, m_pPreset));
        //Select the "..." item again in the combobox.
        m_ui.comboBoxPreset->setCurrentIndex(0);
        auto ntEnabled = m_ui.chkEnabledDevice->isChecked();
        auto enabled = m_pController->isOpen();
        if (wantEnabled && !enabled) {
            enableDevice();
        } else if (!wantEnabled && enabled) {
            disableDevice();
        }

        m_bDirty = false;
    }
}

void DlgPrefController::slotLoadPreset(int chosenIndex) {
    if (chosenIndex == 0) {
        // User picked ...
        return;
    }
    const QString presetPath = m_ui.comboBoxPreset->itemData(chosenIndex).toString();
    // When loading the preset, we only want to load from the same dir as the
    // preset itself, otherwise when loading from the system-wide dir we'll
    // start the search in the user's dir find the existing script,
    // and do nothing.
    const QFileInfo presetFileInfo(presetPath);
    QList<QString> presetDirs;
    presetDirs.append(presetFileInfo.canonicalPath());
    auto pPreset = ControllerPresetFileHandler::loadPreset(presetPath, ControllerManager::getPresetPaths(m_pConfig));
    // Import the preset scripts to the user scripts folder.
    for (QList<ControllerPreset::ScriptFileInfo>::iterator it =
                 pPreset->scripts.begin(); it != pPreset->scripts.end(); ++it) {
        // No need to import builtin scripts.
        if (it->builtin) {
            continue;
        }

        auto scriptPath = ControllerManager::getAbsolutePath(it->name, presetDirs);
        QString importedScriptFileName;
        // If a conflict exists then importScript will provide a new filename to
        // use. If importing fails then load the preset anyway without the
        // import.
        if (m_pControllerManager->importScript(scriptPath, &importedScriptFileName))
            it->name = importedScriptFileName;
    }
    // TODO(rryan): We really should not load the preset here. We should load it
    // into the preferences GUI and then load it to the actual controller once
    // the user hits apply.
    emit(loadPreset(m_pController, pPreset));
    slotDirty();
}

void DlgPrefController::initTableView(QTableView* pTable) {
    // Enable selection by rows and extended selection (ctrl/shift click)
    pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

    pTable->setWordWrap(false);
    pTable->setShowGrid(false);
    pTable->setCornerButtonEnabled(false);
    pTable->setSortingEnabled(true);

    //Work around a Qt bug that lets you make your columns so wide you
    //can't reach the divider to make them small again.
    pTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    pTable->verticalHeader()->hide();
    pTable->verticalHeader()->setDefaultSectionSize(20);
    pTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    pTable->setAlternatingRowColors(true);
}

void DlgPrefController::slotPresetLoaded(ControllerPresetPointer preset) {
    m_ui.labelLoadedPreset->setText(presetName(preset));
    m_ui.labelLoadedPresetDescription->setText(presetDescription(preset));
    m_ui.labelLoadedPresetAuthor->setText(presetAuthor(preset));
    QStringList supportLinks;

    auto forumLink = presetForumLink(preset);
    if (forumLink.length() > 0)
        supportLinks << forumLink;
    auto wikiLink = presetWikiLink(preset);
    if (wikiLink.length() > 0)
        supportLinks << wikiLink;
    // There is always at least one support link.
    // TODO(rryan): This is a horrible general support link for MIDI!
    QString troubleShooting = QString(
        "<a href=\"http://mixxx.org/wiki/doku.php/midi_scripting\">%1</a>")
            .arg(tr("Troubleshooting"));
    supportLinks << troubleShooting;
    auto support = supportLinks.join("&nbsp;");
    m_ui.labelLoadedPresetSupportLinks->setText(support);

    // We mutate this preset so keep a reference to it while we are using it.
    // TODO(rryan): Clone it? Technically a waste since nothing else uses this
    // copy but if someone did they might not expect it to change.
    m_pPreset = preset;
    auto pInputModel =new ControllerInputMappingTableModel(this);
    // If the model reports changes, mark ourselves as dirty.
    connect(pInputModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)),this, SLOT(slotDirty()));
    connect(pInputModel, SIGNAL(rowsInserted(QModelIndex, int, int)),this, SLOT(slotDirty()));
    connect(pInputModel, SIGNAL(rowsRemoved(QModelIndex, int, int)),this, SLOT(slotDirty()));
    pInputModel->setPreset(preset);

    auto pInputProxyModel = new QSortFilterProxyModel(this);
    pInputProxyModel->setSortRole(Qt::UserRole);
    pInputProxyModel->setSourceModel(pInputModel);
    m_ui.m_pInputMappingTableView->setModel(pInputProxyModel);

    for (int i = 0; i < pInputModel->columnCount(); ++i) {
        QAbstractItemDelegate* pDelegate = pInputModel->delegateForColumn(
            i, m_ui.m_pInputMappingTableView);
        if (pDelegate != nullptr) {
            qDebug() << "Setting input delegate for column" << i << pDelegate;
            m_ui.m_pInputMappingTableView->setItemDelegateForColumn(i, pDelegate);
        }
    }

    // Now that we have set the new model our old model can be deleted.
    delete m_pInputProxyModel;
    m_pInputProxyModel = pInputProxyModel;
    delete m_pInputTableModel;
    m_pInputTableModel = pInputModel;

    auto pOutputModel =new ControllerOutputMappingTableModel(this);
    pOutputModel->setPreset(preset);

    auto pOutputProxyModel = new QSortFilterProxyModel(this);
    pOutputProxyModel->setSortRole(Qt::UserRole);
    pOutputProxyModel->setSourceModel(pOutputModel);
    m_ui.m_pOutputMappingTableView->setModel(pOutputProxyModel);

    for (int i = 0; i < pOutputModel->columnCount(); ++i) {
        auto pDelegate = pOutputModel->delegateForColumn(i, m_ui.m_pOutputMappingTableView);
        if (pDelegate != nullptr) {
            qDebug() << "Setting output delegate for column" << i << pDelegate;
            m_ui.m_pOutputMappingTableView->setItemDelegateForColumn(i, pDelegate);
        }
    }
    // Now that we have set the new model our old model can be deleted.
    delete m_pOutputProxyModel;
    m_pOutputProxyModel = pOutputProxyModel;
    delete m_pOutputTableModel;
    m_pOutputTableModel = pOutputModel;

    // Populate the script tab with the scripts this preset uses.
    m_ui.m_pScriptsTableWidget->setRowCount(preset->scripts.length());
    m_ui.m_pScriptsTableWidget->setColumnCount(3);
    m_ui.m_pScriptsTableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Filename")));
    m_ui.m_pScriptsTableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Function Prefix")));
    m_ui.m_pScriptsTableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Built-in")));
    m_ui.m_pScriptsTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    for (int i = 0; i < preset->scripts.length(); ++i) {
        const auto& script = preset->scripts.at(i);

        auto pScriptName = new QTableWidgetItem(script.name);
        m_ui.m_pScriptsTableWidget->setItem(i, 0, pScriptName);
        pScriptName->setFlags(pScriptName->flags() & ~Qt::ItemIsEditable);

        auto pScriptPrefix = new QTableWidgetItem(script.functionPrefix);
        m_ui.m_pScriptsTableWidget->setItem(i, 1, pScriptPrefix);

        // If the script is built-in don't allow editing of the prefix.
        if (script.builtin)
            pScriptPrefix->setFlags(pScriptPrefix->flags() & ~Qt::ItemIsEditable);
        auto pScriptBuiltin = new QTableWidgetItem();
        pScriptBuiltin->setCheckState(script.builtin ? Qt::Checked : Qt::Unchecked);
        pScriptBuiltin->setFlags(pScriptBuiltin->flags() & ~(Qt::ItemIsEnabled |
                                                             Qt::ItemIsEditable |
                                                             Qt::ItemIsUserCheckable));
        m_ui.m_pScriptsTableWidget->setItem(i, 2, pScriptBuiltin);
    }
}
void DlgPrefController::slotEnableDevice(bool enable)
{
    slotDirty();
    emit(controllerEnabled(this, enable));
}

void DlgPrefController::enableDevice() {
    emit(openController(m_pController));
    //TODO: Should probably check if open() actually succeeded.
}
void DlgPrefController::disableDevice()
{
    emit(closeController(m_pController));
}
void DlgPrefController::addInputMapping()
{
    if (m_pInputTableModel) {
        m_pInputTableModel->addEmptyMapping();
        // Ensure the added row is visible.
        auto left = m_pInputProxyModel->mapFromSource(
            m_pInputTableModel->index(m_pInputTableModel->rowCount() - 1, 0));
        auto right = m_pInputProxyModel->mapFromSource(
            m_pInputTableModel->index(m_pInputTableModel->rowCount() - 1,
                                       m_pInputTableModel->columnCount() - 1));
        m_ui.m_pInputMappingTableView->selectionModel()->select(
            QItemSelection(left, right), QItemSelectionModel::Clear | QItemSelectionModel::Select);
        m_ui.m_pInputMappingTableView->scrollTo(left);
        slotDirty();
    }
}

void DlgPrefController::removeInputMappings() {
    if (m_pInputProxyModel) {
        auto selection = m_pInputProxyModel->mapSelectionToSource(
            m_ui.m_pInputMappingTableView->selectionModel()->selection());
        auto selectedIndices = selection.indexes();
        if (selectedIndices.size() > 0 && m_pInputTableModel) {
            m_pInputTableModel->removeMappings(selectedIndices);
            slotDirty();
        }
    }
}

void DlgPrefController::clearAllInputMappings() {
    if (QMessageBox::warning(
            this, tr("Clear Input Mappings"),
            tr("Are you sure you want to clear all input mappings?"),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }
    if (m_pInputTableModel) {
        m_pInputTableModel->clear();
        slotDirty();
    }
}

void DlgPrefController::addOutputMapping() {
    if (m_pOutputTableModel) {
        m_pOutputTableModel->addEmptyMapping();
        // Ensure the added row is visible.
        auto left = m_pOutputProxyModel->mapFromSource(
            m_pOutputTableModel->index(m_pOutputTableModel->rowCount() - 1, 0));
        auto right = m_pOutputProxyModel->mapFromSource(
            m_pOutputTableModel->index(m_pOutputTableModel->rowCount() - 1,
                                       m_pOutputTableModel->columnCount() - 1));
        m_ui.m_pOutputMappingTableView->selectionModel()->select(
            QItemSelection(left, right), QItemSelectionModel::Clear | QItemSelectionModel::Select);
        m_ui.m_pOutputMappingTableView->scrollTo(left);
        slotDirty();
    }
}
void DlgPrefController::removeOutputMappings() {
    if (m_pOutputProxyModel) {
        auto selection = m_pOutputProxyModel->mapSelectionToSource(
            m_ui.m_pOutputMappingTableView->selectionModel()->selection());
        auto selectedIndices = selection.indexes();
        if (selectedIndices.size() > 0 && m_pOutputTableModel) {
            m_pOutputTableModel->removeMappings(selectedIndices);
            slotDirty();
        }
    }
}

void DlgPrefController::clearAllOutputMappings() {
    if (QMessageBox::warning(
            this, tr("Clear Output Mappings"),
            tr("Are you sure you want to clear all output mappings?"),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }
    if (m_pOutputTableModel) {
        m_pOutputTableModel->clear();
        slotDirty();
    }
}
void DlgPrefController::addScript() {
    auto scriptFile = QFileDialog::getOpenFileName(
        this, tr("Add Script"),
        QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation),
        tr("Controller Script Files (*.js)"));
    if (scriptFile.isNull())
        return;
    QString importedScriptFileName;
    if (!m_pControllerManager->importScript(scriptFile, &importedScriptFileName)) {
        QMessageBox::warning(this, tr("Add Script"),
                             tr("Could not add script file: '%s'"));
        return;
    }
    // Don't allow duplicate entries in the table. This could happen if the file
    // is missing (and the user added it to try and fix this) or if the file is
    // already in the presets directory with an identical checksum.
    for (int i = 0; i < m_ui.m_pScriptsTableWidget->rowCount(); ++i) {
        if (m_ui.m_pScriptsTableWidget->item(i, 0)->text() == importedScriptFileName) {
            return;
        }
    }
    auto newRow = m_ui.m_pScriptsTableWidget->rowCount();
    m_ui.m_pScriptsTableWidget->setRowCount(newRow + 1);
    auto pScriptName = new QTableWidgetItem(importedScriptFileName);
    m_ui.m_pScriptsTableWidget->setItem(newRow, 0, pScriptName);
    pScriptName->setFlags(pScriptName->flags() & ~Qt::ItemIsEditable);
    auto pScriptPrefix = new QTableWidgetItem("");
    m_ui.m_pScriptsTableWidget->setItem(newRow, 1, pScriptPrefix);
    auto pScriptBuiltin = new QTableWidgetItem();
    pScriptBuiltin->setCheckState(Qt::Unchecked);
    pScriptBuiltin->setFlags(pScriptBuiltin->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
    m_ui.m_pScriptsTableWidget->setItem(newRow, 2, pScriptBuiltin);
    slotDirty();
}
void DlgPrefController::removeScript() {
    auto selectedIndices = m_ui.m_pScriptsTableWidget->selectionModel()->selection().indexes();
    if (selectedIndices.isEmpty())
        return;
    QList<int> selectedRows;
    for(auto index: selectedIndices)
        selectedRows.append(index.row());
    qSort(selectedRows);
    auto lastRow = -1;
    while (!selectedRows.empty()) {
        auto row = selectedRows.takeLast();
        if (row == lastRow)
            continue;
        // You can't remove a builtin script.
        auto pItem = m_ui.m_pScriptsTableWidget->item(row, 2);
        if (pItem->checkState() == Qt::Checked)
            continue;
        lastRow = row;
        m_ui.m_pScriptsTableWidget->removeRow(row);
    }
    slotDirty();
}

void DlgPrefController::openScript() {
    QModelIndexList selectedIndices = m_ui.m_pScriptsTableWidget->selectionModel()
            ->selection().indexes();
    if (selectedIndices.isEmpty()) {
         QMessageBox::information(
                    this,
                    Version::applicationName(),
                    tr("Please select a script from the list to open."),
                    QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    QSet<int> selectedRows;
    for(auto index: selectedIndices)
        selectedRows.insert(index.row());
    auto scriptPaths = ControllerManager::getPresetPaths(m_pConfig);
    for(auto row: selectedRows) {
        auto scriptName = m_ui.m_pScriptsTableWidget->item(row, 0)->text();
        auto scriptPath = ControllerManager::getAbsolutePath(scriptName, scriptPaths);
        if (!scriptPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(scriptPath));
        }
    }
}
