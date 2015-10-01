/***************************************************************************
                          wvumeter.cpp  -  description
                             -------------------
    begin                : Fri Jul 22 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "widget/wvumeter.h"

#include <QStylePainter>
#include <QStyleOption>
#include <QPaintEvent>
#include <QtDebug>
#include <QPixmap>

#include "widget/wpixmapstore.h"
#include "util/timer.h"
#include "util/math.h"

#define DEFAULT_FALLTIME 20
#define DEFAULT_FALLSTEP 1
#define DEFAULT_HOLDTIME 400
#define DEFAULT_HOLDSIZE 5

WVuMeter::WVuMeter(QWidget* parent)
        : WWidget(parent),
          m_dParameter(0),
          m_dPeakParameter(0),
          m_dLastParameter(0),
          m_dLastPeakParameter(0),
          m_iPixmapLength(0),
          m_bHorizontal(false),
          m_iPeakHoldSize(0),
          m_iPeakFallStep(0),
          m_iPeakHoldTime(0),
          m_iPeakFallTime(0),
          m_dPeakHoldCountdownMs(0) {
    m_timer.start();
}

WVuMeter::~WVuMeter() {
}

void WVuMeter::setup(QDomNode node, const SkinContext& context) {
    // Set pixmaps
    bool bHorizontal = context.hasNode(node, "Horizontal") &&
    context.selectString(node, "Horizontal") == "true";

    // Set background pixmap if available
    if (context.hasNode(node, "PathBack")) {
        QDomElement backPathNode = context.selectElement(node, "PathBack");
        // The implicit default in <1.12.0 was FIXED so we keep it for backwards
        // compatibility.
        setPixmapBackground(context.getPixmapSource(backPathNode),
                            context.selectScaleMode(backPathNode, Paintable::FIXED));
    }

    QDomElement vuNode = context.selectElement(node, "PathVu");
    // The implicit default in <1.12.0 was FIXED so we keep it for backwards
    // compatibility.
    setPixmaps(context.getPixmapSource(vuNode), bHorizontal,
               context.selectScaleMode(vuNode, Paintable::FIXED));

    m_iPeakHoldSize = context.selectInt(node, "PeakHoldSize");
    if (m_iPeakHoldSize < 0 || m_iPeakHoldSize > 100)
        m_iPeakHoldSize = DEFAULT_HOLDSIZE;

    m_iPeakFallStep = context.selectInt(node, "PeakFallStep");
    if (m_iPeakFallStep < 1 || m_iPeakFallStep > 1000)
        m_iPeakFallStep = DEFAULT_FALLSTEP;

    m_iPeakHoldTime = context.selectInt(node, "PeakHoldTime");
    if (m_iPeakHoldTime < 1 || m_iPeakHoldTime > 3000)
        m_iPeakHoldTime = DEFAULT_HOLDTIME;

    m_iPeakFallTime = context.selectInt(node, "PeakFallTime");
    if (m_iPeakFallTime < 1 || m_iPeakFallTime > 1000)
        m_iPeakFallTime = DEFAULT_FALLTIME;
}
void WVuMeter::setPixmapBackground(PixmapSource source, Paintable::DrawMode mode) {
    m_pPixmapBack = WPixmapStore::getPaintable(source, mode);
    if (m_pPixmapBack.isNull() || m_pPixmapBack->isNull()) {
        qDebug() << metaObject()->className()
                 << "Error loading background pixmap:" << source.getPath();
    } else if (mode == Paintable::FIXED)  setFixedSize(m_pPixmapBack->size());
}
void WVuMeter::setPixmaps(PixmapSource source,bool bHorizontal, Paintable::DrawMode mode) {
    m_pPixmapVu = WPixmapStore::getPaintable(source, mode);
    if (m_pPixmapVu.isNull() || m_pPixmapVu->isNull()) 
        qDebug() << "WVuMeter: Error loading vu pixmap" << source.getPath();
    else {
        m_bHorizontal = bHorizontal;
        if (m_bHorizontal)  m_iPixmapLength = m_pPixmapVu->width();
        else  m_iPixmapLength = m_pPixmapVu->height();
    }
}

void WVuMeter::onConnectedControlChanged(double dParameter, double dValue) {
    Q_UNUSED(dValue);
    m_dParameter = clamp(dParameter, 0.0, 1.0);

    if (dParameter > 0.0) {
        setPeak(dParameter);
    } else {
        // A 0.0 value is very unlikely except when the VU Meter is disabled
        m_dPeakParameter = 0;
    }

    double msecsElapsed = m_timer.restart() / 1000000.0;
    updateState(msecsElapsed);
}

void WVuMeter::setPeak(double parameter) {
    if (parameter > m_dPeakParameter) {
        m_dPeakParameter = parameter;
        m_dPeakHoldCountdownMs = m_iPeakHoldTime;
    }
}

void WVuMeter::updateState(double msecsElapsed) {
    // If we're holding at a peak then don't update anything
    m_dPeakHoldCountdownMs -= msecsElapsed;
    if (m_dPeakHoldCountdownMs > 0) {
        return;
    } else {
        m_dPeakHoldCountdownMs = 0;
    }

    // Otherwise, decrement the peak position by the fall step size times the
    // milliseconds elapsed over the fall time multiplier. The peak will fall
    // FallStep times (out of 128 steps) every FallTime milliseconds.
    m_dPeakParameter -= static_cast<double>(m_iPeakFallStep) *
            msecsElapsed /
            static_cast<double>(m_iPeakFallTime * m_iPixmapLength);
    m_dPeakParameter = clamp(m_dPeakParameter, 0.0, 1.0);
}

void WVuMeter::maybeUpdate() {
    if (m_dParameter != m_dLastParameter || m_dPeakParameter != m_dLastPeakParameter) repaint();
}
void WVuMeter::paintEvent(QPaintEvent *) {
    ScopedTimer t("WVuMeter::paintEvent");
    QStyleOption option;
    option.initFrom(this);
    QStylePainter p(this);
    p.drawPrimitive(QStyle::PE_Widget, option);
    if (!m_pPixmapBack.isNull() && !m_pPixmapBack->isNull())m_pPixmapBack->draw(rect(), &p);
    if (!m_pPixmapVu.isNull() && !m_pPixmapVu->isNull()) {
        const double widgetWidth = width();
        const double widgetHeight = height();
        const double pixmapWidth = m_pPixmapVu->width();
        const double pixmapHeight = m_pPixmapVu->height();
        // Draw (part of) vu
        if (m_bHorizontal) {
            const double widgetPosition = clamp(widgetWidth * m_dParameter,0.0, widgetWidth);
            QRectF targetRect(0, 0, widgetPosition, widgetHeight);
            const double pixmapPosition = clamp(pixmapWidth * m_dParameter,0.0, pixmapWidth);
            QRectF sourceRect(0, 0, pixmapPosition,  m_pPixmapVu->height());
            m_pPixmapVu->draw(targetRect, &p, sourceRect);
            if (m_iPeakHoldSize > 0 && m_dPeakParameter > 0.0) {
                const double widgetPeakPosition = clamp(widgetWidth * m_dPeakParameter, 0.0, widgetWidth);
                const double widgetPeakHoldSize = widgetWidth *static_cast<double>(m_iPeakHoldSize) / pixmapWidth;
                const double pixmapPeakPosition = clamp(pixmapWidth * m_dPeakParameter, 0.0, pixmapWidth);
                const double pixmapPeakHoldSize = m_iPeakHoldSize;
                targetRect = QRectF(widgetPeakPosition - widgetPeakHoldSize, 0,widgetPeakHoldSize, widgetHeight);
                sourceRect = QRectF(pixmapPeakPosition - pixmapPeakHoldSize, 0,pixmapPeakHoldSize, pixmapHeight);
                m_pPixmapVu->draw(targetRect, &p, sourceRect);
            }
        } else {
            const double widgetPosition = clamp(widgetHeight * m_dParameter,0.0, widgetHeight);
            QRectF targetRect(0, widgetHeight - widgetPosition,widgetWidth, widgetPosition);
            const double pixmapPosition = clamp(pixmapHeight * m_dParameter,0.0, pixmapHeight);
            QRectF sourceRect(0, pixmapHeight - pixmapPosition,pixmapWidth, pixmapPosition);
            m_pPixmapVu->draw(targetRect, &p, sourceRect);
            if (m_iPeakHoldSize > 0 && m_dPeakParameter > 0.0) {
                const double widgetPeakPosition = clamp(widgetHeight * m_dPeakParameter, 0.0, widgetHeight);
                const double widgetPeakHoldSize = widgetHeight *static_cast<double>(m_iPeakHoldSize) / pixmapHeight;
                const double pixmapPeakPosition = clamp(pixmapHeight * m_dPeakParameter, 0.0, pixmapHeight);
                const double pixmapPeakHoldSize = m_iPeakHoldSize;
                targetRect = QRectF(0, widgetHeight - widgetPeakPosition,widgetWidth, widgetPeakHoldSize);
                sourceRect = QRectF(0, pixmapHeight - pixmapPeakPosition,pixmapWidth, pixmapPeakHoldSize);
                m_pPixmapVu->draw(targetRect, &p, sourceRect);
            }
        }
    }
    m_dLastParameter = m_dParameter;
    m_dLastPeakParameter = m_dPeakParameter;
}
