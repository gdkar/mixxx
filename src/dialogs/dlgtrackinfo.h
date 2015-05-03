#ifndef DLGTRACKINFO_H
#define DLGTRACKINFO_H

#include <QDialog>
#include <QMutex>
#include <QHash>
#include <QList>
#include <QScopedPointer>

#include "dialogs/ui_dlgtrackinfo.h"
#include "trackinfoobject.h"
#include "dialogs/dlgtagfetcher.h"
#include "library/coverart.h"
#include "util/tapfilter.h"
#include "util/types.h"
#include "widget/wcoverartlabel.h"
#include "widget/wcoverartmenu.h"

class Cue;

class DlgTrackInfo : public QDialog, public Ui::DlgTrackInfo {
    Q_OBJECT
  public:
    DlgTrackInfo(QWidget* parent, DlgTagFetcher& DlgTagFetcher);
    virtual ~DlgTrackInfo();

  public slots:
    // Not thread safe. Only invoke via AutoConnection or QueuedConnection, not
    // directly!
    void loadTrack(TrackPointer pTrack);

  signals:
    void next();
    void previous();

  private slots:
    void onNext();
    void onPrev();
    void OK();
    void apply();
    void cancel();
    void trackUpdated();
    void fetchTag();

    void cueActivate();
    void cueDelete();

    void onBpmDouble();
    void onBpmHalve();
    void onBpmTwoThirds();
    void onBpmThreeFourth();
    void onBpmTap(double averageLength, int numSamples);

    void reloadTrackMetadata();
    void updateTrackMetadata();
    void onOpenInFileBrowser();

    void onCoverFound(const QObject* pRequestor, int requestReference,
                        const CoverInfo& info, QPixmap pixmap, bool fromCache);
    void onCoverArtSelected(const CoverArt& art);
    void onReloadCoverArt();

  private:
    void populateFields(TrackPointer pTrack);
    void populateCues(TrackPointer pTrack);
    void saveTrack();
    void unloadTrack(bool save);
    void clear();
    void init();
    QHash<int, Cue*> m_cueMap;
    TrackPointer m_pLoadedTrack;

    QScopedPointer<TapFilter> m_pTapFilter;
    double m_dLastBpm;

    DlgTagFetcher& m_DlgTagFetcher;

    CoverInfo m_loadedCoverInfo;
    WCoverArtLabel* m_pWCoverArtLabel;
};

#endif /* DLGTRACKINFO_H */
