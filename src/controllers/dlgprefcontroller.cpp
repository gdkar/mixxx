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
//#include "controllers/controllerlearningeventfilter.h"
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
          m_pInputProxyModel(nullptr),
          m_pOutputProxyModel(nullptr),
          m_bDirty(false)
{
    m_ui.setupUi(this);

    initTableView(m_ui.m_pInputMappingTableView);
    initTableView(m_ui.m_pOutputMappingTableView);
    initTableView(m_ui.m_pScriptsTableWidget);

    // TODO(rryan): Eh, this really isn't thread safe but it's the way it's been
    // since 1.11.0. We shouldn't be calling Controller methods because it lives
    // in a different thread. Booleans (like isOpen()) are fine but a complex
    // object like a preset involves QHash's and other data structures that
    // really don't like concurrent access.

    m_ui.labelDeviceName->setText(m_pController->getDeviceName());
    QString category = m_pController->getDeviceCategory();
    if (!category.isEmpty()) {
        m_ui.labelDeviceCategory->setText(category);
    } else {
        m_ui.labelDeviceCategory->hide();
    }

    // When the user picks a preset, load it.
//    connect(m_ui.comboBoxPreset, SIGNAL(activated(int)),
//            this, SLOT(slotLoadPreset(int)));

    // When the user toggles the Enabled checkbox, toggle.
    connect(m_ui.chkEnabledDevice, SIGNAL(clicked(bool)),
            this, SLOT(onEnableDevice(bool)));

    // Connect our signals to controller manager.
    connect(this, SIGNAL(openController(Controller*)),
            m_pControllerManager, SLOT(openController(Controller*)));
    connect(this, SIGNAL(closeController(Controller*)),
            m_pControllerManager, SLOT(closeController(Controller*)));
    // Scripts
    connect(m_ui.m_pScriptsTableWidget, SIGNAL(cellChanged(int, int)),
            this, SLOT(onDirty()));
    connect(m_ui.btnAddScript, SIGNAL(clicked()),
            this, SLOT(addScript()));
    connect(m_ui.btnRemoveScript, SIGNAL(clicked()),
            this, SLOT(removeScript()));
    connect(m_ui.btnOpenScript, SIGNAL(clicked()),
            this, SLOT(openScript()));
}

DlgPrefController::~DlgPrefController() = default;

void DlgPrefController::showLearningWizard()
{
    // If the user has checked the "Enabled" checkbox but they haven't hit OK to
    // apply it yet, prompt them to apply the settings before we open the
    // learning dialog. If we don't apply the settings first and open the
    // device, the dialog won't react to controller messages.
/*    if (m_ui.chkEnabledDevice->isChecked() && !m_pController->isOpen()) {
        auto result = QMessageBox::question(
            this,
            tr("Apply device settings?"),
            tr("Your settings must be applied before starting the learning wizard.\n"
               "Apply settings and continue?"),
            QMessageBox::Ok | QMessageBox::Cancel,  // Buttons to be displayed
            QMessageBox::Ok);  // Default button
        // Stop if the user has not pressed the Ok button,
        // which could be the Cancel or the Close Button.
        if (result != QMessageBox::Ok) {
            return;
        }
    }
    onApply();
    // After this point we consider the mapping wizard as dirtying the preset.
    onDirty();
    // Note that DlgControllerLearning is set to delete itself on close using
    // the Qt::WA_DeleteOnClose attribute (so this "new" doesn't leak memory)
   m_pDlgControllerLearning = new DlgControllerLearning(this, m_pController);
    m_pDlgControllerLearning->show();
    auto  pControllerLearning = m_pControllerManager->getControllerLearningEventFilter();
    pControllerLearning->startListening();
    connect(pControllerLearning, SIGNAL(controlClicked(ControlObject*)),
            m_pDlgControllerLearning, SLOT(controlClicked(ControlObject*)));
    connect(m_pDlgControllerLearning, SIGNAL(listenForClicks()),
            pControllerLearning, SLOT(startListening()));
    connect(m_pDlgControllerLearning, SIGNAL(stopListeningForClicks()),
            pControllerLearning, SLOT(stopListening()));*/
//    connect(m_pDlgControllerLearning, SIGNAL(stopLearning()),this, SLOT(show()));
//    connect(m_pDlgControllerLearning, SIGNAL(stopLearning()),this, SIGNAL(mappingEnded()));
}
void DlgPrefController::onDirty()
{
    m_bDirty = true;
}
void DlgPrefController::onUpdate()
{
    // Check if the controller is open.
    auto deviceOpen = m_pController->isOpen();
    // Check/uncheck the "Enabled" box
    m_ui.chkEnabledDevice->setChecked(deviceOpen);
    // If the controller is not mappable, disable the input and output mapping
    // sections and the learning wizard button.
}
void DlgPrefController::onCancel()
{
/*    if (m_pInputTableModel)
        m_pInputTableModel->cancel();

    if (m_pOutputTableModel)
        m_pOutputTableModel->cancel();*/
}
void DlgPrefController::onApply()
{
    if (m_bDirty) {
        // Apply the presets and load the resulting preset.
/*        if (m_pInputTableModel)
            m_pInputTableModel->apply();

        if (m_pOutputTableModel)
            m_pOutputTableModel->apply();*/

        // Load script info from the script table.
        for (auto i = 0; i < m_ui.m_pScriptsTableWidget->rowCount(); ++i) {
            auto scriptFile = m_ui.m_pScriptsTableWidget->item(i, 0)->text();
            // Skip empty rows.
            if (scriptFile.isEmpty())
                continue;
            auto scriptPrefix = m_ui.m_pScriptsTableWidget->item(i, 1)->text();
            auto builtin = m_ui.m_pScriptsTableWidget->item(i, 2)->checkState() == Qt::Checked;
        }
        // Load the resulting preset (which has been mutated by the input/output
        // table models). The controller clones the preset so we aren't touching
        // the same preset.
        //Select the "..." item again in the combobox.
        m_ui.comboBoxPreset->setCurrentIndex(0);
        auto wantEnabled = m_ui.chkEnabledDevice->isChecked();
        auto enabled = m_pController->isOpen();
        if (wantEnabled && !enabled) {
            enableDevice();
        } else if (!wantEnabled && enabled) {
            disableDevice();
        }
        m_bDirty = false;
    }
}
void DlgPrefController::initTableView(QTableView* pTable)
{
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
void DlgPrefController::onEnableDevice(bool enable)
{
    onDirty();
    // Set tree item text to normal/bold.
    emit(controllerEnabled(this, enable));
}
void DlgPrefController::enableDevice()
{
    emit(openController(m_pController));
    //TODO: Should probably check if open() actually succeeded.
}
void DlgPrefController::disableDevice()
{
    emit(closeController(m_pController));
    //TODO: Should probably check if close() actually succeeded.
}
void DlgPrefController::addScript()
{
    auto scriptFile = QFileDialog::getOpenFileName(
        this, tr("Add Script"),
        QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation),
        tr("Controller Script Files (*.js)"));
    if (scriptFile.isNull()) {
        return;
    }
    auto importedScriptFileName = QString{};;
/*    if (!m_pControllerManager->importScript(scriptFile, &importedScriptFileName)) {
        QMessageBox::warning(this, tr("Add Script"),
                             tr("Could not add script file: '%s'"));
        return;
    }*/
    // Don't allow duplicate entries in the table. This could happen if the file
    // is missing (and the user added it to try and fix this) or if the file is
    // already in the presets directory with an identical checksum.
    for (auto i = 0; i < m_ui.m_pScriptsTableWidget->rowCount(); ++i) {
        if (m_ui.m_pScriptsTableWidget->item(i, 0)->text() == importedScriptFileName)
            return;
    }

    auto newRow = m_ui.m_pScriptsTableWidget->rowCount();
    m_ui.m_pScriptsTableWidget->setRowCount(newRow + 1);
    auto  pScriptName = new QTableWidgetItem(importedScriptFileName);
    m_ui.m_pScriptsTableWidget->setItem(newRow, 0, pScriptName);
    pScriptName->setFlags(pScriptName->flags() & ~Qt::ItemIsEditable);

    auto pScriptPrefix = new QTableWidgetItem("");
    m_ui.m_pScriptsTableWidget->setItem(newRow, 1, pScriptPrefix);

    auto pScriptBuiltin = new QTableWidgetItem();
    pScriptBuiltin->setCheckState(Qt::Unchecked);
    pScriptBuiltin->setFlags(pScriptBuiltin->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
    m_ui.m_pScriptsTableWidget->setItem(newRow, 2, pScriptBuiltin);
    onDirty();
}

void DlgPrefController::removeScript()
{
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
    onDirty();
}

void DlgPrefController::openScript()
{
    auto selectedIndices = m_ui.m_pScriptsTableWidget->selectionModel()->selection().indexes();
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

    auto scriptPaths = ControllerManager::getScriptPaths(m_pConfig);
    for(auto row: selectedRows) {
        auto scriptName = m_ui.m_pScriptsTableWidget->item(row, 0)->text();
        auto scriptPath = ControllerManager::getAbsolutePath(scriptName, scriptPaths);
        if (!scriptPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(scriptPath));
        }
    }
}
