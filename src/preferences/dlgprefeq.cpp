/***************************************************************************
                          dlgprefeq.cpp  -  description
                             -------------------
    begin                : Thu Jun 7 2007
    copyright            : (C) 2007 by John Sully
    email                : jsully@scs.ryerson.ca
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QWidget>
#include <QString>
#include <QHBoxLayout>

#include "dlgprefeq.h"
#include "engine/enginefilterbessel4.h"
#include "controlobject.h"
#include "controlobjectslave.h"
#include "util/math.h"
#include "playermanager.h"
#include "effects/effectrack.h"

const char* kConfigKey = "Mixer Profile";
const char* kEnableEqs = "EnableEQs";
const char* kEqsOnly = "EQsOnly";
const char* kSingleEq = "SingleEQEffect";
const char* kDefaultEqId = "org.mixxx.effects.bessel8lvmixeq";
const char* kDefaultMasterEqId = "none";
const char* kDefaultQuickEffectId = "org.mixxx.effects.filter";

const int kFrequencyUpperLimit = 20050;
const int kFrequencyLowerLimit = 16;

DlgPrefEQ::DlgPrefEQ(QWidget* pParent, EffectsManager* pEffectsManager,
                     ConfigObject<ConfigValue>* pConfig)
        : DlgPreferencePage(pParent),
          m_COLoFreq(new ControlObjectSlave(kConfigKey, "LoEQFrequency",this)),
          m_COHiFreq(new ControlObjectSlave(kConfigKey, "HiEQFrequency",this)),
          m_pConfig(pConfig),
          m_lowEqFreq(0.0),
          m_highEqFreq(0.0),
          m_pEffectsManager(pEffectsManager),
          m_firstSelectorLabel(NULL),
          m_pNumDecks(NULL),
          m_inSlotPopulateDeckEffectSelectors(false),
          m_bEqAutoReset(false) {
    m_pEQEffectRack = m_pEffectsManager->getEqualizerRack(0);
    m_pQuickEffectRack = m_pEffectsManager->getQuickEffectRack(0);
    setupUi(this);
    // Connection
    connect(SliderHiEQ, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateHiEQ()));
    connect(SliderHiEQ, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateHiEQ()));
    connect(SliderHiEQ, SIGNAL(sliderReleased()), this, SLOT(slotUpdateHiEQ()));

    connect(SliderLoEQ, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateLoEQ()));
    connect(SliderLoEQ, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateLoEQ()));
    connect(SliderLoEQ, SIGNAL(sliderReleased()), this, SLOT(slotUpdateLoEQ()));

    connect(CheckBoxEqAutoReset, SIGNAL(stateChanged(int)), this, SLOT(slotUpdateEqAutoReset(int)));
    connect(CheckBoxBypass, SIGNAL(stateChanged(int)), this, SLOT(slotBypass(int)));
    connect(CheckBoxEqOnly, SIGNAL(stateChanged(int)),this, SLOT(slotPopulateDeckEffectSelectors()));

    connect(CheckBoxSingleEqEffect, SIGNAL(stateChanged(int)),this, SLOT(slotSingleEqChecked(int)));

    // Add drop down lists for current decks and connect num_decks control
    // to slotNumDecksChanged
    m_pNumDecks = new ControlObjectSlave("Master", "num_decks", this);
    m_pNumDecks->connectValueChanged(SLOT(slotNumDecksChanged(double)));
    slotNumDecksChanged(m_pNumDecks->get());
    setUpMasterEQ();
    loadSettings();
    slotUpdate();
    slotApply();
}

DlgPrefEQ::~DlgPrefEQ() {
    qDeleteAll(m_deckEqEffectSelectors);
    m_deckEqEffectSelectors.clear();

    qDeleteAll(m_deckQuickEffectSelectors);
    m_deckQuickEffectSelectors.clear();

    qDeleteAll(m_filterWaveformEnableCOs);
    m_filterWaveformEnableCOs.clear();
}

void DlgPrefEQ::slotNumDecksChanged(double numDecks) {
    int oldDecks = m_deckEqEffectSelectors.size();
    while (m_deckEqEffectSelectors.size() < static_cast<int>(numDecks)) {
        int deckNo = m_deckEqEffectSelectors.size() + 1;
        auto label = new QLabel(QObject::tr("Deck %1 EQ Effect").arg(deckNo), this);
        auto group = PlayerManager::groupForDeck(m_deckEqEffectSelectors.size());
        m_filterWaveformEnableCOs.append(new ControlObject(ConfigKey(group, "filterWaveformEnable")));
        m_filterWaveformEffectLoaded.append(false);
        // Create the drop down list for EQs
        auto eqComboBox = new QComboBox(this);
        m_deckEqEffectSelectors.append(eqComboBox);
        connect(eqComboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(slotEqEffectChangedOnDeck(int)));
        // Create the drop down list for EQs
        auto quickEffectComboBox = new QComboBox(this);
        m_deckQuickEffectSelectors.append(quickEffectComboBox);
        connect(quickEffectComboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(slotQuickEffectChangedOnDeck(int)));
        if (deckNo == 1) {
            m_firstSelectorLabel = label;
            if (CheckBoxEqOnly->isChecked()) {
                m_firstSelectorLabel->setText(QObject::tr("EQ Effect"));
            }
        }

        // Setup the GUI
        gridLayout_3->addWidget(label, deckNo, 0);
        gridLayout_3->addWidget(eqComboBox, deckNo, 1);
        gridLayout_3->addWidget(quickEffectComboBox, deckNo, 2);
        gridLayout_3->addItem(new QSpacerItem(
                40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum),
                deckNo, 3, 1, 1);
    }
    slotPopulateDeckEffectSelectors();
    for (int i = oldDecks; i < static_cast<int>(numDecks); ++i) {
        // Set the configured effect for box and simpleBox or Bessel8 LV-Mix EQ
        // if none is configured
        QString group = PlayerManager::groupForDeck(i);
        QString configuredEffect = m_pConfig->getValueString(ConfigKey(kConfigKey,
                "EffectForGroup_" + group), kDefaultEqId);
        int selectedEffectIndex = m_deckEqEffectSelectors[i]->findData(configuredEffect);
        if (selectedEffectIndex < 0) {
            selectedEffectIndex = m_deckEqEffectSelectors[i]->findData(kDefaultEqId);
            configuredEffect = kDefaultEqId;
        }
        m_deckEqEffectSelectors[i]->setCurrentIndex(selectedEffectIndex);
        m_filterWaveformEffectLoaded[i] = m_pEffectsManager->isEQ(configuredEffect);
        m_filterWaveformEnableCOs[i]->set(
                m_filterWaveformEffectLoaded[i] &&
                !CheckBoxBypass->checkState());

        QString configuredQuickEffect = m_pConfig->getValueString(ConfigKey(kConfigKey,
                "QuickEffectForGroup_" + group), kDefaultQuickEffectId);
        int selectedQuickEffectIndex =
                m_deckQuickEffectSelectors[i]->findData(configuredQuickEffect);
        if (configuredQuickEffect < 0) {
            configuredQuickEffect =
                    m_deckEqEffectSelectors[i]->findData(kDefaultQuickEffectId);
            configuredEffect = kDefaultQuickEffectId;
        }
        m_deckQuickEffectSelectors[i]->setCurrentIndex(selectedQuickEffectIndex);
    }
    applySelections();
    slotSingleEqChecked(CheckBoxSingleEqEffect->isChecked());
}

static bool isMixingEQ(EffectManifest* pManifest) {
    return pManifest->isMixingEQ();
}

static bool isMasterEQ(EffectManifest* pManifest) {
    return pManifest->isMasterEQ();
}

static bool isForFilterKnob(EffectManifest* pManifest) {
    return pManifest->isForFilterKnob();
}

void DlgPrefEQ::slotPopulateDeckEffectSelectors() {
    m_inSlotPopulateDeckEffectSelectors = true; // prevents a recursive call

    QList<QPair<QString, QString> > availableEQEffectNames;
    QList<QPair<QString, QString> > availableQuickEffectNames;
    EffectsManager::EffectManifestFilterFnc filterEQ;
    EffectsManager::EffectManifestFilterFnc filterFilter;
    if (CheckBoxEqOnly->isChecked()) {
        m_pConfig->set(ConfigKey(kConfigKey, kEqsOnly), QString("yes"));
        filterEQ = isMixingEQ;
        filterFilter = isForFilterKnob;
    } else {
        m_pConfig->set(ConfigKey(kConfigKey, kEqsOnly), QString("no"));
        filterEQ = NULL; // take all;
        filterFilter = NULL;
    }
    availableEQEffectNames =
            m_pEffectsManager->getEffectNamesFiltered(filterEQ);
    availableEQEffectNames.append(QPair<QString,QString>("none", tr("None")));
    availableQuickEffectNames =
            m_pEffectsManager->getEffectNamesFiltered(filterFilter);
    availableQuickEffectNames.append(QPair<QString,QString>("none", tr("None")));

    for(auto box: m_deckEqEffectSelectors) {
        // Populate comboboxes with all available effects
        // Save current selection
        auto selectedEffectId = box->itemData(box->currentIndex()).toString();
        auto selectedEffectName = box->itemText(box->currentIndex());
        box->clear();
        auto currentIndex = -1;// Nothing selected
        auto i = 0;
        for (; i < availableEQEffectNames.size(); ++i) {
            box->addItem(availableEQEffectNames[i].second);
            box->setItemData(i, QVariant(availableEQEffectNames[i].first));
            if (selectedEffectId == availableEQEffectNames[i].first) {currentIndex = i;}
        }
        if (currentIndex < 0 && !selectedEffectName.isEmpty()) {
            // current selection is not part of the new list
            // So we need to add it
            box->addItem(selectedEffectName);
            box->setItemData(i, QVariant(selectedEffectId));
            currentIndex = i;
        }
        box->setCurrentIndex(currentIndex);
    }
    for(auto box: m_deckQuickEffectSelectors) {
        // Populate comboboxes with all available effects
        // Save current selection
        auto selectedEffectId = box->itemData(box->currentIndex()).toString();
        auto selectedEffectName = box->itemText(box->currentIndex());
        box->clear();
        auto currentIndex = -1;// Nothing selected
        auto i = 0;
        for (; i < availableQuickEffectNames.size(); ++i) {
            box->addItem(availableQuickEffectNames[i].second);
            box->setItemData(i, QVariant(availableQuickEffectNames[i].first));
            if (selectedEffectId == availableQuickEffectNames[i].first) {currentIndex = i;}
        }
        if (currentIndex < 0 && !selectedEffectName.isEmpty()) {
            // current selection is not part of the new list
            // So we need to add it
            box->addItem(selectedEffectName);
            box->setItemData(i, QVariant(selectedEffectId));
            currentIndex = i;
        }
        box->setCurrentIndex(currentIndex);
    }
    m_inSlotPopulateDeckEffectSelectors = false;
}

void DlgPrefEQ::slotSingleEqChecked(int checked) {
    auto do_hide = static_cast<bool>(checked);
    m_pConfig->set(ConfigKey(kConfigKey, kSingleEq),do_hide ? QString("yes") : QString("no"));
    for (int i = 2; i < m_deckEqEffectSelectors.size() + 1; ++i) {
        if (do_hide) {
            gridLayout_3->itemAtPosition(i, 0)->widget()->hide();
            gridLayout_3->itemAtPosition(i, 1)->widget()->hide();
            gridLayout_3->itemAtPosition(i, 2)->widget()->hide();
        } else {
            gridLayout_3->itemAtPosition(i, 0)->widget()->show();
            gridLayout_3->itemAtPosition(i, 1)->widget()->show();
            gridLayout_3->itemAtPosition(i, 2)->widget()->show();
        }
    }
    if (m_firstSelectorLabel) {
        if (do_hide) {
            m_firstSelectorLabel->setText(QObject::tr("EQ Effect"));
        } else {
            m_firstSelectorLabel->setText(QObject::tr("Deck 1 EQ Effect"));
        }
    }
    applySelections();
}
void DlgPrefEQ::loadSettings() {
    auto highEqCourse = m_pConfig->getValueString(ConfigKey(kConfigKey, "HiEQFrequency"));
    auto highEqPrecise = m_pConfig->getValueString(ConfigKey(kConfigKey, "HiEQFrequencyPrecise"));
    auto lowEqCourse = m_pConfig->getValueString(ConfigKey(kConfigKey, "LoEQFrequency"));
    auto lowEqPrecise = m_pConfig->getValueString(ConfigKey(kConfigKey, "LoEQFrequencyPrecise"));
    m_bEqAutoReset = static_cast<bool>(m_pConfig->getValueString(ConfigKey(kConfigKey, "EqAutoReset")).toInt());
    CheckBoxEqAutoReset->setChecked(m_bEqAutoReset);
    CheckBoxBypass->setChecked(m_pConfig->getValueString(ConfigKey(kConfigKey, kEnableEqs), QString("yes")) == QString("no"));
    CheckBoxEqOnly->setChecked(m_pConfig->getValueString(ConfigKey(kConfigKey, kEqsOnly), "yes") == "yes");
    CheckBoxSingleEqEffect->setChecked(m_pConfig->getValueString(ConfigKey(kConfigKey, kSingleEq), "yes") == "yes");
    slotSingleEqChecked(CheckBoxSingleEqEffect->isChecked());
    auto lowEqFreq = 0.0;
    auto highEqFreq = 0.0;
    // Precise takes precedence over course.
    lowEqFreq = lowEqCourse.isEmpty() ? lowEqFreq : lowEqCourse.toDouble();
    lowEqFreq = lowEqPrecise.isEmpty() ? lowEqFreq : lowEqPrecise.toDouble();
    highEqFreq = highEqCourse.isEmpty() ? highEqFreq : highEqCourse.toDouble();
    highEqFreq = highEqPrecise.isEmpty() ? highEqFreq : highEqPrecise.toDouble();
    if (lowEqFreq == 0.0 || highEqFreq == 0.0 || lowEqFreq == highEqFreq) {
        setDefaultShelves();
        lowEqFreq = m_pConfig->getValueString(ConfigKey(kConfigKey, "LoEQFrequencyPrecise")).toDouble();
        highEqFreq = m_pConfig->getValueString(ConfigKey(kConfigKey, "HiEQFrequencyPrecise")).toDouble();
    }
    SliderHiEQ->setValue(getSliderPosition(highEqFreq,SliderHiEQ->minimum(),SliderHiEQ->maximum()));
    SliderLoEQ->setValue(getSliderPosition(lowEqFreq,SliderLoEQ->minimum(),SliderLoEQ->maximum()));
    if (m_pConfig->getValueString(ConfigKey(kConfigKey, kEnableEqs), "yes") == QString("yes")) {
        CheckBoxBypass->setChecked(false);
    }
}
void DlgPrefEQ::setDefaultShelves()
{
    m_pConfig->set(ConfigKey(kConfigKey, "HiEQFrequency"), ConfigValue(2500));
    m_pConfig->set(ConfigKey(kConfigKey, "LoEQFrequency"), ConfigValue(250));
    m_pConfig->set(ConfigKey(kConfigKey, "HiEQFrequencyPrecise"), ConfigValue(2500.0));
    m_pConfig->set(ConfigKey(kConfigKey, "LoEQFrequencyPrecise"), ConfigValue(250.0));
}
void DlgPrefEQ::slotResetToDefaults() {
    slotMasterEQToDefault();
    setDefaultShelves();
    for(auto pCombo: m_deckEqEffectSelectors) {pCombo->setCurrentIndex(pCombo->findData(kDefaultEqId));}
    for(auto pCombo: m_deckQuickEffectSelectors) {pCombo->setCurrentIndex(pCombo->findData(kDefaultQuickEffectId));}
    loadSettings();
    CheckBoxBypass->setChecked(Qt::Unchecked);
    CheckBoxEqOnly->setChecked(Qt::Checked);
    CheckBoxSingleEqEffect->setChecked(Qt::Checked);
    m_bEqAutoReset = false;
    CheckBoxEqAutoReset->setChecked(Qt::Unchecked);
    slotUpdate();
    slotApply();
}
void DlgPrefEQ::slotEqEffectChangedOnDeck(int effectIndex) {
    QComboBox* c = qobject_cast<QComboBox*>(sender());
    // Check if qobject_cast was successful
    if (c && !m_inSlotPopulateDeckEffectSelectors) {
        int deckNumber = m_deckEqEffectSelectors.indexOf(c);
        QString effectId = c->itemData(effectIndex).toString();

        // If we are in single-effect mode and the first effect was changed,
        // change the others as well.
        if (deckNumber == 0 && CheckBoxSingleEqEffect->isChecked()) {
            for (int otherDeck = 1;
                    otherDeck < static_cast<int>(m_pNumDecks->get());
                    ++otherDeck) {
                QComboBox* box = m_deckEqEffectSelectors[otherDeck];
                box->setCurrentIndex(effectIndex);
            }
        }

        // This is required to remove a previous selected effect that does not
        // fit to the current ShowAllEffects checkbox
        slotPopulateDeckEffectSelectors();
    }
}

void DlgPrefEQ::slotQuickEffectChangedOnDeck(int effectIndex) {
    QComboBox* c = qobject_cast<QComboBox*>(sender());
    // Check if qobject_cast was successful
    if (c && !m_inSlotPopulateDeckEffectSelectors) {
        int deckNumber = m_deckQuickEffectSelectors.indexOf(c);
        QString effectId = c->itemData(effectIndex).toString();

        // If we are in single-effect mode and the first effect was changed,
        // change the others as well.
        if (deckNumber == 0 && CheckBoxSingleEqEffect->isChecked()) {
            for (int otherDeck = 1;
                    otherDeck < static_cast<int>(m_pNumDecks->get());
                    ++otherDeck) {
                QComboBox* box = m_deckQuickEffectSelectors[otherDeck];
                box->setCurrentIndex(effectIndex);
            }
        }

        // This is required to remove a previous selected effect that does not
        // fit to the current ShowAllEffects checkbox
        slotPopulateDeckEffectSelectors();
    }
}

void DlgPrefEQ::applySelections() {
    if (m_inSlotPopulateDeckEffectSelectors) {return;}
    auto deck = 0;
    auto firstEffectId = QString{};
    auto firstEffectIndex = 0;
    for(auto box: m_deckEqEffectSelectors) {
        auto effectId = box->itemData(box->currentIndex()).toString();
        if (deck == 0) {
            firstEffectId = effectId;
            firstEffectIndex = box->currentIndex();
        } else if (CheckBoxSingleEqEffect->isChecked()) {
            effectId = firstEffectId;
            box->setCurrentIndex(firstEffectIndex);
        }
        auto group = PlayerManager::groupForDeck(deck);
        // Only apply the effect if it changed -- so first interrogate the
        // loaded effect if any.
        auto need_load = true;
        if (m_pEQEffectRack->numEffectChainSlots() > deck) {
            // It's not correct to get a chainslot by index number -- get by
            // group name instead.
            auto chainslot = m_pEQEffectRack->getGroupEffectChainSlot(group);
            if (chainslot && chainslot->numSlots()) {
                auto  effectpointer = chainslot->getEffectSlot(0)->getEffect();
                if (effectpointer && effectpointer->getManifest().id() == effectId) {need_load = false;}
            }
        }
        if (need_load) {
            auto pEffect = m_pEffectsManager->instantiateEffect(effectId);
            m_pEQEffectRack->loadEffectToGroup(group, pEffect);
            m_pConfig->set(ConfigKey(kConfigKey, "EffectForGroup_" + group),ConfigValue(effectId));
            m_filterWaveformEnableCOs[deck]->set(m_pEffectsManager->isEQ(effectId));
            // This is required to remove a previous selected effect that does not
            // fit to the current ShowAllEffects checkbox
            slotPopulateDeckEffectSelectors();
        }
        ++deck;
    }
    deck = 0;
    for(auto box: m_deckQuickEffectSelectors) {
        auto effectId = box->itemData(box->currentIndex()).toString();
        auto group = PlayerManager::groupForDeck(deck);
        if (deck == 0) {
            firstEffectId = effectId;
            firstEffectIndex = box->currentIndex();
        } else if (CheckBoxSingleEqEffect->isChecked()) {
            effectId = firstEffectId;
            box->setCurrentIndex(firstEffectIndex);
        }
        auto pEffect = m_pEffectsManager->instantiateEffect(effectId);
        m_pQuickEffectRack->loadEffectToGroup(group, pEffect);
        m_pConfig->set(ConfigKey(kConfigKey, "QuickEffectForGroup_" + group),ConfigValue(effectId));
        // This is required to remove a previous selected effect that does not
        // fit to the current ShowAllEffects checkbox
        slotPopulateDeckEffectSelectors();
        ++deck;
    }
}
void DlgPrefEQ::slotUpdateHiEQ() {
    if (SliderHiEQ->value() < SliderLoEQ->value())
    {
        SliderHiEQ->setValue(SliderLoEQ->value());
    }
    m_highEqFreq = getEqFreq(SliderHiEQ->value(),SliderHiEQ->minimum(),SliderHiEQ->maximum());
    validate_levels();
    if (m_highEqFreq < 1000) {
        TextHiEQ->setText( QString("%1 Hz").arg((int)m_highEqFreq));
    } else {
        TextHiEQ->setText( QString("%1 kHz").arg((int)m_highEqFreq / 1000.));
    }
    m_pConfig->set(ConfigKey(kConfigKey, "HiEQFrequency"),ConfigValue(QString::number(static_cast<int>(m_highEqFreq))));
    m_pConfig->set(ConfigKey(kConfigKey, "HiEQFrequencyPrecise"),ConfigValue(QString::number(m_highEqFreq, 'f')));
    slotApply();
}
void DlgPrefEQ::slotUpdateLoEQ() {
    if (SliderLoEQ->value() > SliderHiEQ->value())
    {
        SliderLoEQ->setValue(SliderHiEQ->value());
    }
    m_lowEqFreq = getEqFreq(SliderLoEQ->value(),SliderLoEQ->minimum(),SliderLoEQ->maximum());
    validate_levels();
    if (m_lowEqFreq < 1000) {
        TextLoEQ->setText(QString("%1 Hz").arg((int)m_lowEqFreq));
    } else {
        TextLoEQ->setText(QString("%1 kHz").arg((int)m_lowEqFreq / 1000.));
    }
    m_pConfig->set(ConfigKey(kConfigKey, "LoEQFrequency"),ConfigValue(QString::number(static_cast<int>(m_lowEqFreq))));
    m_pConfig->set(ConfigKey(kConfigKey, "LoEQFrequencyPrecise"),ConfigValue(QString::number(m_lowEqFreq, 'f')));
    slotApply();
}
void DlgPrefEQ::slotUpdateMasterEQParameter(int value) {
    auto effect = EffectPointer(m_pEffectMasterEQ);
    if (!effect.isNull()) {
        auto slider = qobject_cast<QSlider*>(sender());
        auto index = slider->property("index").toInt();
        auto param = effect->getKnobParameterForSlot(index);
        if (param) {
            auto dValue = value / 100.0;
            param->setValue(dValue);
            auto valueLabel = m_masterEQValues[index];
            auto valueText = QString::number(dValue);
            valueLabel->setText(valueText);
            m_pConfig->set(ConfigKey(kConfigKey,
                    QString("EffectForGroup_[Master]_parameter%1").arg(index + 1)),ConfigValue(valueText));
        }
    }
}
int DlgPrefEQ::getSliderPosition(double eqFreq, int minValue, int maxValue) {
    if (eqFreq >= kFrequencyUpperLimit) {return maxValue;}
    else if (eqFreq <= kFrequencyLowerLimit) {return minValue;}
    auto dsliderPos = (eqFreq - kFrequencyLowerLimit) / (kFrequencyUpperLimit-kFrequencyLowerLimit);
    dsliderPos = std::pow(dsliderPos, 0.25) * (maxValue - minValue) + minValue;
    return dsliderPos;
}
void DlgPrefEQ::slotApply() {
    m_COLoFreq->set(m_lowEqFreq);
    m_COHiFreq->set(m_highEqFreq);
    m_pConfig->set(ConfigKey(kConfigKey,"EqAutoReset"),ConfigValue(m_bEqAutoReset ? 1 : 0));
    applySelections();
}
// supposed to set the widgets to match internal state
void DlgPrefEQ::slotUpdate() {
    slotUpdateLoEQ();
    slotUpdateHiEQ();
    slotPopulateDeckEffectSelectors();
    CheckBoxEqAutoReset->setChecked(m_bEqAutoReset);
}
void DlgPrefEQ::slotUpdateEqAutoReset(int i) {m_bEqAutoReset = static_cast<bool>(i);}
void DlgPrefEQ::slotBypass(int state) {
    if (state) {
        m_pConfig->set(ConfigKey(kConfigKey, kEnableEqs), QString("no"));
        // Disable effect processing for all decks by setting the appropriate
        // controls to 0 ("[EffectRackX_EffectUnitDeck_Effect1],enable")
        auto deck = 0;
        for(auto  box: m_deckEqEffectSelectors) {
            auto group = getEQEffectGroupForDeck(deck);
            ControlObject::set(ConfigKey(group, "enabled"), 0);
            m_filterWaveformEnableCOs[deck]->set(0);
            deck++;
            box->setEnabled(false);
        }
    } else {
        m_pConfig->set(ConfigKey(kConfigKey, kEnableEqs), QString("yes"));
        // Enable effect processing for all decks by setting the appropriate
        // controls to 1 ("[EffectRackX_EffectUnitDeck_Effect1],enable")
        auto deck = 0;
        ControlObjectSlave enableControl;
        for(auto box: m_deckEqEffectSelectors) {
            auto group = getEQEffectGroupForDeck(deck);
            ControlObject::set(ConfigKey(group, "enabled"), 1);
            m_filterWaveformEnableCOs[deck]->set(m_filterWaveformEffectLoaded[deck]);
            deck++;
            box->setEnabled(true);
        }
    }
    slotApply();
}
void DlgPrefEQ::setUpMasterEQ() {
    connect(pbResetMasterEq, SIGNAL(clicked(bool)),this, SLOT(slotMasterEQToDefault()));
    connect(comboBoxMasterEq, SIGNAL(currentIndexChanged(int)),this, SLOT(slotMasterEqEffectChanged(int)));
    auto configuredEffect = m_pConfig->getValueString(ConfigKey(kConfigKey,"EffectForGroup_[Master]"), kDefaultMasterEqId);
    auto availableMasterEQEffectNames = m_pEffectsManager->getEffectNamesFiltered(isMasterEQ);
    availableMasterEQEffectNames.append(QPair<QString,QString>("none", tr("None")));
    for (int i = 0; i < availableMasterEQEffectNames.size(); ++i) {
        comboBoxMasterEq->addItem(availableMasterEQEffectNames[i].second);
        comboBoxMasterEq->setItemData(i, QVariant(availableMasterEQEffectNames[i].first));
        if (configuredEffect == availableMasterEQEffectNames[i].first) {comboBoxMasterEq->setCurrentIndex(i);}
    }
    slotMasterEqEffectChanged(comboBoxMasterEq->currentIndex());
    // Load parameters from preferences:
    auto effect = EffectPointer(m_pEffectMasterEQ);
    if (!effect.isNull()) {
        auto knobNum = effect->numKnobParameters();
        for (auto i = decltype(knobNum){0}; i < knobNum; i++) {
            auto param = effect->getKnobParameterForSlot(i);
            if (param) {
                auto strValue = m_pConfig->getValueString(ConfigKey(kConfigKey,QString("EffectForGroup_[Master]_parameter%1").arg(i + 1)));
                auto ok = false;
                auto value = strValue.toDouble(&ok);
                if (ok) {setMasterEQParameter(i, value);}
            }
        }
    }
}
void DlgPrefEQ::slotMasterEqEffectChanged(int effectIndex) {
    // clear parameters view first
    qDeleteAll(m_masterEQSliders);
    m_masterEQSliders.clear();
    qDeleteAll(m_masterEQValues);
    m_masterEQValues.clear();
    qDeleteAll(m_masterEQLabels);
    m_masterEQLabels.clear();
    auto effectId = comboBoxMasterEq->itemData(effectIndex).toString();
    if (effectId == "none") {pbResetMasterEq->hide();}
    else {pbResetMasterEq->show();}
    auto pChainSlot = m_pEQEffectRack->getGroupEffectChainSlot("Master");
    if (pChainSlot) {
        auto pChain = pChainSlot->getEffectChain();
        if (pChain.isNull()) {
            pChain = EffectChainPointer(new EffectChain(m_pEffectsManager, QString(),EffectChainPointer()));
            pChain->setName(QObject::tr("Empty Chain"));
            pChainSlot->loadEffectChain(pChain);
        }
        auto pEffect = m_pEffectsManager->instantiateEffect(effectId);
        pChain->replaceEffect(0, pEffect);
        if (pEffect) {
            m_pEffectMasterEQ = pEffect;
            auto knobNum = pEffect->numKnobParameters();
            // Create and set up Master EQ's sliders
            auto i = decltype(knobNum){0};
            for (; i < knobNum; i++) {
                auto param = pEffect->getKnobParameterForSlot(i);
                if (param)
                {
                    // Setup Label
                    auto centerFreqLabel = new QLabel(this);
                    auto labelText = param->manifest().name();
                    m_masterEQLabels.append(centerFreqLabel);
                    centerFreqLabel->setText(labelText);
                    slidersGridLayout->addWidget(centerFreqLabel, 0, i + 1, Qt::AlignCenter);
                    auto slider = new QSlider(this);
                    slider->setMinimum(param->getMinimum() * 100);
                    slider->setMaximum(param->getMaximum() * 100);
                    slider->setSingleStep(1);
                    slider->setValue(param->getDefault() * 100);
                    slider->setMinimumHeight(90);
                    // Set the index as a property because we need it inside slotUpdateFilter()
                    slider->setProperty("index", QVariant(i));
                    slidersGridLayout->addWidget(slider, 1, i + 1, Qt::AlignCenter);
                    m_masterEQSliders.append(slider);
                    connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateMasterEQParameter(int)));
                    auto valueLabel = new QLabel(this);
                    m_masterEQValues.append(valueLabel);
                    auto valueText = QString::number((double)slider->value() / 100);
                    valueLabel->setText(valueText);
                    slidersGridLayout->addWidget(valueLabel, 2, i + 1, Qt::AlignCenter);

                }
            }
        }
    }
    // Update the configured effect for the current QComboBox
    m_pConfig->set(ConfigKey(kConfigKey, "EffectForGroup_[Master]"),ConfigValue(effectId));
}
double DlgPrefEQ::getEqFreq(int sliderVal, int minValue, int maxValue) {
    // We're mapping f(x) = x^4 onto the range kFrequencyLowerLimit,
    // kFrequencyUpperLimit with x [minValue, maxValue]. First translate x into
    // [0.0, 1.0], raise it to the 4th power, and then scale the result from
    // [0.0, 1.0] to [kFrequencyLowerLimit, kFrequencyUpperLimit].
    auto normValue = static_cast<double>(sliderVal - minValue) / (maxValue - minValue);
    // Use a non-linear mapping between slider and frequency.
    normValue = normValue * normValue * normValue * normValue;
    auto result = normValue * (kFrequencyUpperLimit - kFrequencyLowerLimit) + kFrequencyLowerLimit;
    return result;
}
void DlgPrefEQ::validate_levels() {
    m_highEqFreq = math_clamp<double>(m_highEqFreq, kFrequencyLowerLimit, kFrequencyUpperLimit);
    m_lowEqFreq = math_clamp<double>(m_lowEqFreq, kFrequencyLowerLimit,kFrequencyUpperLimit);
    if (m_lowEqFreq == m_highEqFreq) {
        if (m_lowEqFreq == kFrequencyLowerLimit) {
            ++m_highEqFreq;
        } else if (m_highEqFreq == kFrequencyUpperLimit) {
            --m_lowEqFreq;
        } else {
            ++m_highEqFreq;
        }
    }
}
QString DlgPrefEQ::getEQEffectGroupForDeck(int deck) const {
    // The EQ effect is loaded in effect slot 0.
    if (m_pEQEffectRack) {
        return m_pEQEffectRack->formatEffectSlotGroupString(0, PlayerManager::groupForDeck(deck));
    }
    return QString();
}
QString DlgPrefEQ::getQuickEffectGroupForDeck(int deck) const {
    // The quick effect is loaded in effect slot 0.
    if (m_pQuickEffectRack) {
        return m_pQuickEffectRack->formatEffectSlotGroupString(0, PlayerManager::groupForDeck(deck));
    }
    return QString();
}

void DlgPrefEQ::slotMasterEQToDefault() {
    auto effect = EffectPointer(m_pEffectMasterEQ);
    if (!effect.isNull()) {
        auto knobNum = effect->numKnobParameters();
        for (auto i = decltype(knobNum){0}; i < knobNum; i++) {
            auto param = effect->getKnobParameterForSlot(i);
            if (param) {
                auto defaultValue = param->getDefault();
                setMasterEQParameter(i, defaultValue);
            }
        }
    }
}
void DlgPrefEQ::setMasterEQParameter(int i, double value) {
    auto effect = EffectPointer(m_pEffectMasterEQ);
    if (!effect.isNull()) {
        auto  param = effect->getKnobParameterForSlot(i);
        if (param) {
            param->setValue(value);
            m_masterEQSliders[i]->setValue(value * 100);
            auto  valueLabel = m_masterEQValues[i];
            auto  valueText = QString::number(value);
            valueLabel->setText(valueText);
            m_pConfig->set(ConfigKey(kConfigKey,QString("EffectForGroup_[Master]_parameter%1").arg(i + 1)),ConfigValue(valueText));
        }
    }
}
