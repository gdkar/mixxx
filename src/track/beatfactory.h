_Pragma("once")
#include <QHash>
#include "track/beats.h"
class BeatFactory {
  public:
    static BeatsPointer loadBeatsFromByteArray(TrackPointer pTrack,QString beatsVersion,QString beatsSubVersion,QByteArray beatsSerialized);
    static BeatsPointer makeBeatGrid(TrackInfoObject* pTrack,double dBpm, double dFirstBeatSample);
    static QString getPreferredVersion(bool bEnableFixedTempoCorrection);
    static QString getPreferredSubVersion(
        bool bEnableFixedTempoCorrection,
        bool bEnableOffsetCorrection,
        int iMinBpm, int iMaxBpm,
        QHash<QString, QString> extraVersionInfo);
    static BeatsPointer makePreferredBeats(
        TrackPointer pTrack, QVector<double> beats,
        QHash<QString, QString> extraVersionInfo,
        bool bEnableFixedTempoCorrection,
        bool bEnableOffsetCorrection,
        int iSampleRate, int iTotalSamples,
        int iMinBpm, int iMaxBpm);
  private:
    static void deleteBeats(Beats* pBeats);
    BeatFactory() = delete;
};
