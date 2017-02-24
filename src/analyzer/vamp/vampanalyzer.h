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
    using Plugin = Vamp::Plugin;
    using PluginLoader = Vamp::HostExt::PluginLoader;
    using PluginKey = PluginLoader::PluginKey;
    using PluginKeyList = PluginLoader::PluginKeyList;
    using PluginCategoryHierarchy = PluginLoader::PluginCategoryHierarchy;
    using Parameter = Vamp::Plugin::ParameterDescriptor;
    using ParameterList = Vamp::Plugin::ParameterList;
    using Feature = Vamp::Plugin::Feature;
    using FeatureList = Vamp::Plugin::FeatureList;

    VampAnalyzer();
    virtual ~VampAnalyzer();

    bool Init(const QString pluginlibrary, const QString pluginid,
              const int samplerate, const int TotalSamples, bool bFastAnalysis, QString options = QString{});
    bool Process(const CSAMPLE *pIn, const int iLen);
    bool End();
    bool SetParameter(const QString parameter, const double value);
    bool SetParameters(QStringList parametrs);
    bool SetParameters(QString parametrs);

    QVector<double> GetInitFramesVector();
    QVector<double> GetEndFramesVector();
    QVector<QString> GetLabelsVector();
    QVector<double> GetFirstValuesVector();
    QVector<double> GetLastValuesVector();

    void SelectOutput(const int outputnumber);

  private:
    PluginKey m_key;
    int m_iSampleCount;
    int m_iOUT;
    int m_iRemainingSamples;
    int m_iBlockSize;
    int m_iStepSize;
    int m_rate;
    int m_iOutput;

    std::array<CSAMPLE  *,2> m_pluginbuf;
    std::array<std::unique_ptr<CSAMPLE[]>,2> m_pluginstore;
    std::unique_ptr<Vamp::Plugin> m_plugin;
    ParameterList m_Parameters;
    FeatureList   m_Results;

    bool m_bDoNotAnalyseMoreSamples;
    bool m_FastAnalysisEnabled;
    int m_iMaxSamplesToAnalyse;
};

#endif /* ANALYZER_VAMP_VAMPANALYZER_H */
