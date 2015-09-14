#include <QtDebug>

#include "waveform/waveform.h"
#include "proto/waveform.pb.h"

using namespace mixxx::track;
const int kNumChannels = 2;

// Return the smallest power of 2 which is greater than the desired size when
// squared.
namespace{
  int computeTextureStride(int size) {
      auto stride = 256;
      while (stride * stride < size) {stride *= 2;}
      return stride;
  }
};
WaveformData::WaveformData() = default;
WaveformData::WaveformData(int i):m_i(i){}
Waveform::Waveform(const QByteArray data)
        : m_id{-1},
          m_bDirty(true),
          m_dataSize(0),
          m_visualSampleRate(0),
          m_audioVisualRatio(0),
          m_textureStride(computeTextureStride(0)),
          m_completion(-1) {
    readByteArray(data);
}
Waveform::Waveform(int audioSampleRate, int audioSamples,int desiredVisualSampleRate, int maxVisualSamples)
        : m_id{-1},
          m_bDirty(true),
          m_dataSize(0),
          m_visualSampleRate(0),
          m_audioVisualRatio(0),
          m_textureStride(1024),
          m_completion(-1) {
    int numberOfVisualSamples = 0;
    if (audioSampleRate > 0) {
        if (maxVisualSamples == -1) {
            // Waveform
            if (desiredVisualSampleRate < audioSampleRate) {
                m_visualSampleRate.store(static_cast<double>(desiredVisualSampleRate));
            } else {
                m_visualSampleRate.store(static_cast<double>(audioSampleRate));
            }
        } else {
            // Waveform Summary (Overview)
            if (audioSamples > maxVisualSamples) {
                m_visualSampleRate.store((double)maxVisualSamples *(double)audioSampleRate / (double)audioSamples);
            } else {
                m_visualSampleRate.store(audioSampleRate);
            }
        }
        m_audioVisualRatio.store((double)audioSampleRate / (double)m_visualSampleRate.load());
        numberOfVisualSamples = (audioSamples / m_audioVisualRatio.load()) + 1;
        numberOfVisualSamples += numberOfVisualSamples%2;
    }
    assign(numberOfVisualSamples, 0);
    setCompletion(0);
}
Waveform::~Waveform() = default;
int     Waveform::getId()const{return m_id.load();}
void    Waveform::setId(int id){m_id.store(id);}
QString Waveform::getVersion()const{return m_version;}
void    Waveform::setVersion(QString version){m_version.swap(version);}
QString Waveform::getDescription()const{return m_description;}
void    Waveform::setDescription(QString desc){m_description.swap(desc);}
bool    Waveform::isValid()const{return getDataSize()>0&&getVisualSampleRate()>0;}
bool    Waveform::isDirty()const{return m_bDirty.load();}
void    Waveform::setDirty(bool d)const{m_bDirty.store(d);}
double  Waveform::getAudioVisualRatio()const{return m_audioVisualRatio.load();}
double  Waveform::getCompletion()const{return m_completion.load();}
void    Waveform::setCompletion(double c){m_completion.store(c);}
int     Waveform::getTextureStride()const{return m_textureStride;}
int     Waveform::getTextureSize()const{return m_data.size();}
int     Waveform::getDataSize()const{return m_dataSize.load();}
WaveformData * Waveform::data(){return &m_data[0];}
const WaveformData * Waveform::data()const{return &m_data[0];}
WaveformData &Waveform::at(int i){return m_data[i];}
unsigned char &Waveform::low(int i){return m_data[i].filtered.low;}
unsigned char &Waveform::mid(int i){return m_data[i].filtered.mid;}
unsigned char &Waveform::high(int i){return m_data[i].filtered.high;}
unsigned char &Waveform::all(int i){return m_data[i].filtered.all;}
unsigned char Waveform::getAll(int i)const{return m_data[i].filtered.all;}
unsigned char Waveform::getLow(int i)const{return m_data[i].filtered.low;}
unsigned char Waveform::getMid(int i)const{return m_data[i].filtered.mid;}
unsigned char Waveform::getHigh(int i)const{return m_data[i].filtered.high;}
double Waveform::getVisualSampleRate()const{return m_visualSampleRate.load();}
QByteArray Waveform::toByteArray() const {
    io::Waveform waveform;
    waveform.set_visual_sample_rate(m_visualSampleRate.load());
    waveform.set_audio_visual_ratio(m_audioVisualRatio.load());
    io::Waveform::Signal* all = waveform.mutable_signal_all();
    io::Waveform::FilteredSignal* filtered = waveform.mutable_signal_filtered();
    // TODO(rryan) get the actual cutoff values from analyserwaveform.cpp so
    // that if they change we don't have to remember to update these.
    // Frequency cutoffs for butterworth filters:
    // filtered->set_low_cutoff_frequency(200);
    // filtered->set_mid_low_cutoff_frequency(200);
    // filtered->set_mid_high_cutoff_frequency(2000);
    // filtered->set_high_cutoff_frequency(2000);
    // Frequency cutoff for bessel_lowpass4
    filtered->set_low_cutoff_frequency(600);
    // Frequency cutoff for bessel_bandpass
    filtered->set_mid_low_cutoff_frequency(600);
    filtered->set_mid_high_cutoff_frequency(4000);
    // Frequency cutoff for bessel_highpass4
    filtered->set_high_cutoff_frequency(4000);
    auto low = filtered->mutable_low();
    auto mid = filtered->mutable_mid();
    auto high = filtered->mutable_high();
    // TODO(vrince) set max/min for each signal
    auto numChannels = kNumChannels;
    all->set_units(io::Waveform::RMS);
    all->set_channels(numChannels);
    low->set_units(io::Waveform::RMS);
    low->set_channels(numChannels);
    mid->set_units(io::Waveform::RMS);
    mid->set_channels(numChannels);
    high->set_units(io::Waveform::RMS);
    high->set_channels(numChannels);
    auto dataSize = getDataSize();
    for (auto i = 0; i < dataSize; ++i) {
        const auto& datum = m_data.at(i);
        all->add_value(datum.filtered.all);
        low->add_value(datum.filtered.low);
        mid->add_value(datum.filtered.mid);
        high->add_value(datum.filtered.high);
    }
    qDebug() << "Writing waveform from byte array:"
             << "dataSize" << dataSize
             << "allSignalSize" << all->value_size()
             << "visualSampleRate" << waveform.visual_sample_rate()
             << "audioVisualRatio" << waveform.audio_visual_ratio();
    std::string output;
    waveform.SerializeToString(&output);
    return QByteArray(output.data(), output.length());
}
void Waveform::readByteArray(const QByteArray& data) {
    if (data.isNull()) {return;}
    io::Waveform waveform;
    if (!waveform.ParseFromArray(data.constData(), data.size())) {
        qDebug() << "ERROR: Could not parse Waveform from QByteArray of size "
                 << data.size();
        return;
    }
    if (!waveform.has_visual_sample_rate() ||
        !waveform.has_audio_visual_ratio() ||
        !waveform.has_signal_all() ||
        !waveform.has_signal_filtered() ||
        !waveform.signal_filtered().has_low() ||
        !waveform.signal_filtered().has_mid() ||
        !waveform.signal_filtered().has_high()) {
        qDebug() << "ERROR: Waveform proto is missing key data. Skipping.";
        return;
    }
    const auto& all = waveform.signal_all();
    const auto& low = waveform.signal_filtered().low();
    const auto& mid = waveform.signal_filtered().mid();
    const auto& high = waveform.signal_filtered().high();
    qDebug() << "Reading waveform from byte array:"
             << "allSignalSize" << all.value_size()
             << "visualSampleRate" << waveform.visual_sample_rate()
             << "audioVisualRatio" << waveform.audio_visual_ratio();
    resize(all.value_size());
    auto dataSize = getDataSize();
    if (all.value_size() != dataSize) {
        qDebug() << "ERROR: Couldn't resize Waveform to" << all.value_size()
                 << "while reading.";
        resize(0);
        return;
    }
    m_visualSampleRate.store(waveform.visual_sample_rate());
    m_audioVisualRatio.store(waveform.audio_visual_ratio());
    if (low.value_size() != dataSize ||
        mid.value_size() != dataSize ||
        high.value_size() != dataSize) {
        qDebug() << "WARNING: Filtered data size does not match all-signal size.";
    }
    // TODO(XXX) If non-RMS, convert but since we only save RMS today we can add
    // this later.
    auto low_valid = low.units() == io::Waveform::RMS;
    auto mid_valid = mid.units() == io::Waveform::RMS;
    auto high_valid = high.units() == io::Waveform::RMS;
    for (auto i = 0; i < dataSize; ++i) {
        m_data[i].filtered.all = static_cast<unsigned char>(all.value(i));
        bool use_low = low_valid && i < low.value_size();
        bool use_mid = mid_valid && i < mid.value_size();
        bool use_high = high_valid && i < high.value_size();
        m_data[i].filtered.low = use_low ? static_cast<unsigned char>(low.value(i)) : 0;
        m_data[i].filtered.mid = use_mid ? static_cast<unsigned char>(mid.value(i)) : 0;
        m_data[i].filtered.high = use_high ? static_cast<unsigned char>(high.value(i)) : 0;
    }
    m_completion.store(dataSize);
    m_bDirty.store(false);
}
void Waveform::resize(int size) {
    m_dataSize.store(size);
    m_textureStride.store(computeTextureStride(size));
    m_data.resize(m_textureStride.load() * m_textureStride.load());
    m_bDirty.store(true);
}
void Waveform::assign(int size, int value) {
    m_dataSize.store(size);
    m_textureStride.store(computeTextureStride(size));
    m_data.assign(m_textureStride.load() * m_textureStride.load(), value);
    m_bDirty.store(true);
}
void Waveform::dump() const {
    qDebug() << "Waveform" << this
             << "size("+QString::number(getDataSize())+")"
             << "textureStride("+QString::number(m_textureStride.load())+")"
             << "completion("+QString::number(getCompletion())+")"
             << "visualSampleRate("+QString::number(m_visualSampleRate.load())+")"
             << "audioVisualRatio("+QString::number(m_audioVisualRatio.load())+")";
}
