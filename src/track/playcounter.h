_Pragma("once")
#include "util/assert.h"


// Counts the total number of times a track has been played
// and if the track has been played during the current session.
class PlayCounter {
public:
    explicit PlayCounter(int timesPlayed = 0);
    // Sets total number of times a track has been played
    void setTimesPlayed(int iTimesPlayed);
    // Returns the total number of times a track has been played
    int getTimesPlayed() const;
    // Sets the played status of a track for the current session
    // without affecting the play count.
    void setPlayed(bool bPlayed = true);
    // Returns true if track has been played during the current session
    bool isPlayed() const;
    // Sets the played status of a track for the current session and
    // increments or decrements the total play count accordingly.
    void setPlayedAndUpdateTimesPlayed(bool bPlayed = true);
private:
    int m_iTimesPlayed{0};
    bool m_bPlayed{false};
};
bool operator==(const PlayCounter& lhs, const PlayCounter& rhs);
bool operator!=(const PlayCounter& lhs, const PlayCounter& rhs);
