#ifndef ANALYSERKEY_H
#define ANALYSERKEY_H

#include <QHash>
#include <QString>

#include "an_queue/analyser.h"
#include "configobject.h"
#include "trackinfoobject.h"
//#include "vamp/vampanalyser.h"
class VampAnalyser;
class AnalyserKey : public Analyser {
  public:
    AnalyserKey(ConfigObject<ConfigValue>* pConfig);
    virtual ~AnalyserKey();

    virtual bool initialize(TrackPointer tio, int sampleRate, int totalSamples) override;
    virtual bool loadStored(TrackPointer tio) const override;
    virtual void process(const CSAMPLE *pIn, const int iLen) override;
    virtual void finalize(TrackPointer tio) override;
    virtual void cleanup(TrackPointer tio) override;

  private:
    static QHash<QString, QString> getExtraVersionInfo(
        QString pluginId, bool bPreferencesFastAnalysis);

    ConfigObject<ConfigValue>* m_pConfig;
    VampAnalyser* m_pVamp;
    QString m_pluginId;
    int m_iSampleRate;
    int m_iTotalSamples;

    bool m_bPreferencesKeyDetectionEnabled;
    bool m_bPreferencesFastAnalysisEnabled;
    bool m_bPreferencesReanalyzeEnabled;
};

#endif /* ANALYSERKEY_H */
