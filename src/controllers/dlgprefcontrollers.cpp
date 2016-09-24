#include <QDesktopServices>

#include "controllers/dlgprefcontrollers.h"
#include "controllers/controller.h"
#include "preferences/dialog/dlgpreferences.h"
#include "controllers/controllermanager.h"
#include "controllers/dlgprefcontroller.h"
#include "controllers/defs_controllers.h"

DlgPrefControllers::DlgPrefControllers(DlgPreferences* pPreferences,
                                       UserSettingsPointer pConfig,
                                       ControllerManager* pControllerManager,
                                       QTreeWidgetItem* pControllerTreeItem)
        : DlgPreferencePage(pPreferences),
          m_pDlgPreferences(pPreferences),
          m_pConfig(pConfig),
          m_pControllerManager(pControllerManager),
          m_pControllerTreeItem(pControllerTreeItem)
{
    setupUi(this);
    setupControllerWidgets();
    connect(&m_buttonMapper, qOverload<const QString&>(&QSignalMapper::mapped),
            this, &DlgPrefControllers::onOpenLocalFile);

//    connect(btnOpenUserPresets, SIGNAL(clicked()),
//            &m_buttonMapper, SLOT(map()));

//    m_buttonMapper.setMapping(btnOpenUserPresets, userPresetsPath(m_pConfig));

    // Connections
    connect(m_pControllerManager, &ControllerManager::devicesChanged,
            this, &DlgPrefControllers::rescanControllers);
}

DlgPrefControllers::~DlgPrefControllers()
{
    destroyControllerWidgets();
}
void DlgPrefControllers::onOpenLocalFile(const QString& file)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(file));
}
bool DlgPrefControllers::handleTreeItemClick(QTreeWidgetItem* clickedItem)
{
    auto controllerIndex = m_controllerTreeItems.indexOf(clickedItem);
    if (controllerIndex >= 0) {
        auto controllerWidget = m_controllerWindows.value(controllerIndex);
        if (controllerWidget)
            m_pDlgPreferences->switchToPage(controllerWidget);
        return true;
    } else if (clickedItem == m_pControllerTreeItem) {
        // Switch to the root page and expand the controllers tree item.
        m_pDlgPreferences->expandTreeItem(clickedItem);
        m_pDlgPreferences->switchToPage(this);
        return true;
    }
    return false;
}
void DlgPrefControllers::rescanControllers()
{
    destroyControllerWidgets();
    setupControllerWidgets();
}
void DlgPrefControllers::destroyControllerWidgets()
{
    while (!m_controllerWindows.isEmpty()) {
        auto controllerDlg = m_controllerWindows.takeLast();
        m_pDlgPreferences->removePageWidget(controllerDlg);
        delete controllerDlg;
    }

    m_controllerTreeItems.clear();
    while(m_pControllerTreeItem->childCount() > 0) {
        auto controllerWindowLink = m_pControllerTreeItem->takeChild(0);
        delete controllerWindowLink;
    }
}
void DlgPrefControllers::setupControllerWidgets()
{
    // For each controller, create a dialog and put a little link to it in the
    // treepane on the left.
    auto controllerList = m_pControllerManager->getControllerList(false, true);
    qSort(controllerList.begin(), controllerList.end(), controllerCompare);

    for(auto && pController: controllerList) {
        auto controllerDlg = new DlgPrefController(this, pController, m_pControllerManager, m_pConfig);
        connect(this,&DlgPrefControllers::onUpdate,controllerDlg,&DlgPrefController::onUpdate);
        connect(this,&DlgPrefControllers::onApply,controllerDlg,&DlgPrefController::onApply);
        connect(this,&DlgPrefControllers::onCancel,controllerDlg,&DlgPrefController::onCancel);
//        connect(controllerDlg, &DlgPrefController::mappingStarted,m_pDlgPreferences, &DlgPreferences::hide);
//        connect(controllerDlg, &DlgPrefController::mappingEnded,m_pDlgPreferences, &DlgPreferences::show);

        m_controllerWindows.append(controllerDlg);
        m_pDlgPreferences->addPageWidget(controllerDlg);

        connect(controllerDlg, &DlgPrefController::controllerEnabled,
                this, &DlgPrefControllers::onHighlightDevice);

        auto controllerWindowLink = new QTreeWidgetItem(QTreeWidgetItem::Type);
        controllerWindowLink->setIcon(0, QIcon(":/images/preferences/ic_preferences_controllers.png"));
        auto curDeviceName = pController->getDeviceName();
        controllerWindowLink->setText(0, curDeviceName);
        controllerWindowLink->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        controllerWindowLink->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_pControllerTreeItem->addChild(controllerWindowLink);
        m_controllerTreeItems.append(controllerWindowLink);

        // Set the font correctly
        auto temp = controllerWindowLink->font(0);
        temp.setBold(pController->isOpen());
        controllerWindowLink->setFont(0, temp);
    }

    // If no controllers are available, show the "No controllers available"
    // message.
    txtNoControllersAvailable->setVisible(controllerList.empty());
}

void DlgPrefControllers::onHighlightDevice(DlgPrefController* dialog, bool enabled)
{
    auto dialogIndex = m_controllerWindows.indexOf(dialog);
    if (dialogIndex < 0)
        return;
    if(auto controllerWindowLink = m_controllerTreeItems.at(dialogIndex)){
        auto temp = controllerWindowLink->font(0);
        temp.setBold(enabled);
        controllerWindowLink->setFont(0,temp);
    }
}
