//
// C++ Interface: wnumberpos
//
// Description:
//
//
// Author: Tue Haste Andersen <haste@diku.dk>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//
_Pragma("once")
#include "widget/wnumber.h"

class ControlObject;

class WNumberRate : public WNumber {
    Q_OBJECT
  public:
    WNumberRate(const char *group, QWidget *parent=0);
    virtual ~WNumberRate();

  private slots:
    void setValue(double dValue);

  private:
    // Pointer to control objects for rate.
    ControlObject* m_pRateControl;
    ControlObject* m_pRateRangeControl;
    ControlObject* m_pRateDirControl;
};
