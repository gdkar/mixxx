#ifndef CHROMAPRINTER_H
#define CHROMAPRINTER_H

#include <QObject>

#include "trackinfoobject.h"

class ChromaPrinter: public QObject {
  Q_OBJECT

public:
      explicit ChromaPrinter(QObject* parent = nullptr);
      QString getFingerprint(TrackPointer pTrack);
};

#endif //CHROMAPRINTER_H
