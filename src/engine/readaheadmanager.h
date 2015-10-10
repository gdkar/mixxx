// readaheadmanager.h
// Created 8/2/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QLinkedList>
#include <QList>
#include <QPair>
#include <QObject>

#include "util/types.h"
#include "util/math.h"
#include "cachingreader.h"

class LoopingControl;
class RateControl;
class CachingReader;

// ReadAheadManager is a tool for keeping track of the engine's current position
// in a file. In the case that the engine needs to read ahead of the current
// play position (for example, to feed more samples into a library like
// SoundTouch) then this will keep track of how many samples the engine has
// consumed. The getNextSamples() method encapsulates the logic of determining
// whether to take a loop or jump into a single method. Whenever the Engine
// seeks or the current play position is invalidated somehow, the Engine must
// call notifySeek to inform the ReadAheadManager to reset itself to the seek
// point.
class ReadAheadManager : public QObject{
  Q_OBJECT
  public:
    explicit ReadAheadManager(QObject *pParent); // Only for testing: ReadAheadManagerMock
    explicit ReadAheadManager(CachingReader* reader, LoopingControl* pLoopingControl, QObject *pParent);
    virtual ~ReadAheadManager();

    // Call this method to fill buffer with requested_samples out of the
    // lookahead buffer. Provide rate as dRate so that the manager knows the
    // direction the audio is progressing in. Returns the total number of
    // samples read into buffer. Note that it is very common that the total
    // samples read is less than the requested number of samples.
    virtual int getNextSamples(double dRate, CSAMPLE* buffer, int requested_samples);


    // Used to add a new EngineControls that ReadAheadManager will use to decide
    // which samples to return.
    void addLoopingControl();
    void addRateControl(RateControl* pRateControl);

    // Get the current read-ahead position in samples.
    virtual inline int getPlaypos() const {
        return m_iCurrentPosition;
    }

    virtual void notifySeek(int iSeekPosition);

    // hintReader allows the ReadAheadManager to provide hints to the reader to
    // indicate that the given portion of a song is about to be read.
    virtual void hintReader(double dRate, HintVector* hintList);

    virtual int getEffectiveVirtualPlaypositionFromLog(double currentVirtualPlayposition,
                                                       double numConsumedSamples);

    virtual void setReader(CachingReader* pReader) {
        m_pReader = pReader;
    }

    // An entry in the read log indicates the virtual playposition the read
    // began at and the virtual playposition it ended at.
    struct ReadLogEntry {
        double m_vpps;
        double m_vppe;
        ReadLogEntry() = default;
        ReadLogEntry(double vpps,
                     double vppe);
        bool direction() const;
        double length() const;
        double consume(double numSamples);
        bool merge(const ReadLogEntry& other);
    };
  private:
    // virtualPlaypositionEnd is the first sample in the direction that was read
    // that was NOT read as part of this log entry. This is to simplify the
    void addReadLogEntry(double vpps,double vppe);
    LoopingControl* m_pLoopingControl = nullptr;
    RateControl* m_pRateControl = nullptr;
    QLinkedList<ReadLogEntry> m_readAheadLog;
    int m_iCurrentPosition = 0;
    CachingReader* m_pReader    = nullptr;
    CSAMPLE* m_pCrossFadeBuffer = nullptr;
};
Q_DECLARE_TYPEINFO(ReadAheadManager::ReadLogEntry,Q_PRIMITIVE_TYPE);
