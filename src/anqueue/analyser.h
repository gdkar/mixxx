_Pragma("once")
#include "util/types.h"
#include "configobject.h"
#include "trackinfoobject.h"
#include <QObject>
#include <QSharedPointer>
/*
 * An Analyser is an object which wants to process an entire song to
 * calculate some kind of metadata about it. This could be bpm, the
 * summary, key or something else crazy. This is to help consolidate the
 * many different threads currently processing the whole track in Mixxx on load.
 *   -- Adam
 */
class TrackInfoObject;
class Analyser : public QObject{
    Q_OBJECT
public:
    virtual bool initialise(TrackPointer tio, int sampleRate, int totalSamples) = 0;
    virtual bool loadStored(TrackPointer tio) const = 0;
    virtual void process(const CSAMPLE* pIn, const int iLen) = 0;
    virtual void cleanup(TrackPointer  tio) = 0;
    virtual void finalise(TrackPointer tio) = 0;
    virtual ~Analyser();
    explicit Analyser(ConfigObject<ConfigValue>*, QObject *);
protected:
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
};
