/**
 * @file portmidienumerator.h
 * @author Sean Pappalardo spappalardo@mixxx.org
 * @date Thu 15 Mar 2012
 * @brief This class handles discovery and enumeration of DJ controllers that appear under the PortMIDI cross-platform API.
 */

#ifndef RTMIDIENUMERATOR_H
#define RTMIDIENUMERATOR_H

#include "controllers/midi/midienumerator.h"
#include <memory>
#include <QList>
#include <rtmidi/RtMidi.h>
class RtMidiEnumerator : public MidiEnumerator {
    Q_OBJECT
  public:
    RtMidiEnumerator();
    virtual ~RtMidiEnumerator();
    QList<Controller*> queryDevices();
  private:
    std::unique_ptr<RtMidiIn>   m_midiIn{};
    std::unique_ptr<RtMidiOut>  m_midiOut{};
    QList<Controller *> m_devices;
};
bool shouldLinkInputToOutput(const QString input_name,
                             const QString output_name);

#endif