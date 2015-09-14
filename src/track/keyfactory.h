_Pragma("once")
#include <QHash>
#include <QVector>
#include <QString>

#include "track/keys.h"
#include "proto/keys.pb.h"
#include "trackinfoobject.h"

class KeyFactory {
  public:
    using ChromaticKey = mixxx::track::io::key::ChromaticKey;
    using Source       = mixxx::track::io::key::Source;
    static Keys loadKeysFromByteArray(const QString& keysVersion,const QString& keysSubVersion,QByteArray* keysSerialized);
    static Keys makeBasicKeys(ChromaticKey global_key,Source source);
    static Keys makeBasicKeysFromText(const QString& global_key_text,Source source);
    static QString getPreferredVersion();
    static QString getPreferredSubVersion(const QHash<QString, QString>& extraVersionInfo);
    static Keys makePreferredKeys(
        const KeyChangeList& key_changes,
        const QHash<QString, QString>& extraVersionInfo,
        const int iSampleRate, const int iTotalSamples);
};
