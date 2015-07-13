#ifndef ENGINE_ENGINEFILTERLADSPA_H
#define ENGINE_ENGINEFILTERLADSPA_H
#include <ladspa.h>
#include <memory>
#include <vector>
#include <QLibrary>
#include <QMap>
#include <cmath>
#include <cfloat>
#include "engine/engineobject.h"
#include "util/math.h"
  using namespace std;
class EngineFilterLadspa: public EngineObject{
    Q_OBJECT;
public:
    typedef shared_ptr<QLibrary> LibraryPtr;
    struct PortDescriptor{
      LADSPA_PortDescriptor   m_x;
      inline bool isInput()const{return LADSPA_IS_PORT_INPUT(m_x);}
      inline bool isOutput()const{return LADSPA_IS_PORT_OUTPUT(m_x);}
      inline bool isControl()const{return LADSPA_IS_PORT_CONTROL(m_x);}
      inline bool isAudio()const{return LADSPA_IS_PORT_AUDIO(m_x);}
    };
    struct PortRangeHintDescriptor{
      LADSPA_PortRangeHintDescriptor m_x;
      inline bool isBoundedBelow()const{return LADSPA_IS_HINT_BOUNDED_BELOW(m_x);}
      inline bool isBoundedAbove()const{return LADSPA_IS_HINT_BOUNDED_ABOVE(m_x);}
      inline bool isToggled()const{return LADSPA_IS_HINT_TOGGLED(m_x);}
      inline bool isSampleRate()const{return LADSPA_IS_HINT_SAMPLE_RATE(m_x);}
      inline bool isInteger()const{return LADSPA_IS_HINT_INTEGER(m_x);}
    };
    struct LadspaPort{
      QString                 m_name;
      unsigned long           m_index;
      PortDescriptor          m_desc;
      PortRangeHintDescriptor m_hint;
      float                   m_lowerBound;
      float                   m_upperBound;
      mutable float           m_default = NAN;
      float                   m_cvalue;
      float                  *m_avalue;
      LadspaPort() = default;
      LadspaPort(const LADSPA_Descriptor *desc, unsigned long idx){
        if(!desc || idx>desc->PortCount)return;
        m_desc.m_x = desc->PortDescriptors[idx];
        m_name = QString(desc->PortNames[idx]);
        m_hint.m_x = desc->PortRangeHints[idx].HintDescriptor;
        m_lowerBound = desc->PortRangeHints[idx].LowerBound;
        m_upperBound = desc->PortRangeHints[idx].UpperBound;
        m_index = idx;
      }
      bool isOutput()const{return m_desc.isOutput();}
      bool isInput() const{return m_desc.isInput();}
      bool isControl()const{return m_desc.isControl();}
      bool isAudio()const{return m_desc.isAudio();}
      float defaultValue()const{
        if(!LADSPA_IS_HINT_HAS_DEFAULT(m_hint.m_x))return NAN;
        else if(!isnan(m_default)) return m_default;
        else if(LADSPA_IS_HINT_DEFAULT_0(m_hint.m_x))   m_default = 0;
        else if(LADSPA_IS_HINT_DEFAULT_1(m_hint.m_x))   m_default = 1;
        else if(LADSPA_IS_HINT_DEFAULT_100(m_hint.m_x)) m_default = 100;
        else if(LADSPA_IS_HINT_DEFAULT_440(m_hint.m_x)) m_default = 440;
        else if(LADSPA_IS_HINT_DEFAULT_MIDDLE(m_hint.m_x)){
          if(LADSPA_IS_HINT_LOGARITHMIC(m_hint.m_x)) m_default = std::sqrt(m_lowerBound*m_upperBound);
          else if(LADSPA_IS_HINT_INTEGER(m_hint.m_x))m_default = std::floor((m_lowerBound+m_upperBound)*0.5f);
          else                                       m_default =  (m_lowerBound+m_upperBound)*0.5f;
        }
        else if(LADSPA_IS_HINT_DEFAULT_LOW(m_hint.m_x)){
          if(LADSPA_IS_HINT_LOGARITHMIC(m_hint.m_x)) m_default  = std::exp(0.75f*std::log(m_lowerBound)+0.25*std::log(m_upperBound));
          else if(LADSPA_IS_HINT_INTEGER(m_hint.m_x))m_default = std::floor(0.75f*m_lowerBound+0.25f*m_upperBound);
          else                                       m_default  =  0.75f*m_lowerBound+0.25f*m_upperBound;
        }
        else if(LADSPA_IS_HINT_DEFAULT_HIGH(m_hint.m_x)){
          if(LADSPA_IS_HINT_LOGARITHMIC(m_hint.m_x)) m_default = std::exp(0.25f*std::log(m_lowerBound)+0.75*std::log(m_upperBound));
          else if(LADSPA_IS_HINT_INTEGER(m_hint.m_x))m_default = std::floor(0.25f*m_lowerBound+0.75f*m_upperBound);
          else                                       m_default =  0.25f*m_lowerBound+0.75f*m_upperBound;
        }
        else if(LADSPA_IS_HINT_DEFAULT_MINIMUM(m_hint.m_x))m_default = m_lowerBound;
        else if(LADSPA_IS_HINT_DEFAULT_MAXIMUM(m_hint.m_x))m_default = m_upperBound;
        return m_default;
      }
      float constrain(float val)const{
        if(LADSPA_IS_HINT_BOUNDED_BELOW(m_hint.m_x)) val=std::max(val,m_lowerBound);
        if(LADSPA_IS_HINT_BOUNDED_ABOVE(m_hint.m_x)) val=std::min(val,m_upperBound);
        if(LADSPA_IS_HINT_INTEGER(m_hint.m_x))       val=std::round(val);
        if(LADSPA_IS_HINT_TOGGLED(m_hint.m_x))       val=!m_cvalue;
        return val;
      }
    };
private:
  Q_PROPERTY(QString filename READ filename CONSTANT);
  Q_PROPERTY(QString label READ label CONSTANT);
  Q_PROPERTY(int index READ index CONSTANT);
  Q_PROPERTY(quint64 sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged);
protected:
  const QString m_filename;
  const QString m_label;
  int           m_index = -1;
  bool          m_multi_channel = false;
  QLibrary      m_library;
  const LADSPA_Descriptor *m_descriptor = nullptr;
  void                    *m_handle_l = nullptr;
  void                    *m_handle_r = nullptr;
  QMap<QString,LadspaPort> m_ports_l;
  QMap<QString,LadspaPort> m_ports_r;
  quint64                  m_sampleRate = 44100;
  bool                     m_activated = false;
  virtual void          instantiate()
  {
    if(m_descriptor && !m_handle_l){m_handle_l = m_descriptor->instantiate(m_descriptor,m_sampleRate);}
    if(!m_multi_channel && m_descriptor && !m_handle_r){m_handle_r = m_descriptor->instantiate(m_descriptor,m_sampleRate);}
  }
  virtual void          cleanup()
  {
    if(m_descriptor && m_handle_l){deactivate();m_descriptor->cleanup(m_handle_l);m_handle_l=nullptr;}
    if(m_descriptor && m_handle_r){deactivate();m_descriptor->cleanup(m_handle_r);m_handle_l=nullptr;}
  }
  virtual void          activate()
  {
    if(m_descriptor && m_handle_l && !m_activated){m_descriptor->activate(m_handle_l);}
    if(m_descriptor && m_handle_r && !m_activated){m_descriptor->activate(m_handle_r);}
    m_activated = true;
  }
  virtual void        deactivate()
  {
    if(m_descriptor && m_handle_l &&  m_activated){m_descriptor->deactivate(m_handle_l);}
    if(m_descriptor && m_handle_r &&  m_activated){m_descriptor->deactivate(m_handle_r);}
    m_activated = false;
  }
signals:
  void sampleRateChanged(quint64);
public:
  virtual const QString &filename()const{return m_filename;}
  virtual const QString &label()const{return m_label;}
  virtual int            index()const{return m_index;}
  virtual quint64        sampleRate()const{return m_sampleRate;}
  virtual void           setSampleRate(quint64 rate);
  explicit EngineFilterLadspa(const QString &filename, const QString &label, QObject *pParent);
  virtual ~EngineFilterLadspa(){cleanup();}
  virtual void process(const CSAMPLE *pIn, CSAMPLE *pOut,const int iBufferSize);
};
#endif
