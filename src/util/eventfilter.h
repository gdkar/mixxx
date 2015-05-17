#ifndef EVENTFILTER_H
#define EVENTFILTER_H

#include <qglobal.h>
#include <qevent.h>
#include <qcoreapplication.h>
#include <qobject.h>

class EventFilter : public QObject {
  Q_OBJECT
  public:
    explicit EventFilter(QObject *pParent=0):QObject(pParent){}
    virtual bool eventFilter(QObject * /* o */, QEvent * /* e */) = 0;
};
#endif
