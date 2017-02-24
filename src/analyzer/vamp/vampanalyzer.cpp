/*
 * vampanalyzer.cpp
 *
 *  Created on: 14/mar/2011
 *      Author: Vittorio Colao
 */
#include "analyzer/vamp/vampanalyzer.h"
#include "analyzer/vamp/vamppluginloader.h"

VampAnalyzer::VampAnalyzer()
    : m_iSampleCount(0),
      m_iOUT(0),
      m_iRemainingSamples(0),
      m_iBlockSize(0),
      m_iStepSize(0),
      m_rate(0),
      m_iOutput(0),
      m_pluginbuf{nullptr,nullptr},
      m_plugin(nullptr),
      m_bDoNotAnalyseMoreSamples(false),
      m_FastAnalysisEnabled(false),
      m_iMaxSamplesToAnalyse(0) {
}

VampAnalyzer::~VampAnalyzer()
{
}

bool VampAnalyzer::Init(const QString pluginlibrary, const QString pluginid,
                        const int samplerate, const int TotalSamples, bool bFastAnalysis,
                        QString options)
{
    m_iRemainingSamples = TotalSamples;
    m_rate = samplerate;

    if (samplerate <= 0.0) {
        qDebug() << "VampAnalyzer: Track has non-positive samplerate";
        return false;
    }

    if (TotalSamples <= 0) {
        qDebug() << "VampAnalyzer: Track has non-positive # of samples";
        return false;
    }

    if (m_plugin ) {
        m_plugin.reset();
        qDebug() << "VampAnalyzer: kill plugin";
    }

//    auto loader = VampPluginLoader::getInstance();
    auto pluginlist = pluginid.split(":");
    if (pluginlist.size() != 2) {
        qDebug() << "VampAnalyzer: got malformed pluginid: " << pluginid;
        return false;
    }

    auto isNumber = false;
    auto outputnumber = (pluginlist.at(1)).toInt(&isNumber);
    if (!isNumber) {
        qDebug() << "VampAnalyzer: got malformed pluginid: " << pluginid;
        return false;
    }

    auto plugin = pluginlist.at(0);
    auto pluginLoader = mixxx::VampPluginLoader{};
    m_key = pluginLoader.composePluginKey(pluginlibrary.toStdString(),
                                     plugin.toStdString());
    m_plugin = pluginLoader.loadPlugin(m_key, m_rate,
                                  Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE);

    if (!m_plugin) {
        qDebug() << "VampAnalyzer: Cannot load Vamp Plug-in.";
        qDebug() << "Please copy libmixxxminimal.so from build dir to one of the following:";

        for (auto && sub : Vamp::PluginHostAdapter::getPluginPath()) {
            qDebug() << QString::fromStdString(path[i]);
        }
        return false;
    }
    auto outputs = m_plugin->getOutputDescriptors();
    if (outputs.empty()) {
        qDebug() << "VampAnalyzer: Plugin has no outputs!";
        return false;
    }
    SelectOutput(outputnumber);

    m_iBlockSize = m_plugin->getPreferredBlockSize();
    qDebug() << "Vampanalyzer BlockSize: " << m_iBlockSize;
    if (m_iBlockSize == 0) {
        // A plugin that can handle any block size may return 0. The final block
        // size will be set in the initialize() call. Since 0 means it is
        // accepting any size, 1024 should be good
        m_iBlockSize = 1024;
        qDebug() << "Vampanalyzer: setting m_iBlockSize to 1024";
    }

    m_iStepSize = m_plugin->getPreferredStepSize();
    qDebug() << "Vampanalyzer StepSize: " << m_iStepSize;
    if (m_iStepSize == 0 || m_iStepSize > m_iBlockSize) {
        // A plugin may return 0 if it has no particular interest in the step
        // size. In this case, the host should make the step size equal to the
        // block size if the plugin is accepting input in the time domain. If
        // the plugin is accepting input in the frequency domain, the host may
        // use any step size. The final step size will be set in the
        // initialize() call.
        m_iStepSize = m_iBlockSize;
        qDebug() << "Vampanalyzer: setting m_iStepSize to" << m_iStepSize;
    }
    if(!options.isNull())
        SetParameters(options);

    if (!m_plugin->initialise(2, m_iStepSize, m_iBlockSize)) {
        qDebug() << "VampAnalyzer: Cannot initialize plugin";
        return false;
    }
    // Here we are using m_iBlockSize: it cannot be 0
    m_pluginstore[0] = std::make_unique<CSAMPLE[]>(m_iBlockSize);
    m_pluginstore[1] = std::make_unique<CSAMPLE[]>(m_iBlockSize);
    m_pluginbuf[0] = m_pluginstore[0].get();
    m_pluginbuf[1] = m_pluginstore[1].get();

    m_FastAnalysisEnabled = bFastAnalysis;
    if (m_FastAnalysisEnabled) {
        qDebug() << "Using fast analysis methods for BPM and Replay Gain.";
        m_iMaxSamplesToAnalyse = 120 * m_rate; //only consider the first minute
    }
    return true;
}

bool VampAnalyzer::Process(const CSAMPLE *pIn, const int iLen)
{
    if (!m_plugin) {
        qDebug() << "VampAnalyzer: Plugin not loaded";
        return false;
    }

    if (m_pluginbuf[0] == NULL || m_pluginbuf[1] == NULL) {
        qDebug() << "VampAnalyzer: Buffer points to NULL";
        return false;
    }

    if (m_bDoNotAnalyseMoreSamples) {
        return true;
    }

    auto iIN = 0;
    auto lastsamples = false;
    m_iRemainingSamples -= iLen;

    while (iIN < iLen / 2) { //4096
        m_pluginbuf[0][m_iOUT] = pIn[2 * iIN]; //* 32767;
        m_pluginbuf[1][m_iOUT] = pIn[2 * iIN + 1]; //* 32767;

        m_iOUT++;
        iIN++;
        /*
         * Note 'm_iRemainingSamples' is initialized with
         * the number of total samples.
         * Thus, 'm_iRemainingSamples' will only become <= 0
         * if the number of total samples --which may be incorrect--
         * is correct.
         *
         * The following if-block works under optimal conditions
         * If the total number of samples is incorrect
         * VampAnalyzer:End() handles it.
         */
        if (m_iRemainingSamples <= 0 && iIN == iLen / 2) {
            lastsamples = true;
            //qDebug() << "LastSample reached";
            while (m_iOUT < m_iBlockSize) {
                m_pluginbuf[0][m_iOUT] = 0;
                m_pluginbuf[1][m_iOUT] = 0;
                m_iOUT++;
            }
        }
        if (m_iOUT == m_iBlockSize) { //Blocksize 1024
            //qDebug() << "VAMP Block size reached";
            //qDebug() << "Ramaining samples=" << m_iRemainingSamples;
            auto timestamp = Vamp::RealTime::frame2RealTime(m_iSampleCount, m_rate);
            auto features = m_plugin->process(m_pluginbuf.data(), timestamp);
            m_Results.insert(m_Results.end(), features[m_iOutput].begin(),
                             features[m_iOutput].end());

            if (lastsamples) {
                auto features = m_plugin->getRemainingFeatures();
                m_Results.insert(m_Results.end(), features[m_iOutput].begin(),
                                 features[m_iOutput].end());
            }

            m_iSampleCount += m_iBlockSize;
            m_iOUT = 0;

            // The step size indicates preferred distance in sample frames
            // between successive calls to process(). To obey the step-size, we
            // move (m_iBlockSize - m_iStepSize) samples from m_iStepSize'th
            // position to 0.
            while (m_iOUT < (m_iBlockSize - m_iStepSize)) {
                auto lframe = m_pluginbuf[0][m_iOUT + m_iStepSize];
                auto rframe = m_pluginbuf[1][m_iOUT + m_iStepSize];
                m_pluginbuf[0][m_iOUT] = lframe;
                m_pluginbuf[1][m_iOUT] = rframe;
                m_iOUT++;
            }

            // If a track has a duration of more than our set limit, do not
            // analyse more.
            if (m_iMaxSamplesToAnalyse > 0 && m_iSampleCount >= m_iMaxSamplesToAnalyse) {
                m_bDoNotAnalyseMoreSamples = true;
                m_iRemainingSamples = 0;
            }
        }
    }
    return true;
}

bool VampAnalyzer::End()
{
    // If the total number of samples has been estimated incorrectly
    if (m_iRemainingSamples > 0) {
        auto features = m_plugin->getRemainingFeatures();
        m_Results.insert(m_Results.end(), features[m_iOutput].begin(),features[m_iOutput].end());
    }
    // Clearing buffer arrays
    m_pluginbuf[0] = m_pluginbuf[1] = nullptr;
    m_pluginstore[0].release();
    m_pluginstore[1].release();
    return true;
}

bool VampAnalyzer::SetParameter(const QString parameter, double value)
{
    if(m_plugin) {
        m_plugin->setParameter(parameter.toStdString(), float(value));
        qDebug() << "Setting parameter" << parameter << "to value" << value;
        return true;
    }else{
        qDebug() << "failed to set parameter" << parameter << "to value" << value;
        return false;
    }
}
bool VampAnalyzer::SetParameters(QStringList parameters)
{
    qDebug() << "Setting Vamp Plugin Parameters";
    auto ok = false;
    if(m_plugin) {
        auto paramlist = m_plugin->getParameterDescriptors();
        auto parammap = QMap<QString,Parameter>{};
        for(auto && desc : paramlist)
            parammap[QString::fromStdString(desc.identifier)]=desc;
        auto params = QVector<std::pair<QString,float> >{};
        for(auto paramstr: parameters) {
            auto parts = paramstr.split("=");
            if(parts.size() != 2)
                continue;
            auto id = parts[0];
            if(!parammap.contains(id))
                continue;
            auto value = parts[1].toFloat(&ok);
            if(!ok) {
                auto val = parts[1].toStdString();
                value = 0.0f;
                auto && desc = parammap[id];
                if(!desc.valueNames.empty()) {
                    for(auto && dname : desc.valueNames) {
                        if(dname == val) {
                            ok = true;
                            break;
                        }else{
                            value += 1.0f;
                        }
                    }
                }
            }
            if(ok)
                params.append({id,value});
        }
        for(auto && x : params)
            SetParameter(std::get<0>(x),std::get<1>(x));
    }
    return ok;
}
bool VampAnalyzer::SetParameters(QString parameters)
{
    return SetParameters(parameters.split(" "));
}
<<<<<<< HEAD

void VampAnalyzer::SelectOutput(const int outputnumber) {
=======
void VampAnalyzer::SelectOutput(int outputnumber)
{
>>>>>>> pass parameters to vamp plugins
    auto outputs = m_plugin->getOutputDescriptors();
    if (outputnumber >= 0 && outputnumber < int(outputs.size()))
        m_iOutput = outputnumber;
}

QVector<double> VampAnalyzer::GetInitFramesVector()
{
    QVector<double> vectout;
    for (auto & fl : m_Results) {
        if (fl.hasTimestamp) {
            auto ftime0 = fl.timestamp;
            //double ltime0 = ftime0.sec + (double(ftime0.nsec)
            //        / 1000000000.0);
            vectout << static_cast<double>(Vamp::RealTime::realTime2Frame(ftime0, m_rate));
        }
    }
    return vectout;
}

QVector<double> VampAnalyzer::GetEndFramesVector()
{
    QVector<double> vectout;
    for (auto & fl : m_Results) {
        if (fl.hasDuration) {
            auto ftime0 = fl.timestamp;
            auto ftime1 = ftime0 + fl.duration;
            //double ltime1 = ftime1.sec + (double(ftime1.nsec)
            //        / 1000000000.0);
            vectout << static_cast<double>(
                Vamp::RealTime::realTime2Frame(ftime1, m_rate));
        }
    }
    return vectout;
}

QVector<QString> VampAnalyzer::GetLabelsVector()
{
    QVector<QString> vectout;
    for(auto & fl : m_Results)
        vectout << fl.label.c_str();
    return vectout;
}

QVector<double> VampAnalyzer::GetFirstValuesVector()
{
    QVector<double> vectout;
    for (auto fli = m_Results.begin(); fli != m_Results.end(); ++fli) {
        auto vec = fli->values;
        if (!vec.empty())
            vectout << vec[0];
    }
    return vectout;
}

QVector<double> VampAnalyzer::GetLastValuesVector()
{
    QVector<double> vectout;
    for (auto & fl : m_Results) {
        auto && vec = fl.values;
        if (!vec.empty())
            vectout << vec[vec.size() - 1];
    }
    return vectout;
}

//bool GetMeanValuesVector ( int FromOutput , QVector <double> vectout ){
//    if( FromOutput>m_iOutputs || FromOutput < 0) return false;
//        for (Vamp::Plugin::FeatureList::iterator fli =
//                        features[k].begin(); fli
//                        != features[k].end(); ++fli){
//
//        }
//
//}
//
//bool GetMinValuesVector ( int FromOutput , QVector <double> vectout ){
//    if( FromOutput>m_iOutputs || FromOutput < 0) return false;
//        for (Vamp::Plugin::FeatureList::iterator fli =
//                        features[k].begin(); fli
//                        != features[k].end(); ++fli){
//
//        }
//
//}
//
//bool GetMaxValuesVector ( int FromOutput , QVector <double> vectout ){
//    if( FromOutput>m_iOutputs || FromOutput < 0) return false;
//        for (Vamp::Plugin::FeatureList::iterator fli =
//                        features[k].begin(); fli
//                        != features[k].end(); ++fli){
//
//        }
//
//}
