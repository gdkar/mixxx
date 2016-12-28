// readaheadmanager.h
// Created 8/2/2009 by RJ Ryan (rryan@mit.edu)

#ifndef READAHEADMANGER_H
#define READAHEADMANGER_H

#include <QLinkedList>
#include <QList>
#include <QPair>

#include "util/types.h"
#include "util/math.h"
#include "engine/cachingreader.h"

class LoopingControl;
class RateControl;

// ReadAheadManager is a tool for keeping track of the engine's current position
// in a file. In the case that the engine needs to read ahead of the current
// play position (for example, to feed more samples into a library like
// SoundTouch) then this will keep track of how many samples the engine has
// consumed. The getNextSamples() method encapsulates the logic of determining
// whether to take a loop or jump into a single method. Whenever the Engine
// seeks or the current play position is invalidated somehow, the Engine must
// call notifySeek to inform the ReadAheadManager to reset itself to the seek
// point.
class ReadAheadManager : public QObject {
    Q_OBJECT
  public:
    ReadAheadManager(QObject *pParent = nullptr); // Only for testing: ReadAheadManagerMock
    ReadAheadManager(CachingReader* reader,
                              LoopingControl* pLoopingControl, QObject *pParent = nullptr);
    virtual ~ReadAheadManager();

    // Call this method to fill buffer with requested_samples out of the
    // lookahead buffer. Provide rate as dRate so that the manager knows the
    // direction the audio is progressing in. Returns the total number of
    // samples read into buffer. Note that it is very common that the total
    // samples read is less than the requested number of samples.
    virtual SINT getNextSamples(double dRate, CSAMPLE* buffer, SINT requested_samples);
    // Used to add a new EngineControls that ReadAheadManager will use to decide
    // which samples to return.
    void addLoopingControl();
    void addRateControl(RateControl* pRateControl);
    // Get the current read-ahead position in samples.
    // unused in Mixxx, but needed for testing
    virtual double getPlaypos() const;
    virtual void notifySeek(double seekPosition);
    // hintReader allows the ReadAheadManager to provide hints to the reader to
    // indicate that the given portion of a song is about to be read.
    virtual void hintReader(double dRate, HintVector* hintList);
    virtual double getFilePlaypositionFromLog(double currentFilePlayposition,
                                                       double numConsumedSamples);
    virtual void setReader(CachingReader* pReader);
  private:
    // An entry in the read log indicates the virtual playposition the read
    // began at and the virtual playposition it ended at.
    struct ReadLogEntry {
        double m_pts_beg{};
        double m_pts_end{};
        constexpr ReadLogEntry() = default;
        constexpr ReadLogEntry(double _beg, double _end)
        : m_pts_beg{_beg} , m_pts_end{_end} {}
        constexpr bool direction() const { return m_pts_beg <= m_pts_end; }
        constexpr bool empty() const { return m_pts_beg == m_pts_end;}
        constexpr double length()  const { return std::abs(m_pts_end - m_pts_beg);}
        double advancePlayposition(double &count);
        bool   merge(const ReadLogEntry& other);
    };

    // virtualPlaypositionEnd is the first sample in the direction that was read
    // that was NOT read as part of this log entry. This is to simplify the
    void addReadLogEntry(double virtualPlaypositionStart,
                         double virtualPlaypositionEndNonInclusive);

    LoopingControl* m_pLoopingControl;
    RateControl* m_pRateControl;
    QLinkedList<ReadLogEntry> m_readAheadLog;
    double m_currentPosition;
    CachingReader* m_pReader;
    CSAMPLE* m_pCrossFadeBuffer;
};

#endif // READAHEADMANGER_H
