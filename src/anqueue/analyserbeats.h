/* Beat Tracking test via vamp-plugins
 * analyserbeats.h
 *
 *  Created on: 16/mar/2011
 *      Author: Vittorio Colao
 */

_Pragma("once")
#include <QHash>
#include <memory>
#include "analyser.h"
#include "configobject.h"
#include "vamp/vampanalyser.h"

class AnalyserBeats: public Analyser {
  public:
    AnalyserBeats(ConfigObject<ConfigValue>* pConfig);
    virtual ~AnalyserBeats();
    bool initialise(TrackPointer tio, int sampleRate, int totalSamples);
    bool loadStored(TrackPointer tio) const;
    void process(const CSAMPLE *pIn, const int iLen);
    void cleanup(TrackPointer tio);
    void finalise(TrackPointer tio);

  private:
    static QHash<QString, QString> getExtraVersionInfo(QString pluginId, bool bPreferencesFastAnalysis);
    QVector<double> correctedBeats(QVector<double> rawbeats);
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    std::unique_ptr<VampAnalyser> m_pVamp;
    QString m_pluginId;
    bool m_bPreferencesReanalyzeOldBpm = true;
    bool m_bPreferencesFixedTempo = false;
    bool m_bPreferencesOffsetCorrection = true;
    bool m_bPreferencesFastAnalysis = false;
    int m_iSampleRate = -1, m_iTotalSamples = -1;
    int m_iMinBpm = 0, m_iMaxBpm = 9999;
};
