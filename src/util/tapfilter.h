_Pragma("once")
#include <QObject>
#include <QTime>

#include "util/movinginterquartilemean.h"
#include "util/types.h"

class TapFilter : public QObject {
    Q_OBJECT
  public:
    TapFilter(QObject *pParent, int filterLength, int maxInterval);
    virtual ~TapFilter();
  public slots:
    void tap();
  signals:
    void tapped(double averageLength, int numSamples);
  private:
    QTime m_timer;
    MovingInterquartileMean m_mean;
    int m_iMaxInterval;
};
