/*
 * analyserbeats.cpp
 *
 *  Created on: 16/mar/2011
 *      Author: vittorio
 */

#include <QtDebug>
#include <QVector>
#include <QHash>
#include <QString>

#include "trackinfoobject.h"
#include "track/beatmap.h"
#include "track/beatfactory.h"
#include "analyserbeats.h"
#include "track/beatutils.h"
#include "track/beat_preferences.h"

AnalyserBeats::AnalyserBeats(ConfigObject<ConfigValue>* pConfig,QObject *p)
        : Analyser(p),
          m_pConfig(pConfig),
          m_pVamp(nullptr),
          m_bPreferencesReanalyzeOldBpm(false),
          m_bPreferencesFixedTempo(true),
          m_bPreferencesOffsetCorrection(false),
          m_bPreferencesFastAnalysis(false),
          m_iSampleRate(0),
          m_iTotalSamples(0),
          m_iMinBpm(0),
          m_iMaxBpm(9999) {
}

AnalyserBeats::~AnalyserBeats() {
}

bool AnalyserBeats::initialise(TrackPointer tio, int sampleRate, int totalSamples) {
    if (totalSamples == 0) {return false;}
    auto bPreferencesBeatDetectionEnabled = static_cast<bool>(
        m_pConfig->getValueString( ConfigKey(BPM_CONFIG_KEY, BPM_DETECTION_ENABLED)).toInt());
    if (!bPreferencesBeatDetectionEnabled) {
        qDebug() << "Beat calculation is deactivated";
        return false;
    }
    auto bpmLock = tio->hasBpmLock();
    if (bpmLock) {
        qDebug() << "Track is BpmLocked: Beat calculation will not start";
        return false;
    }
    auto allow_above = static_cast<bool>(m_pConfig->getValueString(
        ConfigKey(BPM_CONFIG_KEY, BPM_ABOVE_RANGE_ENABLED)).toInt());
    if (allow_above) {
        m_iMinBpm = 0;
        m_iMaxBpm = 9999;
    } else {
        m_iMinBpm = m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_RANGE_START)).toInt();
        m_iMaxBpm = m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_RANGE_END)).toInt();
    }
    m_bPreferencesFixedTempo = static_cast<bool>(
        m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_FIXED_TEMPO_ASSUMPTION)).toInt());
    m_bPreferencesOffsetCorrection = static_cast<bool>(
        m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_FIXED_TEMPO_OFFSET_CORRECTION)).toInt());
    m_bPreferencesReanalyzeOldBpm = static_cast<bool>(
        m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_REANALYZE_WHEN_SETTINGS_CHANGE)).toInt());
    m_bPreferencesFastAnalysis = static_cast<bool>(
        m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_FAST_ANALYSIS_ENABLED)).toInt());
    auto library = m_pConfig->getValueString ( ConfigKey(VAMP_CONFIG_KEY, VAMP_ANALYSER_BEAT_LIBRARY));
    auto pluginID = m_pConfig->getValueString( ConfigKey(VAMP_CONFIG_KEY, VAMP_ANALYSER_BEAT_PLUGIN_ID));
    m_pluginId = pluginID;
    m_iSampleRate = sampleRate;
    m_iTotalSamples = totalSamples;
    // if we can load a stored track don't reanalyze it
    auto bShouldAnalyze = !loadStored(tio);
    if (bShouldAnalyze) {
        m_pVamp = new VampAnalyser();
        bShouldAnalyze = m_pVamp->Init(library, pluginID, m_iSampleRate, totalSamples,m_bPreferencesFastAnalysis);
        if (!bShouldAnalyze) {
            delete m_pVamp;
            m_pVamp = nullptr;
        }
    }
    if (bShouldAnalyze) { qDebug() << "Beat calculation started with plugin" << pluginID; }
    else { qDebug() << "Beat calculation will not start"; }
    return bShouldAnalyze;
}

bool AnalyserBeats::loadStored(TrackPointer tio) const {
    int iMinBpm;
    int iMaxBpm;
    auto allow_above = static_cast<bool>(m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_ABOVE_RANGE_ENABLED)).toInt());
    if (allow_above) {
        iMinBpm = 0;
        iMaxBpm = 9999;
    } else {
        iMinBpm = m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_RANGE_START)).toInt();
        iMaxBpm = m_pConfig->getValueString(ConfigKey(BPM_CONFIG_KEY, BPM_RANGE_END)).toInt();
    }
    auto bpmLock = tio->hasBpmLock();
    if (bpmLock) {
        qDebug() << "Track is BpmLocked: Beat calculation will not start";
        return true;
    }
    auto library = m_pConfig->getValueString ( ConfigKey(VAMP_CONFIG_KEY, VAMP_ANALYSER_BEAT_LIBRARY));
    auto pluginID = m_pConfig->getValueString( ConfigKey(VAMP_CONFIG_KEY, VAMP_ANALYSER_BEAT_PLUGIN_ID));
    // At first start config for QM and Vamp does not exist --> set default
    // TODO(XXX): This is no longer present in initialise. Remove?
    if (library.isEmpty() || library.isNull())   library = "libmixxxminimal";
    if (pluginID.isEmpty() || pluginID.isNull()) pluginID = "qm-tempotracker:0";
    // If the track already has a Beats object then we need to decide whether to
    // analyze this track or not.
    auto pBeats = tio->getBeats();
    if (pBeats) {
        auto version = pBeats->getVersion();
        auto subVersion = pBeats->getSubVersion();
        auto extraVersionInfo = getExtraVersionInfo( pluginID, m_bPreferencesFastAnalysis);
        auto newVersion = BeatFactory::getPreferredVersion(m_bPreferencesOffsetCorrection);
        auto newSubVersion = BeatFactory::getPreferredSubVersion(m_bPreferencesFixedTempo, m_bPreferencesOffsetCorrection,
            iMinBpm, iMaxBpm, extraVersionInfo);
        if (version == newVersion && subVersion == newSubVersion) {
            // If the version and settings have not changed then if the world is
            // sane, re-analyzing will do nothing.
            return true;
        } else if (m_bPreferencesReanalyzeOldBpm) {
            return false;
        } else if (pBeats->getBpm() == 0.0) {
            qDebug() << "BPM is 0 for track so re-analyzing despite preference settings.";
            return false;
        } else if (pBeats->findNextBeat(0) <= 0.0) {
            qDebug() << "First beat is 0 for grid so analyzing track to find first beat.";
            return false;
        } else {
            qDebug() << "Beat calculation skips analyzing because the track has"
                     << "a BPM computed by a previous Mixxx version and user"
                     << "preferences indicate we should not change it.";
            return true;
        }
    } else {
        // If we got here, we want to analyze this track.
        return false;
    }
}
void AnalyserBeats::process(const CSAMPLE *pIn, const int iLen) {
    if (!m_pVamp) return;
    bool success = m_pVamp->Process(pIn, iLen);
    if (!success) {
        delete m_pVamp;
        m_pVamp = nullptr;
    }
}
void AnalyserBeats::cleanup(TrackPointer /*tio*/) {
    delete m_pVamp;
    m_pVamp = nullptr;
}
void AnalyserBeats::finalise(TrackPointer tio) {
    if (!m_pVamp) return;
    // Call End() here, because the number of total samples may have been
    // estimated incorrectly.
    auto success = m_pVamp->End();
    qDebug() << "Beat Calculation" << (success ? "complete" : "failed");
    auto beats = m_pVamp->GetInitFramesVector();
    delete m_pVamp;
    m_pVamp = nullptr;
    if (beats.isEmpty()) {
        qDebug() << "Could not detect beat positions from Vamp.";
        return;
    }
    auto extraVersionInfo = getExtraVersionInfo( m_pluginId, m_bPreferencesFastAnalysis);
    auto pBeats = BeatFactory::makePreferredBeats(
        tio, beats, extraVersionInfo,
        m_bPreferencesFixedTempo, m_bPreferencesOffsetCorrection,
        m_iSampleRate, m_iTotalSamples,
        m_iMinBpm, m_iMaxBpm);
    auto pCurrentBeats = tio->getBeats();
    // If the track has no beats object then set our newly generated one
    // regardless of beat lock.
    if (!pCurrentBeats) {
        tio->setBeats(pBeats);
        return;
    }
    // If the track received the beat lock while we were analyzing it then we
    // abort setting it.
    if (tio->hasBpmLock()) {
        qDebug() << "Track was BPM-locked as we were analysing it. Aborting analysis.";
        return;
    }
    // If the user prefers to replace old beatgrids with newly generated ones or
    // the old beatgrid has 0-bpm then we replace it.
    bool zeroCurrentBpm = pCurrentBeats->getBpm() == 0.0;
    if (m_bPreferencesReanalyzeOldBpm || zeroCurrentBpm) {
        if (zeroCurrentBpm) {
            qDebug() << "Replacing 0-BPM beatgrid with a" << pBeats->getBpm() << "beatgrid.";
        }
        tio->setBeats(pBeats);
        return;
    }
    // If we got here then the user doesn't want to replace the beatgrid but
    // since the first beat is zero we'll apply the offset we just detected.
    auto currentFirstBeat = pCurrentBeats->findNextBeat(0);
    auto  newFirstBeat = pBeats->findNextBeat(0);
    if (currentFirstBeat == 0.0 && newFirstBeat > 0) {pCurrentBeats->translate(newFirstBeat);}
}
// static
QHash<QString, QString> AnalyserBeats::getExtraVersionInfo(QString pluginId, bool bPreferencesFastAnalysis) {
    QHash<QString, QString> extraVersionInfo;
    extraVersionInfo["vamp_plugin_id"] = pluginId;
    if (bPreferencesFastAnalysis) {extraVersionInfo["fast_analysis"] = "1";}
    return extraVersionInfo;
}