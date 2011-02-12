#ifndef BEATGRID_H
#define BEATGRID_H

#include <QMutex>
#include <QObject>

#include "trackinfoobject.h"
#include "beats.h"

// BeatGrid is an implementation of the Beats interface that implements an
// infinite grid of beats, aligned to a song simply by a starting offset of the
// first beat and the song's average beats-per-minute.
class BeatGrid : public QObject, public virtual Beats {
    Q_OBJECT
  public:
    BeatGrid(QObject* pParent, TrackPointer pTrack, QByteArray* pByteArray=NULL);
    virtual ~BeatGrid();

    // See method comments in beats.h

    virtual Beats::CapabilitiesFlags getCapabilities() const {
        return BEATSCAP_TRANSLATE | BEATSCAP_SCALE;
    }

    virtual QByteArray* toByteArray() const;
    virtual QString getVersion() const;

    ////////////////////////////////////////////////////////////////////////////
    // Beat calculations
    ////////////////////////////////////////////////////////////////////////////

    virtual double findNextBeat(double dSamples) const;
    virtual double findPrevBeat(double dSamples) const;
    virtual double findClosestBeat(double dSamples) const;
    virtual void findBeats(double startSample, double stopSample, QList<double>* pBeatsList) const;
    virtual bool hasBeatInRange(double startSample, double stopSample) const;
    virtual double getBpm() const;
    virtual double getBpmRange(double startSample, double stopSample) const;

    ////////////////////////////////////////////////////////////////////////////
    // Beat mutations
    ////////////////////////////////////////////////////////////////////////////

    virtual void addBeat(double dBeatSample);
    virtual void removeBeat(double dBeatSample);
    virtual void moveBeat(double dBeatSample, double dNewBeatSample);
    virtual void translate(double dNumSamples);
    virtual void scale(double dScalePercentage);

  signals:
    void updated();

  private slots:
    void slotTrackBpmUpdated(double bpm);

  private:
    void readByteArray(QByteArray* pByteArray);
    // For internal use only.
    bool isValid() const;

    mutable QMutex m_mutex;
    int m_iSampleRate;
    double m_dBpm, m_dFirstBeat;
    double m_dBeatLength;
};


#endif /* BEATGRID_H */
