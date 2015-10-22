/**
 * @file dlgprefsound.cpp
 * @author Bill Good <bkgood at gmail dot com>
 * @date 20100625
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QtDebug>
#include <QMessageBox>
#include <QMetaEnum>
#include "dlgprefsound.h"
#include "dlgprefsounditem.h"
#include "engine/enginebuffer.h"
#include "engine/enginemaster.h"
#include "playermanager.h"
#include "soundmanager.h"
#include "sounddevice.h"
#include "util/rlimit.h"
#include "controlobject.h"

/**
 * Construct a new sound preferences pane. Initializes and populates all the
 * all the controls to the values obtained from SoundManager.
 */
DlgPrefSound::DlgPrefSound(QWidget* pParent, SoundManager* pSoundManager,PlayerManager* pPlayerManager, ConfigObject<ConfigValue>* pConfig)
        : DlgPreferencePage(pParent),
          m_pSoundManager(pSoundManager),
          m_pPlayerManager(pPlayerManager),
          m_pConfig(pConfig),
          m_settingsModified(false),
          m_loading(false)
{
    setupUi(this);
    connect(m_pSoundManager,&SoundManager::devicesUpdated,this,&DlgPrefSound::refreshDevices);
    apiComboBox->clear();
    apiComboBox->addItem(tr("None"), "None");
    updateAPIs();
    connect(apiComboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(apiChanged(int)));
    sampleRateComboBox->clear();
    for(auto srate: m_pSoundManager->getSampleRates())
    {
        if (srate > 0) sampleRateComboBox->addItem(tr("%1 Hz").arg(srate), srate);
    }
    connect(sampleRateComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [&](auto i){m_config.setSampleRate(sampleRateComboBox->currentData().toInt());});
    connect(sampleRateComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &DlgPrefSound::updateAudioBufferSizes);
    connect(audioBufferComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [&](auto i){m_config.setAudioBufferSizeIndex(i + 1);});
    deviceSyncComboBox->clear();
    deviceSyncComboBox->addItem(tr("Default (long delay)"),QVariant{1});
    deviceSyncComboBox->addItem(tr("Experimental (no delay)"),QVariant{2});
    deviceSyncComboBox->setCurrentIndex(1);
    connect(deviceSyncComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(syncBuffersChanged(int)));

    keylockComboBox->clear();
    auto keylockEnum = QMetaEnum::fromType<EngineBuffer::KeylockEngine>();
    for (int i = 0; i < keylockEnum.keyCount(); ++i)
    {
      keylockComboBox->addItem(QString(keylockEnum.key(i)),keylockEnum.value(i));
    }
    initializePaths();
    loadSettings();
    connect(apiComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(settingChanged()));
    connect(sampleRateComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(settingChanged()));
    connect(audioBufferComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(settingChanged()));
    connect(deviceSyncComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(settingChanged()));
    connect(keylockComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(settingChanged()));

    connect(queryButton, SIGNAL(clicked()),
        this, SLOT(queryClicked()));

    connect(m_pSoundManager, SIGNAL(outputRegistered(AudioOutput, AudioSource*)),
        this, SLOT(addPath(AudioOutput)));
    connect(m_pSoundManager, SIGNAL(outputRegistered(AudioOutput, AudioSource*)),
        this, SLOT(loadSettings()));

    connect(m_pSoundManager, SIGNAL(inputRegistered(AudioInput, AudioDestination*)),
        this, SLOT(addPath(AudioInput)));
    
    connect(m_pSoundManager, SIGNAL(inputRegistered(AudioInput, AudioDestination*)),
        this, SLOT(loadSettings()));

    m_pMasterAudioLatencyOverloadCount = new ControlObject(ConfigKey("Master", "audio_latency_overload_count"), this);
    m_pMasterAudioLatencyOverloadCount->connectValueChanged(SLOT(bufferUnderflow(double)));
    m_pMasterLatency = new ControlObject(ConfigKey("Master", "latency"), this);
    m_pMasterLatency->connectValueChanged(SLOT(masterLatencyChanged(double)));
    m_pHeadDelay = new ControlObject(ConfigKey("Master", "headDelay"), this);
    m_pMasterDelay = new ControlObject(ConfigKey("Master", "delay"), this);
    headDelaySpinBox->setValue(m_pHeadDelay->get());
    masterDelaySpinBox->setValue(m_pMasterDelay->get());
    m_pMasterEnabled = new ControlObject(ConfigKey("Master", "enabled"), this);
    masterMixComboBox->addItem(tr("Disabled"));
    masterMixComboBox->addItem(tr("Enabled"));
    masterMixComboBox->setCurrentIndex(m_pMasterEnabled->get() ? 1 : 0);
    connect(masterMixComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        m_pMasterEnabled ,static_cast<void(ControlObject::*)(double)>(&ControlObject::set));
    connect(m_pMasterEnabled,&ControlObject::valueChanged,
        [=](auto val){masterMixComboBox->setCurrentIndex( val>0);});
    m_pMasterMonoMixdown = new ControlObject(ConfigKey("Master", "mono_mixdown"), this);
    masterOutputModeComboBox->addItem(tr("Stereo"));
    masterOutputModeComboBox->addItem(tr("Mono"));
    masterOutputModeComboBox->setCurrentIndex(m_pMasterMonoMixdown->get() ? 1 : 0);
    connect(masterOutputModeComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        m_pMasterMonoMixdown,static_cast<void(ControlObject::*)(double)>(&ControlObject::set));
    connect(m_pMasterMonoMixdown,&ControlObject::valueChanged,
        masterOutputModeComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::setCurrentIndex));
    m_pMasterTalkoverMix = new ControlObject(ConfigKey("Master", "talkover_mix"), this);
    micMixComboBox->addItem(tr("Master output"));
    micMixComboBox->addItem(tr("Broadcast and Recording only"));
    micMixComboBox->setCurrentIndex(m_pMasterTalkoverMix->get());
    connect(micMixComboBox,static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),m_pMasterTalkoverMix,
        static_cast<void(ControlObject::*)(double)>(&ControlObject::set));
    connect(m_pMasterTalkoverMix,&ControlObject::valueChanged, [=](auto val){micMixComboBox->setCurrentIndex(val?1:0);});
    m_pKeylockEngine = new ControlObject(ConfigKey("Master", "keylock_engine"), this);
    connect(headDelaySpinBox,static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
        m_pHeadDelay,static_cast<void(ControlObject::*)(double)>(&ControlObject::set));
    connect(masterDelaySpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
        m_pMasterDelay,static_cast<void(ControlObject::*)(double)>(&ControlObject::set));
#ifdef __LINUX__
    qDebug() << "RLimit Cur " << RLimit::getCurRtPrio();
    qDebug() << "RLimit Max " << RLimit::getMaxRtPrio();
    if (RLimit::isRtPrioAllowed()) {limitsHint->hide();}
#else
    // the limits warning is a Linux only thing
    limitsHint->hide();
#endif // __LINUX__

}
DlgPrefSound::~DlgPrefSound() = default;
/**
 * Slot called when the preferences dialog  is opened or this pane is
 * selected.
 */
void DlgPrefSound::slotUpdate() {
    // this is unfortunate, because slotUpdate is called every time
    // we change to this pane, we lose changed and unapplied settings
    // every time. There's no real way around this, just anothe argument
    // for a prefs rewrite -- bkgood
    loadSettings();
    m_settingsModified = false;
}
/**
 * Slot called when the Apply or OK button is pressed.
 */
void DlgPrefSound::slotApply() {
    if (!m_settingsModified) {return;}
    m_pKeylockEngine->set(keylockComboBox->currentIndex());
    m_pConfig->set(ConfigKey("Master", "keylock_engine"),ConfigValue(keylockComboBox->currentIndex()));
    m_config.clearInputs();
    m_config.clearOutputs();
    emit(writePaths(&m_config));
    int err = m_pSoundManager->setConfig(m_config);
    if (err != OK) {
        QString error;
        QString deviceName(tr("a device"));
        QString detailedError(tr("An unknown error occurred"));
        auto device = m_pSoundManager->getErrorDevice();
        if (device ) {
            deviceName = tr("sound device \"%1\"").arg(device->getDisplayName());
            detailedError = device->getError();
        }
        switch (err) {
        case SOUNDDEVICE_ERROR_DUPLICATE_OUTPUT_CHANNEL:
            error = tr("Two outputs cannot share channels on %1").arg(deviceName);
            break;
        default:
            error = tr("Error opening %1\n%2").arg(deviceName, detailedError);
            break;
        }
        QMessageBox::warning(NULL, tr("Configuration error"), error);
    }
    m_settingsModified = false;
    loadSettings(); // in case SM decided to change anything it didn't like
}
/**
 * Initializes (and creates) all the path items. Each path item widget allows
 * the user to input a sound device name and channel number given a description
 * of what will be done with that info. Inputs and outputs are grouped by tab,
 * and each path item has an identifier (Master, Headphones, ...) and an index,
 * if necessary.
 */
void DlgPrefSound::initializePaths()
{
    for(auto out: m_pSoundManager->registeredOutputs()) addPath(out);
    for(auto in: m_pSoundManager->registeredInputs()) addPath(in);
}
void DlgPrefSound::addPath(AudioOutput output)
{
    // if we already know about this output, don't make a new entry
    auto _children = outputTab->findChildren<DlgPrefSoundItem*>(QString{});
    for(auto item: _children)
    {
        if (item->type() == output.getType())
        {
            if (AudioPath::isIndexed(item->type()))
            {
                if (item->index() == output.getIndex()) return;
            } else return;
        }
    }
    DlgPrefSoundItem *toInsert;
    auto type = output.getType();
    if (AudioPath::isIndexed(type)) toInsert = new DlgPrefSoundItem(outputTab, type,m_outputDevices, false, output.getIndex());
    else                            toInsert = new DlgPrefSoundItem(outputTab, type,m_outputDevices, false);
    connect(this,&DlgPrefSound::refreshOutputDevices,toInsert,&DlgPrefSoundItem::refreshDevices);
    insertItem(toInsert, outputVLayout);
    connectSoundItem(toInsert);
}

void DlgPrefSound::addPath(AudioInput input)
{
    DlgPrefSoundItem *toInsert;
    // if we already know about this input, don't make a new entry
    auto _children = inputTab->findChildren<DlgPrefSoundItem*>(QString{});
    for(auto item: _children )
    {
        if (item->type() == input.getType())
        {
            if (AudioPath::isIndexed(item->type()))
            {
                if (item->index() == input.getIndex()) return;
            }
            else return;
        }
    }
    auto type = input.getType();
    if (AudioPath::isIndexed(type)) toInsert = new DlgPrefSoundItem(inputTab, type,m_inputDevices, true, input.getIndex());
    else  toInsert = new DlgPrefSoundItem(inputTab, type,m_inputDevices, true);
    connect(this, &DlgPrefSound::refreshInputDevices
        ,toInsert, &DlgPrefSoundItem::refreshDevices);
    insertItem(toInsert, inputVLayout);
    connectSoundItem(toInsert);
}
void DlgPrefSound::connectSoundItem(DlgPrefSoundItem *item)
{
    connect(item, SIGNAL(settingChanged()),
        this, SLOT(settingChanged()));
    connect(this, SIGNAL(loadPaths(const SoundManagerConfig&)),
        item, SLOT(loadPath(const SoundManagerConfig&)));
    connect(this, SIGNAL(writePaths(SoundManagerConfig*)),
        item, SLOT(writePath(SoundManagerConfig*)));
    connect(this, SIGNAL(updatingAPI()),
        item, SLOT(save()));
    connect(this, SIGNAL(updatedAPI()),
        item, SLOT(reload()));
}
void DlgPrefSound::insertItem(DlgPrefSoundItem *pItem, QVBoxLayout *pLayout)
{
    int pos;
    for (pos = 0; pos < pLayout->count() - 1; ++pos)
    {
        auto pOther(qobject_cast<DlgPrefSoundItem*>(pLayout->itemAt(pos)->widget()));
        if (!pOther) continue;
        if (pItem->type() < pOther->type()) break;
        else if (pItem->type() == pOther->type() && AudioPath::isIndexed(pItem->type()) && pItem->index() < pOther->index()) break;
    }
    pLayout->insertWidget(pos, pItem);
}

/**
 * Convenience overload to load settings from the SoundManagerConfig owned by
 * SoundManager.
 */
void DlgPrefSound::loadSettings() {loadSettings(m_pSoundManager->getConfig());}

/**
 * Loads the settings in the given SoundManagerConfig into the dialog.
 */
void DlgPrefSound::loadSettings(const SoundManagerConfig &config) {
    m_loading = true; // so settingsChanged ignores all our modifications here
    m_config = config;
    auto apiIndex = apiComboBox->findData(m_config.getAPI());
    if (apiIndex != -1) {apiComboBox->setCurrentIndex(apiIndex);}
    auto sampleRateIndex = sampleRateComboBox->findData(m_config.getSampleRate());
    if (sampleRateIndex != -1) {
        sampleRateComboBox->setCurrentIndex(sampleRateIndex);
        if (audioBufferComboBox->count() <= 0) {
            updateAudioBufferSizes(sampleRateIndex); // so the latency combo box is
            // sure to be populated, if setCurrentIndex is called with the
            // currentIndex, the currentIndexChanged signal won't fire and
            // the updateLatencies slot won't run -- bkgood lp bug 689373
        }
    }
    auto sizeIndex = audioBufferComboBox->findData(m_config.getAudioBufferSizeIndex());
    if (sizeIndex != -1) {audioBufferComboBox->setCurrentIndex(sizeIndex);}
    auto syncBuffers = m_config.getSyncBuffers();
    auto syncIndex   = deviceSyncComboBox->findData(QVariant(syncBuffers));
    if ( syncIndex != -1 ) { deviceSyncComboBox->setCurrentIndex(syncIndex);}
    auto  keylock_engine = m_pConfig->getValueString(ConfigKey("Master", "keylock_engine"), "1").toInt();
    keylockComboBox->setCurrentIndex(keylock_engine);
    emit(loadPaths(m_config));
    m_loading = false;
}

/**
 * Slot called when the user selects a different API, or the
 * software changes it programatically (for instance, when it
 * loads a value from SoundManager). Refreshes the device lists
 * for the new API and pushes those to the path items.
 */
void DlgPrefSound::apiChanged(int index)
{
    m_config.setAPI(apiComboBox->itemData(index).toString());
    refreshDevices();
    // JACK sets its own latency
    if (m_config.getAPI() == MIXXX_PORTAUDIO_JACK_STRING)
    {
        latencyLabel->setEnabled(false);
        audioBufferComboBox->setEnabled(false);
    }
    else
    {
        latencyLabel->setEnabled(true);
        audioBufferComboBox->setEnabled(true);
    }
}
/**
 * Updates the list of APIs, trying to keep the API and device selections
 * constant if possible.
 */
void DlgPrefSound::updateAPIs()
{
    auto currentAPI = apiComboBox->itemData(apiComboBox->currentIndex()).toString();
    emit(updatingAPI());
    while (apiComboBox->count() > 1) { apiComboBox->removeItem(apiComboBox->count() - 1);}
    for(auto api: m_pSoundManager->getHostAPIList()) {apiComboBox->addItem(api, api);}
    int newIndex = apiComboBox->findData(currentAPI);
    if (newIndex > -1) {apiComboBox->setCurrentIndex(newIndex);}
    emit(updatedAPI());
}
/**
 * Slot called when the latency combo box is changed to update the
 * latency in the config.
 */
void DlgPrefSound::syncBuffersChanged(int )
{
    auto data = deviceSyncComboBox->currentData();
    auto ok   = false;
    auto val  = data.toInt(&ok);
    if(ok) m_config.setSyncBuffers(val);
}
// Slot called whenever the selected sample rate is changed. Populates the
// audio buffer input box with SMConfig::kMaxLatency values, starting at 1ms,
// representing a number of frames per buffer, which will always be a power
// of 2 (so the values displayed in ms won't be constant between sample rates,
// but they'll be close).
void DlgPrefSound::updateAudioBufferSizes(int sampleRateIndex)
{
    auto sampleRate = sampleRateComboBox->itemData(sampleRateIndex).toDouble();
    auto oldSizeIndex = audioBufferComboBox->currentIndex();
    auto framesPerBuffer = 1; // start this at 0 and inf loop happens
    // we don't want to display any sub-1ms buffer sizes (well maybe we do but I
    // don't right now!), so we iterate over all the buffer sizes until we
    // find the first that gives us a buffer size >= 1 ms -- bkgood
    // no div-by-0 in the next line because we don't allow srates of 0 in our
    // srate list when we construct it in the ctor -- bkgood
    for (; framesPerBuffer / sampleRate * 1000 < 1.0; framesPerBuffer *= 2)
    {
    }
    audioBufferComboBox->clear();
    for (auto i = 0; i < SoundManagerConfig::kMaxAudioBufferSizeIndex; ++i)
    {
        auto  latency = framesPerBuffer * 1e3 / sampleRate ;
        // i + 1 in the next line is a latency index as described in SSConfig
        audioBufferComboBox->addItem(tr("%1 ms").arg(latency,0,'g',3), i + 1);
        framesPerBuffer <<= 1; // *= 2
    }
    if (oldSizeIndex < audioBufferComboBox->count() && oldSizeIndex >= 0) audioBufferComboBox->setCurrentIndex(oldSizeIndex);
    else audioBufferComboBox->setCurrentIndex(audioBufferComboBox->count() - 1);
}
/**
 * Slot called when device lists go bad to refresh them, or the API
 * just changes and we need to display new devices.
 */
void DlgPrefSound::refreshDevices()
{
    if (m_config.getAPI() == "None")
    {
        m_outputDevices.clear();
        m_inputDevices.clear();
    }
    else
    {
        m_outputDevices = m_pSoundManager->getDeviceList(m_config.getAPI(), true, false);
        m_inputDevices  = m_pSoundManager->getDeviceList(m_config.getAPI(), false, true);
    }
    emit(refreshOutputDevices(m_outputDevices));
    emit(refreshInputDevices(m_inputDevices));
}

/**
 * Called when any of the combo boxes in this dialog are changed. Enables the
 * apply button and marks that settings have been changed so that
 * DlgPrefSound::slotApply knows to apply them.
 */
void DlgPrefSound::settingChanged()
{
    if (m_loading) return; // doesn't count if we're just loading prefs
    m_settingsModified = true;
}
/**
 * Slot called when the "Query Devices" button is clicked.
 */
void DlgPrefSound::queryClicked()
{
    m_pSoundManager->queryDevices();
    updateAPIs();
}
/**
 * Slot called when the "Reset to Defaults" button is clicked.
 */
void DlgPrefSound::slotResetToDefaults()
{
    SoundManagerConfig newConfig;
    newConfig.loadDefaults(m_pSoundManager, SoundManagerConfig::ALL);
    loadSettings(newConfig);
    {
      auto keylockEnum = QMetaEnum::fromType<EngineBuffer::KeylockEngine>();
      keylockComboBox->setCurrentIndex( keylockComboBox->findData(static_cast<int>(EngineBuffer::KeylockEngine::RubberBand)));
      m_pKeylockEngine->set(static_cast<int>(EngineBuffer::KeylockEngine::RubberBand));
    }
    masterMixComboBox->setCurrentIndex(1);
    m_pMasterEnabled->set(1.0);

    masterDelaySpinBox->setValue(0.0);
    m_pMasterDelay->set(0.0);

    headDelaySpinBox->setValue(0.0);
    m_pHeadDelay->set(0.0);

    // Enable talkover master output
    m_pMasterTalkoverMix->set(0.0);
    micMixComboBox->setCurrentIndex(0);

    settingChanged(); // force the apply button to enable
}
void DlgPrefSound::bufferUnderflow(double count)
{
    bufferUnderflowCount->setText(QString::number(count));
    update();
}
void DlgPrefSound::masterLatencyChanged(double latency)
{
    currentLatency->setText(QString("%1 ms").arg(latency));
    update();
}
