/***************************************************************************
                        alphabetafilter.h  -  description
                        -------------------
begin                : Wed May 12 2010
copyright            : (C) 2010 by Sean M. Pappalardo
email                : spappalardo@mixxx.org

This is essentially just a C++ version of xwax's pitch.h,
    which is Copyright (C) 2010 Mark Hills <mark@pogo.org.uk>
***************************************************************************/

#ifndef ALPHABETAFILTER_H
#define ALPHABETAFILTER_H

#include <QtGlobal>
#include <QObject>
#include <QMetaType>
#include <QMetaObject>
#include <QtQml>
#include <QtQuick>

// This is a simple alpha-beta filter. It follows the example from Wikipedia
// closely: http://en.wikipedia.org/wiki/Alpha_beta_filter
//
// Given an initial position and velocity, learning parameters alpha and beta,
// and a series of input observations of the distance travelled, the filter
// predicts the real position and velocity.
class AlphaBetaFilter : public QObject {
    Q_OBJECT
    Q_PROPERTY(double dt MEMBER m_dt)
    Q_PROPERTY(double position READ predictedPosition)
    Q_PROPERTY(double velocity READ predictedVelocity)
    Q_PROPERTY(double alpha MEMBER m_alpha)
    Q_PROPERTY(double beta MEMBER m_beta)
  public:
    Q_INVOKABLE AlphaBetaFilter(QObject *p = nullptr);
    // values were concluded experimentally for time code vinyl.
    Q_INVOKABLE void init(double dt, double v, double alpha = 1.0/512, double beta = (1.0/512)/1024);
    // Input an observation to the filter; in the last dt seconds the position
    // has moved by dx.
    //
    // Because the values come from a digital controller, the values for dx are
    // discrete rather than smooth.
    Q_INVOKABLE void observe(double dx);
    // Get the velocity after filtering.
    double predictedVelocity() const;
    // Get the position after filtering.
    double predictedPosition() const;
  private:
    // Whether init() has been called.
    bool m_initialized = false;
    // State of the rate calculation filter
    double m_dt, m_x = 0., m_v = 0., m_alpha = 1./512, m_beta = (1./512)/1024;
};

#endif
