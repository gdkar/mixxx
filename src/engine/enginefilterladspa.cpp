#include "engine/enginefilterladspa.h"

EngineFilterLadspa::EngineFilterLadspa(const QString &filename, const QString &_label, QObject *pParent)
    : EngineObject(pParent)
    , m_filename(filename)
    , m_label(_label)
    , m_library(filename){
  LADSPA_Descriptor_Function ladspa_descriptor = (LADSPA_Descriptor_Function)m_library.resolve("ladspa_descriptor");
  const LADSPA_Descriptor *desc=nullptr;
  unsigned long idx = 0;
  for(;(desc=ladspa_descriptor(idx));idx++)
  {
    if(m_label == QString(desc->Label))
    {
      m_index = idx;
      m_descriptor = desc;
      break;
    }
  }
  if(m_index>=0)
  {
    for(idx=0; idx<m_descriptor->PortCount;idx++)
    {
      m_ports_r[QString(m_descriptor->PortNames[idx])]=(LadspaPort(m_descriptor,idx));
      m_ports_l[QString(m_descriptor->PortNames[idx])]=(LadspaPort(m_descriptor,idx));
    }
  }
}
void EngineFilterLadspa::setSampleRate(quint64 rate)
{
  if(rate!=m_sampleRate)
  {
    m_sampleRate=rate;
    if(m_handle_l || m_handle_r){
      cleanup();
      instantiate();
    }
    emit sampleRateChanged(rate);
  }
}
void EngineFilterLadspa::process(const CSAMPLE *pIn, CSAMPLE *pOut, const int iBufferSize)
{
  instantiate();
  activate   ();
  auto data_in_l = (CSAMPLE*)alloca(iBufferSize*sizeof(CSAMPLE));
  auto data_in_r = (CSAMPLE*)alloca(iBufferSize*sizeof(CSAMPLE));
  auto data_out_l = (CSAMPLE*)alloca(iBufferSize*sizeof(CSAMPLE));
  auto data_out_r = (CSAMPLE*)alloca(iBufferSize*sizeof(CSAMPLE));
  for(int i = 0; i < iBufferSize; i++){
    data_in_l[i] = pIn[2*i+0];
    data_in_r[i] = pIn[2*i+1];
  }
  for(auto &&port : m_ports_l)
  {
    if(port.isControl())
    {
      m_descriptor->connect_port(m_handle_l,port.m_index,&port.m_cvalue);
    }else{
      if(m_multi_channel)
      {
        if(port.isInput() && port.m_name.endsWith('L'))
        {
          m_descriptor->connect_port(m_handle_l,port.m_index,data_in_l);
        }
        else if(port.isOutput() && port.m_name.endsWith("L"))
        {
          m_descriptor->connect_port(m_handle_l,port.m_index,data_out_l);
        }
        else if(port.isInput() && port.m_name.endsWith("R"))
        {
          m_descriptor->connect_port(m_handle_l,port.m_index,data_in_r);
        }
        else if(port.isOutput() && port.m_name.endsWith("R"))
        {
          m_descriptor->connect_port(m_handle_l,port.m_index,data_out_r);
        }
      }else{
        if(port.isInput())
        {
          m_descriptor->connect_port(m_handle_l,port.m_index,data_in_l);
        }else{
          m_descriptor->connect_port(m_handle_l,port.m_index,data_out_l);
        }
      }
    }
  }
  if(!m_multi_channel){
    for(auto &&port : m_ports_r)
    {
      if(port.isControl())
      {
        m_descriptor->connect_port(m_handle_r,port.m_index,&port.m_cvalue);
      }else{
        if(port.isInput())
        {
          m_descriptor->connect_port(m_handle_r,port.m_index,data_in_r);
        }else{
          m_descriptor->connect_port(m_handle_r,port.m_index,data_out_r);
        }
      }
    }
  }
  m_descriptor->run(m_handle_l,iBufferSize);
  if(!m_multi_channel)
    m_descriptor->run(m_handle_r,iBufferSize);
  for(int i = 0; i < iBufferSize; i++)
  {
    pOut[2*i+0] = data_out_l[i];
    pOut[2*i+1] = data_out_r[i];
  }
}
