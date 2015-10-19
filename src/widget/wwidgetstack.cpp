#include <QtDebug>

#include "widget/wwidgetstack.h"
#include "controlobject.h"
WidgetStackControlListener::WidgetStackControlListener(QObject* pParent,
                                                       ControlObject* pControl,
                                                       int index)
        : QObject(pParent),
          m_control(new ControlObject(pControl ? pControl->getKey() : ConfigKey(),this)),
          m_index(index) {
    connect(m_control, SIGNAL(valueChanged(double)),this, SLOT(slotValueChanged(double)));
}

WidgetStackControlListener::~WidgetStackControlListener() {}

void WidgetStackControlListener::slotValueChanged(double v) {
    if (v > 0.0) {emit(switchToWidget());}
    else {emit(hideWidget());}
}

void WidgetStackControlListener::onCurrentWidgetChanged(int index) {
    if (index == m_index && m_control->get() == 0.0) {m_control->set(1.0);}
    else if (index != m_index && m_control->get() != 0.0) {m_control->set(0.0);}
}
void WidgetStackControlListener::setControl(double v){m_control->set(v);}
WWidgetStack::WWidgetStack(QWidget* pParent,
                           ControlObject* pNextControl,
                           ControlObject* pPrevControl,
                           ControlObject* pCurrentPageControl)
        : QStackedWidget(pParent),
          WBaseWidget(this),
          m_nextControl(new ControlObject(pNextControl ? pNextControl->getKey() : ConfigKey(),this)),
          m_prevControl(new ControlObject(pPrevControl ? pPrevControl->getKey() : ConfigKey(),this)),
          m_currentPageControl(
              new ControlObject(
                  pCurrentPageControl ?
                  pCurrentPageControl->getKey() : ConfigKey(),this)) {
    connect(m_nextControl, SIGNAL(valueChanged(double)),this, SLOT(onNextControlChanged(double)));
    connect(m_prevControl, SIGNAL(valueChanged(double)),this, SLOT(onPrevControlChanged(double)));
    connect(m_currentPageControl, SIGNAL(valueChanged(double)),this, SLOT(onCurrentPageControlChanged(double)));
    connect(&m_showMapper, SIGNAL(mapped(int)),this, SLOT(showIndex(int)));
    connect(&m_hideMapper, SIGNAL(mapped(int)),this, SLOT(hideIndex(int)));
}
// override
void WWidgetStack::Init() {
    WBaseWidget::Init();
    connect(this, SIGNAL(currentChanged(int)),this, SLOT(onCurrentPageChanged(int)));
}
WWidgetStack::~WWidgetStack() {}
QSize WWidgetStack::sizeHint() const {
    auto pWidget = currentWidget();
    return pWidget ? pWidget->sizeHint() : QSize();
}
QSize WWidgetStack::minimumSizeHint() const {
    auto pWidget = currentWidget();
    return pWidget ? pWidget->minimumSizeHint() : QSize();
}
void WWidgetStack::showIndex(int index) {
    // Only respond to changes if the stack is visible.  This allows multiple
    // stacks to use the same trigger COs without causing conflicts.
    if (isVisible()) {setCurrentIndex(index);}
}
void WWidgetStack::hideIndex(int index) {
    if (!isVisible()) {return;}
    if (currentIndex() == index) {
        auto  it = m_hideMap.constFind(index);
        if (it != m_hideMap.cend()) {setCurrentIndex(*it);}
        else {
            // TODO: This default behavior is a little odd, is it really what
            // we want?  Or should we save the previously-selected page and then
            // switch to that.
            setCurrentIndex((index + 1) % count());
        }
    }
}
void WWidgetStack::showEvent(QShowEvent*) {
    int index = static_cast<int>(m_currentPageControl->get());
    // Set the page triggers to match the current index.
    for (auto it = m_listeners.begin(),end = m_listeners.end(); it != end; ++it) {
        it.value()->setControl(it.key() == index ? 1.0 : 0.0);
    }
    setCurrentIndex(index);
}

void WWidgetStack::onNextControlChanged(double v) {
    if (!isVisible()) {return;}
    if (v > 0.0) {setCurrentIndex((currentIndex() + 1) % count());}
}
void WWidgetStack::onPrevControlChanged(double v) {
    if (!isVisible()) {return;}
    if (v > 0.0) {
        int newIndex = currentIndex() - 1;
        while (newIndex < 0) {newIndex += count();}
        setCurrentIndex(newIndex);
    }
}
void WWidgetStack::onCurrentPageChanged(int index) {
    if (!isVisible()) {return; }
    m_currentPageControl->set(static_cast<double>(index));
}
void WWidgetStack::onCurrentPageControlChanged(double v) {
    if (!isVisible()) {return;}
    int newIndex = static_cast<int>(v);
    setCurrentIndex(newIndex);
}
void WWidgetStack::addWidgetWithControl(QWidget* pWidget, ControlObject* pControl,int on_hide_select) {
    int index = addWidget(pWidget);
    if (pControl) {
        auto pListener = new WidgetStackControlListener( this, pControl, index);
        m_showMapper.setMapping(pListener, index);
        m_hideMapper.setMapping(pListener, index);
        m_listeners[index] = pListener;
        if (pControl->get() > 0) { setCurrentIndex(count()-1); }
        pListener->onCurrentWidgetChanged(currentIndex());
        connect(pListener, SIGNAL(switchToWidget()),&m_showMapper, SLOT(map()));
        connect(pListener, SIGNAL(hideWidget()),&m_hideMapper, SLOT(map()));
        connect(this, SIGNAL(currentChanged(int)),pListener, SLOT(onCurrentWidgetChanged(int)));
    }
        // The value in the current page control overrides whatever initial
        // values the individual page triggers may have.
    if (m_currentPageControl->get() == index) {setCurrentIndex(index);}
    if (on_hide_select != -1) {m_hideMap[index] = on_hide_select;}
}
bool WWidgetStack::event(QEvent* pEvent) {
    if (pEvent->type() == QEvent::ToolTip) {updateTooltip();}
    return QFrame::event(pEvent);
}
