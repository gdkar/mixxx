/*****************************************************************************
 *  Copyright © 2012 John Maguire <john.maguire@gmail.com>                   *
 *                   David Sansome <me@davidsansome.com>                     *
 *  This work is free. You can redistribute it and/or modify it under the    *
 *  terms of the Do What The Fuck You Want To Public License, Version 2,     *
 *  as published by Sam Hocevar.                                             *
 *  See http://www.wtfpl.net/ for more details.                              *
 *****************************************************************************/
    
_Pragma("once")
#include <QAbstractNetworkCache>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class NetworkAccessManager : public QNetworkAccessManager
{
  Q_OBJECT
  public:
    NetworkAccessManager(QObject* parent = nullptr);
  protected:
    QNetworkReply* createRequest(Operation op, QNetworkRequest request,QIODevice* outgoingData);
};
class NetworkTimeouts : public QObject {
    Q_OBJECT
  public:
    NetworkTimeouts(int timeout_msec, QObject* parent = nullptr);
    void addReply(QNetworkReply* reply);
    void setTimeout(int msec);
  protected:
    void timerEvent(QTimerEvent* e);
  private slots:
    void replyFinished();
  private:
    int m_timeout_msec;
    QMap<QNetworkReply*, int> m_timers;
};
