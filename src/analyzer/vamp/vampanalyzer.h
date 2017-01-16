/*
 * vampanalyzer.h
 *  Created on: 14/mar/2011
 *      Author: Vittorio Colao
 *      Original ideas taken from Audacity VampEffect class from Chris Cannam.
 */

#ifndef ANALYZER_VAMP_VAMPANALYZER_H
#define ANALYZER_VAMP_VAMPANALYZER_H

#include <QString>
#include <QList>
#include <QVector>
#include <vamp-hostsdk/vamp-hostsdk.h>
#include <utility>
#include <memory>
#include <array>
#include "analyzer/vamp/vamppluginloader.h"
#include "preferences/usersettings.h"
#include "util/sample.h"

class VampAnalyzer {
  public:
    VampAnalyzer();
    virtual ~VampAnalyzer();

    bool Init(const QString pluginlibrary, const QString pluginid,
              const int samplerate, const int TotalSamples, bool bFastAnalysis);
    bool Process(const CSAMPLE *pIn, const int iLen);
    bool End();
    bool SetParameter(const QString parameter, const double value);

    QVector<double> GetInitFramesVector();
    QVector<double> GetEndFramesVector();
    QVector<QString> GetLabelsVector();
    QVector<double> GetFirstValuesVector();
    QVector<double> GetLastValuesVector();

    // Initialize the VAMP_PATH environment variable to point to the default
    // places that Mixxx VAMP plugins are deployed on installation. If a
    // VAMP_PATH environment variable is already set by the user, then this
    // method appends to that.
    static void initializePluginPaths();
    void SelectOutput(const int outputnumber);

  private:
    Vamp::HostExt::PluginLoader::PluginKey m_key;
    int m_iSampleCount, m_iOUT, m_iRemainingSamples,
        m_iBlockSize, m_iStepSize, m_rate, m_iOutput;
    std::array<CSAMPLE  *,2> m_pluginbuf;
    std::array<std::unique_ptr<CSAMPLE[]>,2> m_pluginstore;
    std::unique_ptr<Vamp::Plugin> m_plugin;
    Vamp::Plugin::ParameterList mParameters;
    Vamp::Plugin::FeatureList m_Results;

    bool m_bDoNotAnalyseMoreSamples;
    bool m_FastAnalysisEnabled;
    int m_iMaxSamplesToAnalyse;
};

#endif /* ANALYZER_VAMP_VAMPANALYZER_H */
