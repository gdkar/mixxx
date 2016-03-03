#include "track/playcounter.h"
void PlayCounter::setPlayedAndUpdateTimesPlayed(bool bPlayed)
{
    // This should never happen if the play counter is used
    // as intended! But since this class provides independent
    // setters for both members we need to check and re-establish
    // the class invariant just in case.
    DEBUG_ASSERT_AND_HANDLE(!m_bPlayed || (0 < m_iTimesPlayed)) {
        // Make sure that the number of times played does
        // not become negative!
        m_iTimesPlayed = 1;
    }
    if (bPlayed) {
        // Increment always
        ++m_iTimesPlayed;
    } else if (m_bPlayed) {
        // Decrement only if already marked as played
        DEBUG_ASSERT(0 < m_iTimesPlayed);
        --m_iTimesPlayed;
    }
    m_bPlayed = bPlayed;
}
PlayCounter::PlayCounter(int timesPlayed)
    : m_iTimesPlayed(timesPlayed)
{
}
// Sets total number of times a track has been played
void PlayCounter::setTimesPlayed(int iTimesPlayed)
{
    DEBUG_ASSERT(0 <= iTimesPlayed);
    m_iTimesPlayed = iTimesPlayed;
}
// Returns the total number of times a track has been played
int PlayCounter::getTimesPlayed() const
{
    return m_iTimesPlayed;
}
// Sets the played status of a track for the current session
// without affecting the play count.
void PlayCounter::setPlayed(bool bPlayed)
{
    m_bPlayed = bPlayed;
}
// Returns true if track has been played during the current session
bool PlayCounter::isPlayed() const {
    return m_bPlayed;
}
bool operator==(const PlayCounter& lhs, const PlayCounter& rhs) {
    return (lhs.getTimesPlayed() == rhs.getTimesPlayed())
            && (lhs.isPlayed() == rhs.isPlayed());
}
bool operator!=(const PlayCounter& lhs, const PlayCounter& rhs)
{
    return !(lhs == rhs);
}
