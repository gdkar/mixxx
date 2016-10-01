#include "util/alphabetafilter.h"

AlphaBetaFilter::AlphaBetaFilter(QObject *p )
:QObject(p)
,m_initialized(false),
m_dt(0.0),
m_x(0.0),
m_v(0.0),
m_alpha(0.0),
m_beta(0.0) {
    }

void AlphaBetaFilter::observe(double dx)
{
    if (!m_initialized)
        return;

    auto predicted_x = m_x + m_v * m_dt;
    auto predicted_v = m_v;
    auto residual_x = dx - predicted_x;

    m_x = predicted_x + residual_x * m_alpha;
    m_v = predicted_v + residual_x * m_beta / m_dt;

    // relative to previous
    m_x -= dx;
}

// Get the velocity after filtering.
double AlphaBetaFilter::predictedVelocity() const
{
    return m_v;
}

// Get the position after filtering.
double AlphaBetaFilter::predictedPosition() const
{
    return m_x;
}
void AlphaBetaFilter::init(double dt, double v, double alpha , double beta )
{
    m_initialized = true;
    m_dt = dt;
    m_x = 0.0;
    m_v = v;
    m_alpha = alpha;
    m_beta = beta;
}

