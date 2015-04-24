#ifndef ANALYSERTRACK_H
#define ANALYSERTRACK_H
#include <QtDebug>
#include <QMutex>
#include <QMutexLocker>
#include <typeinfo>
#include <qmath.h>
#include <qsharedpointer.h>
#include <qatomic.h>
#include <QThread>
#include <QRunnable>
#include "trackinfoobject.h"
#include "analyser.h"
#include "library/trackcollection.h"
#include "util/timer.h"
#include "soundsourceproxy.h"
#include "samplebuffer.h"

class AnalyserTrack : public QObject {
  Q_OBJECT;

  public:
    explicit AnalyserTrack(TrackPointer pTrack,QObject *parent=0);
    virtual ~AnlyserTrack();

  public slots:
    void onBlockReady();
    void addAnalyser(QSharedPointer<Analyser> an);
  signals:
    void blockReady();
    void trackProgress(int progress);
    void trackComplete(TrackPointer pTrack);

  private:
    Mixxx::AudioSourcePointer m_pAudioSource;
    SoundSourceProxy          m_soundSourceProxy;
    TrackPointer              m_pTrack;
    QVector<QSharedPointer<Analyser> > m_analysers;
    SampleBuffer              m_sampleBuffer;
};

#endif
