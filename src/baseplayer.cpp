#include "baseplayer.h"

BasePlayer::BasePlayer(const QString &group, QObject* pParent)
        : QObject(pParent),
          m_group(group) {
}

BasePlayer::~BasePlayer() {

}
