#include <QtDebug>
#include <QTime>
#include <QLocale>

#include "widget/wtime.h"
#include "util/cmdlineargs.h"

WTime::WTime(QWidget *parent)
        : WLabel(parent),
          m_sTimeFormat("h:mm AP"),
          m_iInterval(s_iMinuteInterval) {
    m_pTimer = new QTimer(this);
    m_pTimer->setTimerType(Qt::PreciseTimer);
}
WTime::~WTime()
{
    delete m_pTimer;
}
void WTime::setup(QDomNode node, const SkinContext& context)
{
    WLabel::setup(node, context);
    setTimeFormat(node, context);
    m_pTimer->start(m_iInterval);
    connect(m_pTimer, SIGNAL(timeout()),this, SLOT(refreshTime()));
    refreshTime();
}
void WTime::setTimeFormat(QDomNode node, const SkinContext& context)
{
    // if a custom format is defined, all other formatting flags are ignored
    if (!context.hasNode(node, "CustomFormat")) {
        // check if seconds should be shown
        auto secondsFormat = context.selectString(node, "ShowSeconds");
        // long format is equivalent to showing seconds
        QLocale::FormatType format;
        if(secondsFormat == "true" || secondsFormat == "yes")
        {
            format = QLocale::LongFormat;
            m_iInterval = s_iSecondInterval;
        }
        else
        {
            format = QLocale::ShortFormat;
            m_iInterval = s_iMinuteInterval;
        }
        m_sTimeFormat = QLocale().timeFormat(format);
    } else m_sTimeFormat = context.selectString(node, "CustomFormat");
}

void WTime::refreshTime()
{
    auto time = QTime::currentTime();
    auto timeString = time.toString(m_sTimeFormat);
    if (text() != timeString) setText(timeString);
}
