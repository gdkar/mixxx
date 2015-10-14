// readaheadmanager.h
// Created 8/2/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QVector>
#include <QList>
#include <QPair>
#include <QObject>
#include "util/types.h"
#include "util/math.h"
#include "cachingreader.h"
#include <memory>

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
  public:
    explicit ReadAheadManager(QObject *p); // Only for testing: ReadAheadManagerMock
    explicit ReadAheadManager(CachingReader* reader,
                              LoopingControl* pLoopingControl,
                              QObject *p);
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
    virtual int getPlaypos() const;
    virtual void notifySeek(int iSeekPosition);
    // hintReader allows the ReadAheadManager to provide hints to the reader to
    // indicate that the given portion of a song is about to be read.
    virtual void hintReader(double dRate, HintVector* hintList);
    virtual int getEffectiveVirtualPlaypositionFromLog(double currentVirtualPlayposition,double numConsumedSamples);
    virtual void setReader(CachingReader* pReader);
    // An entry in the read log indicates the virtual playposition the read
    // began at and the virtual playposition it ended at.
    struct ReadLogEntry {
        double m_start = 0;
        double m_end   = 0;
        ReadLogEntry();
        ReadLogEntry(double start,double end);
        int direction() const;
        double length() const ;
        double consume(double numSamples);
        bool empty() const;
        bool merge(const ReadLogEntry& other);
    };
  private:
    // virtualPlaypositionEnd is the first sample in the direction that was read
    // that was NOT read as part of this log entry. This is to simplify the
    void addReadLogEntry(double start,double end);
    LoopingControl* m_pLoopingControl = nullptr;
    RateControl* m_pRateControl       = nullptr;
    QVector<ReadLogEntry> m_readAheadLog;
    int m_iCurrentPosition;
    CachingReader* m_pReader          = nullptr;
    std::unique_ptr<CSAMPLE[]> m_pCrossFadeBuffer;
};
Q_DECLARE_TYPEINFO(ReadAheadManager::ReadLogEntry,Q_PRIMITIVE_TYPE);
