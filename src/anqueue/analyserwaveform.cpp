#include <QImage>
#include <QtDebug>
#include <QTime>
#include <QtDebug>

#include "analyserwaveform.h"
#include "engine/engineobject.h"
#include "engine/enginefilterbutterworth8.h"
#include "engine/enginefilterbessel4.h"
#include "library/trackcollection.h"
#include "library/dao/analysisdao.h"
#include "trackinfoobject.h"
#include "waveform/waveformfactory.h"
namespace{
  inline CSAMPLE scaleSignal(CSAMPLE invalue, FilterIndex index = FilterCount) {
      if (!invalue) { return 0;}
      else if (index == Low || index == Mid) {
          //return pow(invalue, 2 * 0.5);
          return invalue;
      } else return std::pow(invalue, 2 * 0.316f);
  }
}
WaveformStride::WaveformStride(double samples, double averageSamples)
        : m_position(0),
          m_length(samples),
          m_averageLength(averageSamples),
          m_averagePosition(0),
          m_averageDivisor(0),
          m_postScaleConversion(static_cast<float>( std::numeric_limits<unsigned char>::max())) {
    for (int i = 0; i < ChannelCount; ++i) {
        m_overallData[i] = 0.0f;
        m_averageOverallData[i] = 0.0f;
        for (int f = 0; f < FilterCount; ++f) {
            m_filteredData[i][f] = 0.0f;
            m_averageFilteredData[i][f] = 0.0f;
        }
    }
}
void WaveformStride::reset() {
        m_position = 0;
        m_averageDivisor = 0;
        for (int i = 0; i < ChannelCount; ++i) {
            m_overallData[i] = 0.0f;
            m_averageOverallData[i] = 0.0f;
            for (int f = 0; f < FilterCount; ++f) {
                m_filteredData[i][f] = 0.0f;
                m_averageFilteredData[i][f] = 0.0f;
            }
        }
    }
void WaveformStride::store(WaveformData* data) {
    for (int i = 0; i < ChannelCount; ++i) {
        data[i].filtered.all = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_overallData[i]) + 0.5f));
        data[i].filtered.low = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_filteredData[i][Low], Low) + 0.5f));
        data[i].filtered.mid = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_filteredData[i][Mid], Mid) + 0.5f));
        data[i].filtered.high= static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_filteredData[i][High], High) + 0.5f));
    }
    m_averageDivisor++;
    for (int i = 0; i < ChannelCount; ++i) {
        m_averageOverallData[i] += m_overallData[i];
        m_overallData[i] = 0.0f;
        for (int f = 0; f < FilterCount; ++f) {
            m_averageFilteredData[i][f] += m_filteredData[i][f];
            m_filteredData[i][f] = 0.0f;
        }
    }
}
void WaveformStride::averageStore(WaveformData *data)
{
  if ( m_averageDivisor){
    for (int i = 0; i < ChannelCount; ++i) {
        data[i].filtered.all = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_averageOverallData[i]/m_averageDivisor) + 0.5f));
        data[i].filtered.low= static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_averageFilteredData[i][Low]/m_averageDivisor) + 0.5f));
        data[i].filtered.mid= static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_averageFilteredData[i][Mid]/m_averageDivisor) + 0.5f));
        data[i].filtered.high= static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_averageFilteredData[i][High]/m_averageDivisor) + 0.5f));
    }  
  }else{
    for (int i = 0; i < ChannelCount; ++i) {
        data[i].filtered.all = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_overallData[i]) + 0.5f));
        data[i].filtered.low = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_filteredData[i][Low], Low) + 0.5f));
        data[i].filtered.mid = static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_filteredData[i][Mid], Mid) + 0.5f));
        data[i].filtered.high= static_cast<unsigned char>(std::min(255.f,m_postScaleConversion * scaleSignal(m_filteredData[i][High], High) + 0.5f));
    }
  }
}
AnalyserWaveform::AnalyserWaveform(ConfigObject<ConfigValue>* pConfig, QObject *p) 
  : Analyser(p)
{
    qDebug() << "AnalyserWaveform::AnalyserWaveform()";
    m_filter[0] = 0;
    m_filter[1] = 0;
    m_filter[2] = 0;
    static std::atomic<int> i{0};
    m_database = QSqlDatabase::addDatabase("QSQLITE", "WAVEFORM_ANALYSIS" + QString::number(i++));
    if (!m_database.isOpen()) {
        m_database.setHostName("localhost");
        m_database.setDatabaseName(QDir(pConfig->getSettingsPath()).filePath("mixxxdb.sqlite"));
        m_database.setUserName("mixxx");
        m_database.setPassword("mixxx");
        //Open the database connection in this thread.
        if (!m_database.open()) {qDebug() << "Failed to open database from analyser thread." << m_database.lastError();}
    }
    m_timer       = std::make_unique<QTime>();
    m_analysisDao = std::make_unique<AnalysisDao>(m_database, pConfig);
}
AnalyserWaveform::~AnalyserWaveform() {
    qDebug() << "AnalyserWaveform::~AnalyserWaveform()";
    destroyFilters();
    m_database.close();
}
bool AnalyserWaveform::initialise(TrackPointer tio, int sampleRate, int totalSamples) {
    m_skipProcessing = false;
    m_timer->start();
    if (totalSamples == 0) {
        qWarning() << "AnalyserWaveform::initialise - no waveform/waveform summary";
        return false;
    }
    // If we don't need to calculate the waveform/wavesummary, skip.
    if (loadStored(tio)) { m_skipProcessing = true;}
    else {
        // Now actually initialize the AnalyserWaveform:
        destroyFilters();
        createFilters(sampleRate);
        //TODO (vrince) Do we want to expose this as settings or whatever ?
        const auto mainWaveformSampleRate = 441;
        // two visual sample per pixel in full width overview in full hd
        const auto summaryWaveformSamples = 2 * 1920;
        m_waveform        .reset(new Waveform( sampleRate, totalSamples, mainWaveformSampleRate, -1));
        m_waveformSummary .reset(new Waveform(sampleRate, totalSamples, mainWaveformSampleRate,summaryWaveformSamples));
        // Now, that the Waveform memory is initialized, we can set set them to
        // the TIO. Be aware that other threads of Mixxx can touch them from
        // now.
        tio->setWaveform(m_waveform);
        tio->setWaveformSummary(m_waveformSummary);
        m_waveformData = m_waveform->data();
        m_waveformSummaryData = m_waveformSummary->data();
        m_stride = WaveformStride(m_waveform->getAudioVisualRatio(), m_waveformSummary->getAudioVisualRatio());
        m_currentStride = 0;
        m_currentSummaryStride = 0;
        //debug
        //m_waveform->dump();
        //m_waveformSummary->dump();
    }
    return !m_skipProcessing;
}
bool AnalyserWaveform::loadStored(TrackPointer tio) const {
    auto pTrackWaveform = tio->getWaveform();
    auto pTrackWaveformSummary = tio->getWaveformSummary();
    auto pLoadedTrackWaveform = decltype(pTrackWaveform){};
    auto pLoadedTrackWaveformSummary = decltype(pTrackWaveformSummary){};
    auto trackId = tio->getId();
    auto missingWaveform = pTrackWaveform.isNull();
    auto missingWavesummary = pTrackWaveformSummary.isNull();
    if (trackId.isValid() && (missingWaveform || missingWavesummary)) {
        auto analyses = m_analysisDao->getAnalysesForTrack(trackId);
        for(const auto &analysis : analyses) {
            auto vc = WaveformFactory::VersionClass{};
            if (analysis.type == AnalysisDao::TYPE_WAVEFORM) {
                vc = WaveformFactory::waveformVersionToVersionClass(analysis.version);
                if (missingWaveform && vc == WaveformFactory::VC_USE) {
                    pLoadedTrackWaveform = ConstWaveformPointer(WaveformFactory::loadWaveformFromAnalysis(analysis));
                    missingWaveform = false;
                } else if (vc != WaveformFactory::VC_KEEP) {m_analysisDao->deleteAnalysis(analysis.analysisId);}
            } if (analysis.type == AnalysisDao::TYPE_WAVESUMMARY) {
                vc = WaveformFactory::waveformSummaryVersionToVersionClass(analysis.version);
                if (missingWavesummary && vc == WaveformFactory::VC_USE) {
                    pLoadedTrackWaveformSummary = ConstWaveformPointer(WaveformFactory::loadWaveformFromAnalysis(analysis));
                    missingWavesummary = false;
                } else if (vc != WaveformFactory::VC_KEEP) {m_analysisDao->deleteAnalysis(analysis.analysisId);}
            }
        }
    }
    // If we don't need to calculate the waveform/wavesummary, skip.
    if (!missingWaveform && !missingWavesummary) {
        qDebug() << "AnalyserWaveform::loadStored - Stored waveform loaded";
        if (pLoadedTrackWaveform)        {tio->setWaveform(pLoadedTrackWaveform);}
        if (pLoadedTrackWaveformSummary) {tio->setWaveformSummary(pLoadedTrackWaveformSummary);}
        return true;
    }
    return false;
}
void AnalyserWaveform::createFilters(int sampleRate) {
    // m_filter[Low] = new EngineFilterButterworth8(FILTER_LOWPASS, sampleRate, 200);
    // m_filter[Mid] = new EngineFilterButterworth8(FILTER_BANDPASS, sampleRate, 200, 2000);
    // m_filter[High] = new EngineFilterButterworth8(FILTER_HIGHPASS, sampleRate, 2000);
    m_filter[Low]  = std::make_unique<EngineFilterBessel4Low>(sampleRate, 600,nullptr);
    m_filter[Mid]  = std::make_unique<EngineFilterBessel4Band>(sampleRate, 600, 4000,nullptr);
    m_filter[High] = std::make_unique<EngineFilterBessel4High>(sampleRate, 4000,nullptr);
    // settle filters for silence in preroll to avoids ramping (Bug #1406389)
    for (int i = 0; i < FilterCount; ++i) {m_filter[i]->assumeSettled();}
}
void AnalyserWaveform::destroyFilters() {
    for (int i = 0; i < FilterCount; ++i) {m_filter[i] = nullptr;;}
}
void AnalyserWaveform::process(const CSAMPLE* buffer, const int bufferLength) {
    if (m_skipProcessing || !m_waveform || !m_waveformSummary) return;
    //this should only append once if bufferLength is constant
    if (bufferLength > (int)m_buffers[0].size()) {
        m_buffers[Low].resize(bufferLength);
        m_buffers[Mid].resize(bufferLength);
        m_buffers[High].resize(bufferLength);
    }
    m_filter[Low]->process (buffer, &m_buffers[Low][0], bufferLength);
    m_filter[Mid]->process (buffer, &m_buffers[Mid][0], bufferLength);
    m_filter[High]->process(buffer, &m_buffers[High][0], bufferLength);
    for (int i = 0; i < bufferLength; i+=2) {
        // Take max value, not average of data
        CSAMPLE cover[2] = { std::abs(buffer[i]),          std::abs(buffer[i + 1]) };
        CSAMPLE clow[2]  = { std::abs(m_buffers[Low][i]),  std::abs(m_buffers[Low][i + 1]) };
        CSAMPLE cmid[2]  = { std::abs(m_buffers[Mid][i]),  std::abs(m_buffers[Mid][i + 1]) };
        CSAMPLE chigh[2] = { std::abs(m_buffers[High][i]), std::abs(m_buffers[High][i + 1]) };
        storeIfGreater(&m_stride.m_overallData[Left], cover[Left]);
        storeIfGreater(&m_stride.m_overallData[Right], cover[Right]);
        storeIfGreater(&m_stride.m_filteredData[Left][Low], clow[Left]);
        storeIfGreater(&m_stride.m_filteredData[Right][Low], clow[Right]);
        storeIfGreater(&m_stride.m_filteredData[Left][Mid], cmid[Left]);
        storeIfGreater(&m_stride.m_filteredData[Right][Mid], cmid[Right]);
        storeIfGreater(&m_stride.m_filteredData[Left][High], chigh[Left]);
        storeIfGreater(&m_stride.m_filteredData[Right][High], chigh[Right]);
        m_stride.m_position++;
        if (std::fmod(m_stride.m_position, m_stride.m_length) < 1) {
            if (m_currentStride + ChannelCount > m_waveform->size()) {
                qWarning() << "AnalyserWaveform::process - currentStride >= waveform size";
                return;
            }
            m_stride.store(m_waveformData + m_currentStride);
            m_currentStride += 2;
            m_waveform->setCompletion(m_currentStride);
        }
        if (std::fmod(m_stride.m_position, m_stride.m_averageLength) < 1) {
            if (m_currentSummaryStride + ChannelCount > m_waveformSummary->size()) {
                qWarning() << "AnalyserWaveform::process - current summary stride >= waveform summary size";
                return;
            }
            m_stride.averageStore(m_waveformSummaryData + m_currentSummaryStride);
            m_currentSummaryStride += 2;
            m_waveformSummary->setCompletion(m_currentSummaryStride);
        }
    }
}
void AnalyserWaveform::cleanup(TrackPointer tio) {
    if (m_skipProcessing) {return;}
    tio->setWaveform(ConstWaveformPointer());
    // Since clear() could delete the waveform, clear our pointer to the
    // waveform's vector data first.
    m_waveformData = nullptr;
    m_waveform.clear();
    tio->setWaveformSummary(ConstWaveformPointer());
    // Since clear() could delete the waveform, clear our pointer to the
    // waveform's vector data first.
    m_waveformSummaryData = nullptr;
    m_waveformSummary.clear();
}
void AnalyserWaveform::finalise(TrackPointer tio) {
    if (m_skipProcessing) return;
    // Force completion to waveform size
    if (m_waveform)
    {
        m_waveform->setCompletion(m_waveform->size());
        m_waveform->setVersion(WaveformFactory::currentWaveformVersion());
        m_waveform->setDescription(WaveformFactory::currentWaveformDescription());
        // Since clear() could delete the waveform, clear our pointer to the
        // waveform's vector data first.
        m_waveformData = nullptr;
        m_waveform.clear();
        tio->waveformUpdated();
    }
    // Force completion to waveform size
    if (m_waveformSummary)
    {
        m_waveformSummary->setCompletion(m_waveformSummary->size());
        m_waveformSummary->setVersion(WaveformFactory::currentWaveformSummaryVersion());
        m_waveformSummary->setDescription(WaveformFactory::currentWaveformSummaryDescription());
        // Since clear() could delete the waveform, clear our pointer to the
        // waveform's vector data first.
        m_waveformSummaryData = nullptr;
        m_waveformSummary.clear();
        tio->waveformSummaryUpdated();
    }
}
void AnalyserWaveform::storeIfGreater(float* pDest, float source) {if (*pDest < source) {*pDest = source;}}
