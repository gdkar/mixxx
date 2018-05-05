/**
* @file portmidienumerator.cpp
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Thu 15 Mar 2012
* @brief This class handles discovery and enumeration of DJ controller devices that appear under the PortMIDI cross-platform API.
*/

#include <RtMidi.h>
#include <QRegExp>
#include <utility>
#include <map>

#include "controllers/midi/rtmidienumerator.h"
#include "controllers/midi/rtmidicontroller.h"
#include "util/cmdlineargs.h"

RtMidiEnumerator::RtMidiEnumerator(QObject *p)
: MidiEnumerator(p)
{
    try {
        m_midiIn = std::make_unique<RtMidiIn>();
        m_midiOut = std::make_unique<RtMidiOut>();
    }
    catch(const RtMidiError &error) {
        qWarning() << "RtMidiError" << error.what() ;
    }
    catch(...) {
        qWarning() << "unknown error!";
    }
}
RtMidiEnumerator::~RtMidiEnumerator()
{
    qDebug() << "Deleting RtMidi devices...";
    for(auto & dev : m_devices) {
        try {
            delete dev;
        }
        catch(const RtMidiError &error) {
            qWarning() << "RtMidiError" << error.what() ;
        }
        catch(...) {
            qWarning() << "unknown error!";
        }
        dev = nullptr;
    }
    m_devices.clear();
}
/** Enumerate the MIDI devices
  * This method needs a bit of intelligence because PortMidi (and the underlying MIDI APIs) like to split
  * output and input into separate devices. Eg. PortMidi would tell us the Hercules is two half-duplex devices.
  * To help simplify a lot of code, we're going to aggregate these two streams into a single full-duplex device.
  */
QList<Controller*> RtMidiEnumerator::queryDevices()
{
    qDebug() << "Scanning PortMIDI devices:";
    if(!m_midiIn || !m_midiOut)
        return QList<Controller*>{};
    for(auto & dev : m_devices) {
        delete dev;
        dev = nullptr;
    }
    try {
        m_devices.clear();
        auto num_inputs = int(m_midiIn->getPortCount());
        auto num_outputs= int(m_midiOut->getPortCount());

        auto inputDevIndex = -1;
        auto outputDevIndex = -1;

        std::map<int, std::string> unassignedOutputDevices;

        // Build a complete list of output devices for later pairing
        for (int i = 0; i < num_outputs; i++) {
            auto deviceName = m_midiOut->getPortName(i);
            qDebug() << " Found output device" << "#" << i << deviceName.c_str();
            unassignedOutputDevices[i] = deviceName;
        }
        // Search for input devices and pair them with output devices if applicable
        for (int i = 0; i < num_inputs; i++) {
            auto deviceName = m_midiIn->getPortName(i);
            qDebug() << " Found input device" << "#" << i << deviceName.c_str();
            inputDevIndex = i;
            //Reset our output device variables before we look for one incase we find none.
            outputDevIndex = -1;
            auto outputName = std::string{};
            for(auto it = unassignedOutputDevices.begin(); it != unassignedOutputDevices.end(); ++it) {
                if(shouldLinkInputToOutput(QString::fromStdString(deviceName), QString::fromStdString(it->second))) {
                    outputDevIndex = it->first;
                    outputName = it->second;
                    it = unassignedOutputDevices.erase(it);
                    qDebug() << "    Linking to output device #" << outputDevIndex << outputName.c_str();
                    break;
                }
            }
            auto currentDevice = new RtMidiController(inputDevIndex, QString::fromStdString(deviceName), outputDevIndex, QString::fromStdString(outputName));
            m_devices.push_back(currentDevice);
        }
        for(auto it = unassignedOutputDevices.begin(); it != unassignedOutputDevices.end(); ++it) {
            m_devices.push_back(new RtMidiController(-1, QString{},it->first, QString::fromStdString(it->second)));
        }
    }
    catch(const RtMidiError &error) {
        qWarning() << "RtMidiError" << error.what() ;
    }
    catch(...) {
        qWarning() << "unknown error!";
    }

    return m_devices;
}
