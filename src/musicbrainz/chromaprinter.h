_Pragma("once")
#include <QObject>
#include "trackinfoobject.h"
class ChromaPrinter: public QObject {
  Q_OBJECT
public:
      explicit ChromaPrinter(QObject* parent = nullptr);
      virtual ~ChromaPrinter();
      QString  getFingerprint(TrackPointer pTrack);
};
