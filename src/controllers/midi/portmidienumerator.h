/**
 * @file portmidienumerator.h
 * @author Sean Pappalardo spappalardo@mixxx.org
 * @date Thu 15 Mar 2012
 * @brief This class handles discovery and enumeration of DJ controllers that appear under the PortMIDI cross-platform API.
 */

_Pragma("once")
#include "controllers/midi/midienumerator.h"

class PortMidiEnumerator : public MidiEnumerator {
    Q_OBJECT
    static ThisFactory<PortMidiEnumerator> m_factory;
  public:
    PortMidiEnumerator();
    virtual ~PortMidiEnumerator();
    QList<Controller*> queryDevices();
  private:
    QList<Controller*> m_devices;
    static PortMidiEnumerator   s_inst;
};

// For testing.
bool shouldLinkInputToOutput(const QString input_name,const QString output_name);
