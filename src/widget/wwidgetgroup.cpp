#include "widget/wwidgetgroup.h"

#include <QLayout>
#include <QMap>
#include <QStylePainter>
#include <QStackedLayout>

#include "skin/skincontext.h"
#include "widget/wwidget.h"
#include "widget/wpixmapstore.h"
#include "util/debug.h"

WWidgetGroup::WWidgetGroup(QWidget* pParent)
        : QFrame(pParent),
          WBaseWidget(this),
          m_pPixmapBack(nullptr)
{
    setObjectName("WidgetGroup");
}
WWidgetGroup::~WWidgetGroup() = default;
int WWidgetGroup::layoutSpacing() const
{
    if(auto pLayout = layout() ) return pLayout->spacing();
    return 0;
}
void WWidgetGroup::setLayoutSpacing(int spacing)
{
    //qDebug() << "WWidgetGroup::setSpacing" << spacing;
    if (spacing < 0)
    {
        qDebug() << "WWidgetGroup: Invalid spacing:" << spacing;
        return;
    }
    if(auto pLayout = layout()) pLayout->setSpacing(spacing);
}
QRect WWidgetGroup::layoutContentsMargins() const
{
    auto pLayout = layout();
    auto margins = pLayout ? pLayout->contentsMargins() : contentsMargins();
    return QRect(margins.left(), margins.top(), margins.right(), margins.bottom());
}
void WWidgetGroup::setLayoutContentsMargins(QRect rectMargins)
{
    // qDebug() << "WWidgetGroup::setLayoutContentsMargins" << rectMargins.x()
    //          << rectMargins.y() << rectMargins.width() << rectMargins.height();
    if (rectMargins.x() < 0 || rectMargins.y() < 0 ||
            rectMargins.width() < 0 || rectMargins.height() < 0) {
        qDebug() << "WWidgetGroup: Invalid ContentsMargins rectangle:"
                 << rectMargins;
        return;
    }
    setContentsMargins(rectMargins.x(), rectMargins.y(),rectMargins.width(), rectMargins.height());
    if (auto pLayout = layout())
        pLayout->setContentsMargins(rectMargins.x(), rectMargins.y(),rectMargins.width(), rectMargins.height());
}
Qt::Alignment WWidgetGroup::layoutAlignment() const
{
    if(auto pLayout = layout())return pLayout->alignment();
    return Qt::Alignment();
}
void WWidgetGroup::setLayoutAlignment(int alignment)
{
    //qDebug() << "WWidgetGroup::setLayoutAlignment" << alignment;
    if (auto pLayout = layout()) {
        pLayout->setAlignment(static_cast<Qt::Alignment>(alignment));
    }
}
void WWidgetGroup::setup(QDomNode node, const SkinContext& context) {
    setContentsMargins(0, 0, 0, 0);
    // Set background pixmap if available
    if (context.hasNode(node, "BackPath")) {
        auto backPathNode = context.selectElement(node, "BackPath");
        setPixmapBackground(context.getPixmapSource(backPathNode),
                            context.selectScaleMode(backPathNode, Paintable::TILE));
    }
    auto pLayout = static_cast<QLayout*>(nullptr);
    if (context.hasNode(node, "Layout")) {
        auto layout = context.selectString(node, "Layout");
        if (layout == "vertical")pLayout = new QVBoxLayout();
        else if (layout == "horizontal")pLayout = new QHBoxLayout();
        else if (layout == "stacked")
        {
            auto pStackedLayout = new QStackedLayout();
            pStackedLayout->setStackingMode(QStackedLayout::StackAll);
            pLayout = pStackedLayout;
        }
        // Set common layout parameters.
        if (pLayout)
        {
            pLayout->setSpacing(0);
            pLayout->setContentsMargins(0, 0, 0, 0);
            pLayout->setAlignment(Qt::AlignCenter);
        }
    }
    if (pLayout && context.hasNode(node, "SizeConstraint"))
    {
        QMap<QString, QLayout::SizeConstraint> constraints;
        constraints["SetDefaultConstraint"] = QLayout::SetDefaultConstraint;
        constraints["SetFixedSize"] = QLayout::SetFixedSize;
        constraints["SetMinimumSize"] = QLayout::SetMinimumSize;
        constraints["SetMaximumSize"] = QLayout::SetMaximumSize;
        constraints["SetMinAndMaxSize"] = QLayout::SetMinAndMaxSize;
        constraints["SetNoConstraint"] = QLayout::SetNoConstraint;
        auto sizeConstraintStr = context.selectString(node, "SizeConstraint");
        if (constraints.contains(sizeConstraintStr)) {
            pLayout->setSizeConstraint(constraints[sizeConstraintStr]);
        } else qDebug() << "Could not parse SizeConstraint:" << sizeConstraintStr;
    }
    if (pLayout) {setLayout(pLayout);}
}
void WWidgetGroup::setPixmapBackground(PixmapSource source, Paintable::DrawMode mode)
{
    // Load background pixmap
    m_pPixmapBack = WPixmapStore::getPaintable(source, mode);
    if (!m_pPixmapBack) qDebug() << "WWidgetGroup: Error loading background pixmap:" << source.getPath();
}
void WWidgetGroup::addWidget(QWidget* pChild)
{
    if(pChild)
    {
      if(auto pLayout = layout())pLayout->addWidget(pChild);
    }
}
void WWidgetGroup::paintEvent(QPaintEvent* pe)
{
    QFrame::paintEvent(pe);
    if (m_pPixmapBack)
    {
        QStylePainter p(this);
        m_pPixmapBack->draw(rect(), &p);
    }
}
void WWidgetGroup::resizeEvent(QResizeEvent* re)
{
    // Paint things styled by style sheet
    QFrame::resizeEvent(re);
}
bool WWidgetGroup::event(QEvent* pEvent)
{
    if (pEvent->type() == QEvent::ToolTip) updateTooltip();
    return QFrame::event(pEvent);
}
void WWidgetGroup::fillDebugTooltip(QStringList* debug)
{
    WBaseWidget::fillDebugTooltip(debug);
    *debug << QString("LayoutAlignment: %1").arg(toDebugString(layoutAlignment()))
           << QString("LayoutContentsMargins: %1").arg(toDebugString(layoutContentsMargins()))
           << QString("LayoutSpacing: %1").arg(layoutSpacing());
}
