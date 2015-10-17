_Pragma("once")
#include <QImage>
#include <QSqlDatabase>
#include <limits>
#include <memory>

#include "configobject.h"
#include "analyser.h"
#include "waveform/waveform.h"
#include "util/math.h"

//NOTS vrince some test to segment sound, to apply color in the waveform
//#define TEST_HEAT_MAP

class EngineFilterIIRBase;
class Waveform;
class AnalysisDao;

struct WaveformStride {
    WaveformStride(double samples, double averageSamples);
    void reset();
    void store(WaveformData* data);
    void averageStore(WaveformData* data);
    int m_position;
    double m_length;
    double m_averageLength;
    int m_averagePosition;
    int m_averageDivisor;
    float m_overallData[ChannelCount];
    float m_filteredData[ChannelCount][FilterCount];
    float m_averageOverallData[ChannelCount];
    float m_averageFilteredData[ChannelCount][FilterCount];
    float m_postScaleConversion;
};
class AnalyserWaveform : public Analyser {
  public:
    AnalyserWaveform(ConfigObject<ConfigValue>* pConfig, QObject *p=nullptr);
    virtual ~AnalyserWaveform();
    bool initialise(TrackPointer tio, int sampleRate, int totalSamples);
    bool loadStored(TrackPointer tio) const;
    void process(const CSAMPLE *buffer, const int bufferLength);
    void cleanup(TrackPointer tio);
    void finalise(TrackPointer tio);
  private:
    void storeCurentStridePower();
    void resetCurrentStride();
    void createFilters(int sampleRate);
    void destroyFilters();
    void storeIfGreater(float* pDest, float source);
  private:
    bool m_skipProcessing = false;
    WaveformPointer m_waveform;
    WaveformPointer m_waveformSummary;
    WaveformData* m_waveformData        = nullptr;
    WaveformData* m_waveformSummaryData = nullptr;
    WaveformStride m_stride;
    int m_currentStride = 0;
    int m_currentSummaryStride = 0;
    std::unique_ptr<EngineFilterIIRBase> m_filter[FilterCount];
    std::vector<float> m_buffers[FilterCount];
    std::unique_ptr<QTime> m_timer;
    QSqlDatabase m_database;
    std::unique_ptr<AnalysisDao> m_analysisDao ;
};
