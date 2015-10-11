#ifndef DLGMISSING_H
#define DLGMISSING_H

#include "preferences/ui_dlgmissing.h"
#include "configobject.h"
#include "library/library.h"
#include "library/libraryview.h"
#include "library/trackcollection.h"

class WTrackTableView;
class MissingTableModel;

class DlgMissing : public QWidget, public Ui::DlgMissing, public LibraryView {
    Q_OBJECT
  public:
    DlgMissing(QWidget* parent, ConfigObject<ConfigValue>* pConfig,
               Library* pLibrary, TrackCollection* pTrackCollection,
               QObject* pKeyboard);
    virtual ~DlgMissing();

    void onShow();
    void onSearch(const QString& text);

  public slots:
    void clicked();
    void selectAll();
    void selectionChanged(const QItemSelection&, const QItemSelection&);
    void setTrackTableFont(const QFont& font);
    void setTrackTableRowHeight(int rowHeight);

  signals:
    void trackSelected(TrackPointer pTrack);

  private:
    void activateButtons(bool enable);
    WTrackTableView* m_pTrackTableView;
    MissingTableModel* m_pMissingTableModel;
};

#endif //DLGMISSING_H