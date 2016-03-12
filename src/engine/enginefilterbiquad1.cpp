#include <stdio.h>
#include "engine/enginefilterbiquad1.h"

EngineFilterBiquad1LowShelving::EngineFilterBiquad1LowShelving(double sampleRate,double centerFreq,double Q)
:EngineFilterIIR(nullptr,5,IIR_LP, "LsBq/%1/%2")
{
    m_startFromDry = true;
    setFrequencyCorners(sampleRate, centerFreq, Q, 0);
}
void EngineFilterBiquad1LowShelving::setFrequencyCorners(double sampleRate,double centerFreq,double Q,double dBgain)
{
    auto ptr = m_spec.arg(Q).arg(dBgain);
    setCoefs(sampleRate, 5, ptr.toLocal8Bit().constData(), centerFreq);
}
EngineFilterBiquad1Peaking::EngineFilterBiquad1Peaking(double sampleRate,double centerFreq, double Q)
:EngineFilterIIR(nullptr,5,IIR_BP, "PkBq/%1/%2")
{
    m_startFromDry = true;
    setFrequencyCorners(sampleRate, centerFreq, Q, 0);
}
void EngineFilterBiquad1Peaking::setFrequencyCorners(double sampleRate,double centerFreq,double Q,double dBgain)
{
    auto ptr = m_spec.arg(Q).arg(dBgain);
    setCoefs(sampleRate, 5, ptr, centerFreq);
}
EngineFilterBiquad1HighShelving::EngineFilterBiquad1HighShelving(double sampleRate,double centerFreq,double Q)
:EngineFilterIIR(nullptr,5,IIR_HP, "HsBq/%1/%2")
{
    m_startFromDry = true;
    setFrequencyCorners(sampleRate, centerFreq, Q, 0);
}

void EngineFilterBiquad1HighShelving::setFrequencyCorners(double sampleRate,double centerFreq,double Q,double dBgain)
{
    auto ptr = m_spec.arg(Q).arg(dBgain);
    setCoefs(sampleRate, 5, ptr.toLocal8Bit().constData(), centerFreq);
}
EngineFilterBiquad1Low::EngineFilterBiquad1Low(double sampleRate,double centerFreq,double Q,bool startFromDry)
:EngineFilterIIR(nullptr,2,IIR_LP, "LpBq/%1")
{
    m_startFromDry = startFromDry;
    setFrequencyCorners(sampleRate, centerFreq, Q);
}
void EngineFilterBiquad1Low::setFrequencyCorners(double sampleRate,double centerFreq,double Q)
{
    auto ptr = m_spec.arg(Q);
    setCoefs(sampleRate, 2, ptr, centerFreq);
}
EngineFilterBiquad1Band::EngineFilterBiquad1Band(double sampleRate,double centerFreq,double Q)
:EngineFilterIIR(nullptr, 2,IIR_BP, "BpBq/%1")
{
    setFrequencyCorners(sampleRate, centerFreq, Q);
}
void EngineFilterBiquad1Band::setFrequencyCorners(double sampleRate,double centerFreq,double Q)
{
    auto ptr = m_spec.arg(Q);
    setCoefs(sampleRate, 2, ptr, centerFreq);
}
EngineFilterBiquad1High::EngineFilterBiquad1High(double sampleRate,double centerFreq,double Q,bool startFromDry)
:EngineFilterIIR(nullptr,2,IIR_HP, "HpBq/%1")
{
    m_startFromDry = startFromDry;
    setFrequencyCorners(sampleRate, centerFreq, Q);
}
void EngineFilterBiquad1High::setFrequencyCorners(double sampleRate,double centerFreq,double Q)
{
    auto ptr = m_spec.arg(Q);
    setCoefs(sampleRate, 2, ptr, centerFreq);
}
