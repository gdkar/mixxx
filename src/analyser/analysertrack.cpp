#include "analysertrack.h"

AnalyserTrack::AnalyserTrack(TrackPointer pTrack,QObject *parent)
  : QObject(parent)
  , m_pTrack(pTrack)
  , m_soundSourceProxy(pTrack)
  , m_pAudioSource(m_soundSourceProxy.open(2){

}
