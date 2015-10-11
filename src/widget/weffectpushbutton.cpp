#include "widget/weffectpushbutton.h"

#include <QtDebug>

#include "widget/effectwidgetutils.h"

WEffectPushButton::WEffectPushButton(QWidget* pParent, EffectsManager* pEffectsManager)
        : WPushButton(pParent),
          m_pEffectsManager(pEffectsManager),
          m_pButtonMenu(nullptr) {
}
WEffectPushButton::~WEffectPushButton() = default;
void WEffectPushButton::setup(QDomNode node, const SkinContext* context) {
    // Setup parent class.
    WPushButton::setup(node, context);
    m_pButtonMenu = new QMenu(this);
    connect(m_pButtonMenu, SIGNAL(triggered(QAction*)),this, SLOT(slotActionChosen(QAction*)));
    // EffectWidgetUtils propagates NULLs so this is all safe.
    auto pRack = EffectWidgetUtils::getEffectRackFromNode(node, context, m_pEffectsManager);
    auto pChainSlot = EffectWidgetUtils::getEffectChainSlotFromNode(node, context, pRack);
    auto pEffectSlot = EffectWidgetUtils::getEffectSlotFromNode(node, context, pChainSlot);
    auto pParameterSlot =EffectWidgetUtils::getButtonParameterSlotFromNode(node, context, pEffectSlot);
    if (pParameterSlot) {
        m_pEffectParameterSlot = pParameterSlot;
        connect(pParameterSlot.data(), SIGNAL(updated()),this, SLOT(parameterUpdated()));
        parameterUpdated();
    } else {
        SKIN_WARNING(node, context)
                << "EffectPushButton node could not attach to effect parameter.";
    }
}

void WEffectPushButton::onConnectedControlChanged(double dParameter, double dValue) {
    for(auto action: m_pButtonMenu->actions()) {
        if (action->data().toDouble() == dValue) {
            action->setChecked(true);
            break;
        }
    }
    WPushButton::onConnectedControlChanged(dParameter, dValue);
}

void WEffectPushButton::mousePressEvent(QMouseEvent* e) {
    auto rightClick = e->button() == Qt::RightButton;
    if (rightClick && m_pButtonMenu->actions().size()) {
        m_pButtonMenu->exec(e->globalPos());
        return;
    }
    // Pass all other press events to the base class.
    WPushButton::mousePressEvent(e);
    // The push handler may have set the left value. Check the corresponding
    // QAction.
    auto leftValue = getControlParameterLeft();
    for(auto action: m_pButtonMenu->actions()) {
        if (action->data().toDouble() == leftValue) {
            action->setChecked(true);
            break;
        }
    }
}
void WEffectPushButton::mouseReleaseEvent(QMouseEvent* e) {
    // Pass all other release events to the base class.
    WPushButton::mouseReleaseEvent(e);
    // The release handler may have set the left value. Check the corresponding
    // QAction.
    auto leftValue = getControlParameterLeft();
    for(auto action: m_pButtonMenu->actions()) {
        if (action->data().toDouble() == leftValue) {
            action->setChecked(true);
            break;
        }
    }
}

void WEffectPushButton::parameterUpdated() {
    m_pButtonMenu->clear();
    auto options = m_pEffectParameterSlot->getManifest().getSteps();
    // qDebug() << " HERE IS THE OPTIONS SIZE: " << options.size() << m_pEffectParameterSlot->getManifest().name();
    m_iNoStates = options.size();
    if (m_iNoStates == 0) {
        // Toggle button without a menu
        m_iNoStates = 2;
        return;
    }
    auto value = getControlParameterLeft();
    auto actionGroup = new QActionGroup(m_pButtonMenu);
    actionGroup->setExclusive(true);
    for (auto i = 0; i < options.size(); i++) {
        // action is added automatically to actionGroup
        auto action = new QAction(actionGroup);
        // qDebug() << options[i].first;
        action->setText(options[i].first);
        action->setData(options[i].second);
        action->setCheckable(true);
        if (options[i].second == value) {action->setChecked(true);}
        m_pButtonMenu->addAction(action);
    }
}
void WEffectPushButton::slotActionChosen(QAction* action) {
    action->setChecked(true);
    setControlParameter(action->data().toDouble());
}
