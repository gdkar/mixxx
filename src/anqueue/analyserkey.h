_Pragma("once")
#include <QHash>
#include <QString>

#include "analyser.h"
#include "configobject.h"
#include "trackinfoobject.h"
#include "vamp/vampanalyser.h"

class AnalyserKey : public Analyser {
  Q_OBJECT;
  public:
    AnalyserKey(ConfigObject<ConfigValue>* pConfig, QObject *);
    virtual ~AnalyserKey();
    bool initialise(TrackPointer tio, int sampleRate, int totalSamples);
    bool loadStored(TrackPointer tio) const;
    void process(const CSAMPLE *pIn, const int iLen);
    void finalise(TrackPointer tio);
    void cleanup(TrackPointer tio);
  private:
    static QHash<QString, QString> getExtraVersionInfo(QString pluginId, bool bPreferencesFastAnalysis);
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    VampAnalyser* m_pVamp = nullptr;
    QString m_pluginId;
    int m_iSampleRate = 44100;
    int m_iTotalSamples                    = 0;
    bool m_bPreferencesKeyDetectionEnabled = false;
    bool m_bPreferencesFastAnalysisEnabled = false;
    bool m_bPreferencesReanalyzeEnabled    = false;
};
