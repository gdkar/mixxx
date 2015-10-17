_Pragma("once")
#include <QObject>
#include "trackinfoobject.h"
class ChromaPrinter: public QObject {
  Q_OBJECT
public:
      explicit ChromaPrinter(QObject* parent = nullptr);
      QString getFingerprint(TrackPointer pTrack);
};
