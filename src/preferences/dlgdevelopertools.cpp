#include <QDateTime>

#include "dlgdevelopertools.h"

#include "control/control.h"
#include "util/cmdlineargs.h"
#include "util/statsmanager.h"


DlgDeveloperTools::DlgDeveloperTools(QWidget* pParent,
                                     ConfigObject<ConfigValue>* pConfig)
        : QDialog(pParent) {
    Q_UNUSED(pConfig);
    setupUi(this);

    auto controlsList = ControlDoublePrivate::getControls();
    auto controlAliases =ControlDoublePrivate::getControlAliases();
    for(auto pControl : controlsList){
        if (pControl) {
          m_controlModel.addControl(pControl->getKey(), pControl->name(),pControl->description());
          auto aliasKey = controlAliases[pControl->getKey()];
          if (!aliasKey.isNull()) {
              m_controlModel.addControl(aliasKey, pControl->name(),"Alias for " + pControl->getKey().group + pControl->getKey().item);
          }
        }
    }
    m_controlProxyModel.setSourceModel(&m_controlModel);
    m_controlProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_controlProxyModel.setFilterKeyColumn(ControlModel::CONTROL_COLUMN_FILTER);
    controlsTable->setModel(&m_controlProxyModel);
    controlsTable->hideColumn(ControlModel::CONTROL_COLUMN_TITLE);
    controlsTable->hideColumn(ControlModel::CONTROL_COLUMN_DESCRIPTION);
    controlsTable->hideColumn(ControlModel::CONTROL_COLUMN_FILTER);
    auto  pManager = StatsManager::instance();
    if (pManager) {
        connect(pManager, SIGNAL(statUpdated(const Stat&)),&m_statModel, SLOT(statUpdated(const Stat&)));
        pManager->emitAllStats();
    }
    m_statProxyModel.setSourceModel(&m_statModel);
    statsTable->setModel(&m_statProxyModel);
    auto logFileName = QDir(CmdlineArgs::Instance().getSettingsPath()).filePath("mixxx.log");
    m_logFile.setFileName(logFileName);
    if (!m_logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ERROR: Could not open log file";
    }
    // Connect search box signals to the library
    connect(controlSearch, SIGNAL(search(const QString&)),this, SLOT(slotControlSearch(const QString&)));
    connect(controlSearch, SIGNAL(searchCleared()),this, SLOT(slotControlSearchClear()));
    connect(controlDump, SIGNAL(clicked()),this, SLOT(slotControlDump()));
    // Set up the log search box
    connect(logSearch, SIGNAL(returnPressed()),this, SLOT(slotLogSearch()));
    connect(logSearchButton, SIGNAL(clicked()),this, SLOT(slotLogSearch()));
    m_logCursor = logTextView->textCursor();
    // Update at 2FPS.
    startTimer(500);
    // Delete this dialog when its closed. We don't want any persistence.
    setAttribute(Qt::WA_DeleteOnClose);
}
DlgDeveloperTools::~DlgDeveloperTools() = default;

void DlgDeveloperTools::timerEvent(QTimerEvent* pEvent) {
    Q_UNUSED(pEvent);
    if (m_logFile.isOpen()) {
        QStringList newLines;
        while (true) {
            auto line = m_logFile.readLine();
            if (line.isEmpty()) break;
            newLines.append(QString::fromLocal8Bit(line));
        }
        if (!newLines.isEmpty()) logTextView->append(newLines.join(""));
    }
    // To save on CPU, only update the models when they are visible.
    if (toolTabWidget->currentWidget() == controlsTab)controlsTable->update();
    else if (toolTabWidget->currentWidget() == statsTab) {
        if(auto  pManager = StatsManager::instance()) pManager->updateStats();
    }
}
void DlgDeveloperTools::slotControlSearch(const QString& search) {
    m_controlProxyModel.setFilterFixedString(search);
}
void DlgDeveloperTools::slotControlSearchClear() {
    m_controlProxyModel.setFilterFixedString(QString());
}
void DlgDeveloperTools::slotControlDump() {
    auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh'h'mm'm'ss's'");
    auto dumpFileName = CmdlineArgs::Instance().getSettingsPath() +"/co_dump_" + timestamp + ".csv";
    QFile dumpFile;
    // Note: QFile is closed if it falls out of scope
    dumpFile.setFileName(dumpFileName);
    if (!dumpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "open" << dumpFileName << "failed";
        return;
    }

    auto controlsList = ControlDoublePrivate::getControls();
    for (auto pControl : controlsList)
    {
        if (pControl) {
            QString line = pControl->getKey().group + "," +
                           pControl->getKey().item + "," +
                           QString::number(pControl->get()) + "\n";
            dumpFile.write(line.toLocal8Bit());
        }
    }
}
void DlgDeveloperTools::slotLogSearch() {
    QString textToFind = logSearch->text();
    m_logCursor = logTextView->document()->find(textToFind, m_logCursor);
    logTextView->setTextCursor(m_logCursor);
}
