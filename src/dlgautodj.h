#ifndef DLGAUTODJ_H
#define DLGAUTODJ_H

#include <QWidget>
#include <QString>

#include "ui_dlgautodj.h"
#include "configobject.h"
#include "library/libraryview.h"
#include "library/library.h"
#include "library/trackcollection.h"
#include "library/autodj/autodjprocessor.h"

class PlaylistTableModel;
class WTrackTableView;

class DlgAutoDJ : public QWidget, public Ui::DlgAutoDJ, public LibraryView {
    Q_OBJECT
  public:
    DlgAutoDJ(QWidget* parent, ConfigObject<ConfigValue>* pConfig,
              Library* pLibrary,
              AutoDJProcessor* pProcessor, TrackCollection* pTrackCollection,
              QObject* pKeyboard);
    virtual ~DlgAutoDJ();

    void onShow();
    void onSearch(const QString& text);
    void loadSelectedTrack();
    void loadSelectedTrackToGroup(QString group, bool play);
    void moveSelection(int delta);

  public slots:
    void shufflePlaylistButton(bool buttonChecked);
    void skipNextButton(bool buttonChecked);
    void fadeNowButton(bool buttonChecked);
    void toggleAutoDJButton(bool enable);
    void transitionTimeChanged(int time);
    void transitionSliderChanged(int value);
    void autoDJStateChanged(int state);
    void setTrackTableFont(const QFont& font);
    void setTrackTableRowHeight(int rowHeight);

  signals:
    void addRandomButton(bool buttonChecked);
    void loadTrack(TrackPointer tio);
    void loadTrackToPlayer(TrackPointer tio, QString group, bool);
    void trackSelected(TrackPointer pTrack);

  private:
    AutoDJProcessor* m_pAutoDJProcessor = nullptr;
    WTrackTableView* m_pTrackTableView = nullptr;
    PlaylistTableModel* m_pAutoDJTableModel = nullptr;
};

#endif //DLGAUTODJ_H
