#include <rubberband/RubberBandStretcher.h>
#include "engine/scaledreader.h"

#include "util/sample.h"
#include "sources/audiosource.h"

ScaledReader::ScaledReader(QObject *pParent)
        : QObject(pParent)
        , m_rb(std::make_unique<RubberBand::RubberBandStretcher>(
            m_sampleRate,
            2,
            RubberBand::RubberBandStretcher::OptionProcessRealTime))
{
    m_rb->setTimeRatio(8.0);
    m_rb->setTimeRatio(1./8.);
    m_rb->setTimeRatio(1.0);
}
void ScaledReader::setPosition(double _position)
{
    if(!m_tempoRatio)
        return;
    auto sample_target = _position / m_baseRate;
    if(reverse()) {
        m_src_pos = std::min<SINT>(
            sample_target + kForwardBlockSize
          , m_audioSource->getFrameCount()
            );
    }else{
        m_src_pos = m_audioSource->seekSampleFrame(
            std::max<SINT>(
                0
              , sample_target - kForwardBlockSize
                )
            );
    }
    m_position = m_src_pos * m_baseRate;
    m_output_buf.clear();
    m_rb->reset();
    skipSampleFrames((_position - m_position) / m_tempoRatio);
    emit positionChanged(position());
}
SINT ScaledReader::readSampleFrames(SINT count, CSAMPLE *data)
{
    auto nread = SINT{0};
    while(nread < count) {
        if(m_output_buf.size()) {
            auto chunk = std::min<SINT>(count,m_output_buf.size() / 2);
            auto mid = std::next(m_output_buf.begin(),chunk * 2);
            std::copy(m_output_buf.begin(),mid, data + (2*nread));
            m_output_buf.erase(m_output_buf.begin(),mid);
            m_position += chunk * m_tempoRatio;
            nread += chunk;
            continue;
        }
        if(!m_rb->available() && !decode_next())
                break;
        retrieve();
    }
    return nread;
}
SINT ScaledReader::skipSampleFrames(SINT count)
{
    auto nread = SINT{0};
    while(nread < count) {
        auto chunk = retrieve_drop(count - nread);
        nread += chunk;
        if(!chunk && !decode_next())
            break;
    }
    return nread;
}
void ScaledReader::setAudioSource(mixxx::AudioSourcePointer _src)
{
    if(_src == m_audioSource)
        return;
    m_audioSource = _src;
    clear();
    emit audioSourceChanged(_src);
    setSampleRate(sampleRate());
}
void ScaledReader::setSampleRate(SINT _rate)
{
    if(_rate != m_sampleRate) {
        m_sampleRate = _rate;
        if(m_audioSource) {
            m_baseRate = m_sampleRate * 1. / m_audioSource->getSamplingRate();
        }else{
            m_baseRate = 1.;
        }
        setScaleParameters(pitchRatio(),tempoRatio(),position());
    }
}
SINT ScaledReader::retrieve_drop(SINT count)
{
    auto dropped = std::min<SINT>(m_output_buf.size() / 2, count);
    if(dropped) {
        m_output_buf.erase(m_output_buf.begin(),std::next(m_output_buf.begin(),dropped * 2));
        m_position += dropped * m_tempoRatio;
    }
    while(dropped < count) {
        auto available = m_rb->available();
        if(!available)
            break;
        auto chunk = m_rb->retrieve(m_retrieve_buf.data(), std::min<SINT>({SINT(m_retrieve_storage.size() / 2), available,count - dropped}));
        dropped += chunk;
        m_position += chunk * m_tempoRatio;
        if(!chunk)
            break;
    }
    return dropped;
}

bool ScaledReader::retrieve()
{
    auto success = false;
    while(auto available = m_rb->available()) {
        auto chunk = m_rb->retrieve(m_retrieve_buf.data(), std::min<SINT>(m_retrieve_storage.size() / 2, available));
        if(!chunk)
            break;
        success = true;
        for(auto i = 0u; i < chunk; ++i ) {
            m_output_buf.push_back(m_retrieve_buf[0][i]);
            m_output_buf.push_back(m_retrieve_buf[1][i]);
        }
    }
    return success;
}

void ScaledReader::setScaleParameters(
    double pitch_ratio,
    double tempo_ratio,
    double pos)
{
    auto pitch_changed = pitch_ratio != pitchRatio();
    auto tempo_changed = tempo_ratio != tempoRatio();
    auto reverse_changed = tempoRatio() * tempo_ratio < 0;
    auto kMinSeekSpeed  = 1. / 1024;
    auto absolute_tempo = std::abs(tempo_ratio);
    auto time_ratio_inverse = 1.;
    if(absolute_tempo < kMinSeekSpeed) {
        if(m_tempoRatio) {
            m_tempoRatio = 0;
            emit(tempoRatioChanged(0));
        }
        if(m_position != pos) {
            m_position   = pos;
            emit(positionChanged(pos));
        }
        if(m_pitchRatio != pitch_ratio) {
            m_pitchRatio = pitch_ratio;
            emit(pitchRatioChanged(m_pitchRatio));
        }
        return;
    }else {
        m_rb->reset();
        m_rb->setTimeRatio(1. / time_ratio_inverse);
        if(!m_rb->getInputIncrement()) {
            while(!m_rb->getInputIncrement()) {
                time_ratio_inverse += 0.0001;
                m_rb->setTimeRatio(1./time_ratio_inverse);
            }
        }
        m_tempoRatio = std::copysign(time_ratio_inverse / m_baseRate, tempo_ratio);
    }
    m_pitchRatio = pitch_ratio;
    if(auto pitchScale = std::abs(m_baseRate * pitch_ratio)) {
        m_rb->setPitchScale(pitchScale);
    }
    setPosition(pos);
    if(pitch_changed)
        emit pitchRatioChanged(pitchRatio());
    if(tempo_changed)
        emit tempoRatioChanged(tempoRatio());
    if(reverse_changed)
        emit reverseChanged(reverse());
}

bool ScaledReader::decode_next()
{
    if(!m_audioSource)
        return false;
    if(reverse()) {
        auto target = std::min<SINT>(m_src_pos, kReverseBlockSize);
        auto _src_pos = m_audioSource->seekSampleFrame(m_src_pos - target);
        target = m_src_pos - _src_pos;
        m_src_pos = _src_pos;
        m_decode_buf.resize(target * 2);
        auto nread = m_audioSource->readSampleFrames(target, m_decode_buf.data());
        if(nread < target)
            return false;
        m_planar_storage.resize(m_decode_buf.size());
        m_planar_buf = std::vector<CSAMPLE*>{ m_planar_storage.data(), m_planar_storage.data() + target};
        SampleUtil::deinterleaveBuffer(m_planar_buf[1], m_planar_buf[0], m_decode_buf.data(), target);
        std::reverse(m_planar_storage.begin(),m_planar_storage.end());
        m_rb->process(m_planar_buf.data(), target, false);
    }else{
        auto target = std::min<SINT>(kForwardBlockSize, m_audioSource->getFrameCount() - m_src_pos);
        m_decode_buf.resize(target * 2);
        auto nread = m_audioSource->readSampleFrames(target, m_decode_buf.data());
        m_src_pos += nread;
        if(!nread)
            return false;
        m_planar_storage.resize(nread);
        m_planar_buf = std::vector<CSAMPLE*>{ m_planar_storage.data(), m_planar_storage.data() + nread};
        SampleUtil::deinterleaveBuffer(m_planar_buf[0], m_planar_buf[1], m_decode_buf.data(), nread);
        m_rb->process(m_planar_buf.data(), nread, false);
    }
    return true;
}
void ScaledReader::clear()
{
    m_rb->reset();
    m_output_buf.clear();
}
void ScaledReader::setTempoRatio(double tr)
{
    setScaleParameters(tr, pitchRatio(), position());
}
void ScaledReader::setPitchRatio(double pr)
{
    setScaleParameters(tempoRatio(), pr, position());
}
SINT ScaledReader::block_size() const
{
    return reverse() ? kReverseBlockSize : kForwardBlockSize;
}
mixxx::AudioSourcePointer ScaledReader::audioSource() const
{
    return m_audioSource;
}
ScaledReader::~ScaledReader() = default;
