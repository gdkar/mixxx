#include "dlgprefeffects.h"

#include "effects/effectsmanager.h"

DlgPrefEffects::DlgPrefEffects(QWidget* pParent,
                               ConfigObject<ConfigValue>* pConfig,
                               EffectsManager* pEffectsManager)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_pEffectsManager(pEffectsManager) {
    setupUi(this);
    connect(availableEffectsList,
            SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this,
            SLOT(slotEffectSelected(QListWidgetItem*, QListWidgetItem*)));
}
void DlgPrefEffects::slotUpdate() {
    clear();
    auto effectIds = m_pEffectsManager->getAvailableEffects();
    for(auto id: effectIds) {addEffectToList(id);}
    if (!effectIds.isEmpty()) {availableEffectsList->setCurrentRow(0);}
}
void DlgPrefEffects::addEffectToList(const QString& effectId) {
    auto manifestAndBackend = m_pEffectsManager->getEffectManifestAndBackend(effectId);
    auto pItem = new QListWidgetItem();
    pItem->setText(manifestAndBackend.first.name());
    pItem->setData(Qt::UserRole, effectId);
    availableEffectsList->addItem(pItem);
}
void DlgPrefEffects::slotApply() {}
void DlgPrefEffects::slotResetToDefaults() {}
void DlgPrefEffects::clear() {
    availableEffectsList->clear();
    effectName->clear();
    effectAuthor->clear();
    effectDescription->clear();
    effectVersion->clear();
    effectType->clear();
}
void DlgPrefEffects::slotEffectSelected(QListWidgetItem* pCurrent,QListWidgetItem* /*pPrevious*/) {
    if (!pCurrent) {return;}
    auto effectId = pCurrent->data(Qt::UserRole).toString();
    auto manifestAndBackend = m_pEffectsManager->getEffectManifestAndBackend(effectId);
    const auto& manifest = manifestAndBackend.first;
    effectName->setText(manifest.name());
    effectAuthor->setText(manifest.author());
    effectDescription->setText(manifest.description());
    effectVersion->setText(manifest.version());
    if (manifestAndBackend.second) {effectType->setText(manifestAndBackend.second->getName());}
    else {effectType->clear();}
}
