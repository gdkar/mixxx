/**
 * @file dlgprefsounditem.cpp
 * @author Bill Good <bkgood at gmail dot com>
 * @date 20100704
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QPoint>

#include "dlgprefsounditem.h"
#include "sounddevice.h"
#include "soundmanagerconfig.h"

/**
 * Constructs a new preferences sound item, representing an AudioPath and SoundDevice
 * with a label and two combo boxes.
 * @param type The AudioPathType of the path to be represented
 * @param devices The list of devices for the user to choose from (either a collection
 * of input or output devices).
 * @param isInput true if this is representing an AudioInput, false otherwise
 * @param index the index of the represented AudioPath, if applicable
 */
DlgPrefSoundItem::DlgPrefSoundItem(QWidget *parent, AudioPathType type,
                                   QList<SoundDevice*> &devices, bool isInput,
                                   unsigned int index)
        : QWidget(parent),
          m_type(type),
          m_index(index),
          m_devices(devices),
          m_isInput(isInput)
{
    setupUi(this);
    typeLabel->setText(AudioPath::getTrStringFromType(type, index));
    deviceComboBox->addItem(tr("None"), "None");
    connect(deviceComboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(deviceChanged(int)));
    connect(channelComboBox, SIGNAL(currentIndexChanged(int)),this, SIGNAL(settingChanged()));
    refreshDevices(m_devices);
}
DlgPrefSoundItem::~DlgPrefSoundItem() = default;
/**
 * Slot called when the parent preferences pane updates its list of sound
 * devices, to update the item widget's list of devices to display.
 */
void DlgPrefSoundItem::refreshDevices(const QList<SoundDevice*> &devices)
{
    m_devices = devices;
    auto oldDev = deviceComboBox->itemData(deviceComboBox->currentIndex()).toString();
    deviceComboBox->setCurrentIndex(0);
    // not using combobox->clear means we can leave in "None" so it
    // doesn't flicker when you switch APIs... cleaner Mixxx :) bkgood
    while (deviceComboBox->count() > 1) deviceComboBox->removeItem(deviceComboBox->count() - 1);
    for(auto device: m_devices)
    {
        if (!hasSufficientChannels(device)) continue;
        deviceComboBox->addItem(device->getDisplayName(), device->getInternalName());
    }
    int newIndex = deviceComboBox->findData(oldDev);
    if (newIndex != -1) deviceComboBox->setCurrentIndex(newIndex);
}
/**
 * Slot called when the device combo box selection changes. Updates the channel
 * combo box.
 */
void DlgPrefSoundItem::deviceChanged(int index)
{
    channelComboBox->clear();
    auto selection = deviceComboBox->itemData(index).toString();
    auto numChannels = 0;
    if (selection == "None")
    {
        goto emitAndReturn;
    }
    else
    {
        for(auto device: m_devices)
        {
            if (device->getInternalName() == selection)
            {
                if (m_isInput) numChannels = device->getNumInputChannels();
                else numChannels = device->getNumOutputChannels();
            }
        }
    }
    if (numChannels == 0)
    {
        goto emitAndReturn;
    }
    else
    {
        auto minChannelsForType = AudioPath::minChannelsForType(m_type);
        auto maxChannelsForType = AudioPath::maxChannelsForType(m_type);
        // Count down from the max so that stereo channels are first.
        for (int channelsForType = maxChannelsForType; channelsForType >= minChannelsForType; --channelsForType)
        {
            for (auto i = 1; i + (channelsForType - 1) <= numChannels; i += channelsForType)
            {
                auto channelString = QString{};
                if (channelsForType == 1) channelString = tr("Channel %1").arg(i);
                else channelString = tr("Channels %1 - %2").arg(QString::number(i),QString::number(i + channelsForType - 1));
                // Because QComboBox supports QPoint natively (via QVariant) we
                // use a QPoint to store the channel info. x is the channel base
                // and y is the channel count. We use i - 1 because the channel
                // base is 0-indexed.
                channelComboBox->addItem(channelString, QPoint(i - 1, channelsForType));
            }
        }
    }
emitAndReturn:
    emit(settingChanged());
}

/**
 * Slot called to load the respective AudioPath from a SoundManagerConfig
 * object.
 * @note If there are multiple AudioPaths matching this instance's type
 *       and index (if applicable), then only the first one is used. A more
 *       advanced preferences pane may one day allow multiples.
 */
void DlgPrefSoundItem::loadPath(const SoundManagerConfig &config)
{
    if (m_isInput)
    {
        auto inputs = config.getInputs();
        for(auto devName: inputs.uniqueKeys())
        {
            for(auto in: inputs.values(devName))
            {
                if (in.getType() == m_type && in.getIndex() == m_index)
                {
                    setDevice(devName);
                    setChannel(in.getChannelGroup().getChannelBase(),in.getChannelGroup().getChannelCount());
                    return; // we're just using the first one found, leave
                            // multiples to a more advanced dialog -- bkgood
                }
            }
        }
    }
    else
    {
        auto  outputs = config.getOutputs();
        for(auto devName: outputs.uniqueKeys())
        {
            for(auto out: outputs.values(devName))
            {
                if (out.getType() == m_type && out.getIndex() == m_index)
                {
                    setDevice(devName);
                    setChannel(out.getChannelGroup().getChannelBase(),
                               out.getChannelGroup().getChannelCount());
                    return; // we're just using the first one found, leave
                            // multiples to a more advanced dialog -- bkgood
                }
            }
        }
    }
    // if we've gotten here without returning, we didn't find a path applicable
    // to us so set some defaults -- bkgood
    setDevice("None"); // this will blank the channel combo box
}

/**
 * Slot called when the underlying DlgPrefSound wants this Item to
 * record its respective path with the SoundManagerConfig instance at
 * config.
 */
void DlgPrefSoundItem::writePath(SoundManagerConfig *config) const
{
    if (auto device = getDevice())
    {
        // Because QComboBox supports QPoint natively (via QVariant) we use a QPoint
        // to store the channel info. x is the channel base and y is the channel
        // count.
        auto channelData = channelComboBox->itemData( channelComboBox->currentIndex()).toPoint();
        auto channelBase = channelData.x();
        auto channelCount = channelData.y();
        if (m_isInput)
        {
            config->addInput(device->getInternalName(),AudioInput(m_type, channelBase, channelCount, m_index));
        }
        else
        {
            config->addOutput(device->getInternalName(),AudioOutput(m_type, channelBase, channelCount, m_index));
        }
    }
}

/**
 * Slot called to tell the Item to save its selections for later use.
 */
void DlgPrefSoundItem::save()
{
    m_savedDevice = deviceComboBox->itemData(deviceComboBox->currentIndex()).toString();
    m_savedChannel = channelComboBox->itemData(channelComboBox->currentIndex()).toPoint();
}

/**
 * Slot called to reload Item with previously saved settings.
 */
void DlgPrefSoundItem::reload()
{
    auto newDevice = deviceComboBox->findData(m_savedDevice);
    if (newDevice > -1) deviceComboBox->setCurrentIndex(newDevice);
    auto newChannel = channelComboBox->findData(m_savedChannel);
    if (newChannel > -1) channelComboBox->setCurrentIndex(newChannel);
}

/**
 * Gets the currently selected SoundDevice
 * @returns pointer to SoundDevice, or NULL if the "None" option is selected.
 */
SoundDevice* DlgPrefSoundItem::getDevice() const
{
    auto selection = deviceComboBox->itemData(deviceComboBox->currentIndex()).toString();
    if (selection == "None") return nullptr;
    for(auto device: m_devices)
    {
        if (selection == device->getInternalName()) return device;
    }
    // looks like something became invalid ???
    deviceComboBox->setCurrentIndex(0); // set it to none
    return nullptr;
}
/**
 * Selects a device in the device combo box given a SoundDevice
 * internal name, or selects "None" if the device isn't found.
 */
void DlgPrefSoundItem::setDevice(const QString &deviceName)
{
    auto index = deviceComboBox->findData(deviceName);
    if (index != -1) deviceComboBox->setCurrentIndex(index);
    else deviceComboBox->setCurrentIndex(deviceComboBox->findData("None")); 
}
/**
 * Selects a channel in the channel combo box given a channel number,
 * or selects the first channel if the given channel isn't found.
 */
void DlgPrefSoundItem::setChannel(unsigned int channelBase,unsigned int channels)
{
    // Because QComboBox supports QPoint natively (via QVariant) we use a QPoint
    // to store the channel info. x is the channel base and y is the channel
    // count.
    auto index = channelComboBox->findData(QPoint(channelBase, channels));
    if (index != -1) channelComboBox->setCurrentIndex(index);
    else channelComboBox->setCurrentIndex(0);
}
/**
 * Checks that a given device can act as a source/input for our type.
 */
int DlgPrefSoundItem::hasSufficientChannels(const SoundDevice *device) const
{
    auto needed = AudioPath::minChannelsForType(m_type);
    if (m_isInput) return device->getNumInputChannels() >= needed;
    else return device->getNumOutputChannels() >= needed;
}
