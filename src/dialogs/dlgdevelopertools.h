#ifndef DLGDEVELOPERTOOLS_H
#define DLGDEVELOPERTOOLS_H

#include <QDialog>
#include <QSortFilterProxyModel>
#include <QTimerEvent>

#include "dialogs/ui_dlgdevelopertoolsdlg.h"
#include "configobject.h"
#include "controlobject.h"
#include "control/controlmodel.h"
#include "util/statmodel.h"

class DlgDeveloperTools : public QDialog, public Ui::DlgDeveloperTools {
    Q_OBJECT
  public:
    DlgDeveloperTools(QWidget* pParent, ConfigObject<ConfigValue>* pConfig);
    virtual ~DlgDeveloperTools();

  protected:
    void timerEvent(QTimerEvent* pTimerEvent);

  private slots:
    void onControlSearch(const QString& search);
    void onControlSearchClear();
    void onLogSearch();
    void onControlDump();

  private:
    ControlModel m_controlModel;
    QSortFilterProxyModel m_controlProxyModel;

    StatModel m_statModel;
    QSortFilterProxyModel m_statProxyModel;

    QFile m_logFile;
    QTextCursor m_logCursor;

};

#endif /* DLGDEVELOPERTOOLS_H */
