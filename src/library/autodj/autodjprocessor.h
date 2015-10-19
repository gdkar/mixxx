_Pragma("once")
#include <QObject>
#include <QString>
#include <QModelIndexList>

#include "util.h"
#include "trackinfoobject.h"
#include "configobject.h"
#include "library/playlisttablemodel.h"
#include "engine/enginechannel.h"

class ControlPushButton;
class ControlObject;
class TrackCollection;
class PlayerManager;
class Player;

class DeckAttributes : public QObject {
    Q_OBJECT
  public:
    DeckAttributes(int index,
                   Player* pPlayer,
                   EngineChannel::ChannelOrientation orientation);
    virtual ~DeckAttributes();
    bool isLeft() const;
    bool isRight() const;
    bool isPlaying() const;
    void stop();
    void play();
    double playPosition() const;
    void setPlayPosition(double playpos);
    bool isRepeat() const;
    void setRepeat(bool enabled);
    TrackPointer getLoadedTrack() const;
  signals:
    void playChanged(DeckAttributes* deck, bool playing);
    void playPositionChanged(DeckAttributes* deck, double playPosition);
    void trackLoaded(DeckAttributes* deck, TrackPointer pTrack);
    void trackLoadFailed(DeckAttributes* deck, TrackPointer pTrack);
    void trackUnloaded(DeckAttributes* deck, TrackPointer pTrack);

  private slots:
    void slotPlayPosChanged(double v);
    void slotPlayChanged(double v);
    void slotTrackLoaded(TrackPointer pTrack);
    void slotTrackLoadFailed(TrackPointer pTrack);
    void slotTrackUnloaded(TrackPointer pTrack);

  public:
    int index;
    QString group;
    double posThreshold;
    double fadeDuration;

  private:
    EngineChannel::ChannelOrientation m_orientation;
    ControlObject* m_playPos;
    ControlObject* m_play;
    ControlObject* m_repeat;
    Player* m_pPlayer;
};
class AutoDJProcessor : public QObject {
    Q_OBJECT
  public:
    enum AutoDJState {
        ADJ_IDLE = 0,
        ADJ_P1FADING,
        ADJ_P2FADING,
        ADJ_ENABLE_P1LOADED,
        ADJ_ENABLE_P1PLAYING,
        ADJ_DISABLED
    };

    enum AutoDJError {
        ADJ_OK = 0,
        ADJ_IS_INACTIVE,
        ADJ_QUEUE_EMPTY,
        ADJ_BOTH_DECKS_PLAYING,
        ADJ_DECKS_3_4_PLAYING,
        ADJ_NOT_TWO_DECKS
    };
    Q_ENUMS(AutoDJState);
    Q_ENUMS(AutoDJError);
    AutoDJProcessor(QObject* pParent,
                    ConfigObject<ConfigValue>* pConfig,
                    PlayerManager* pPlayerManager,
                    int iAutoDJPlaylistId,
                    TrackCollection* pCollection);
    virtual ~AutoDJProcessor();
    int getState() const;
    int getTransitionTime() const;
    PlaylistTableModel* getTableModel() const;
  public slots:
    virtual void setTransitionTime(int seconds);
    virtual AutoDJError shufflePlaylist(const QModelIndexList& selectedIndices);
    virtual AutoDJError skipNext();
    virtual AutoDJError fadeNow();
    virtual AutoDJError toggleAutoDJ(bool enable);
  signals:
    void loadTrackToPlayer(TrackPointer pTrack, QString group,bool play);
    void autoDJStateChanged(int state);
    void transitionTimeChanged(int time);
    void randomTrackRequested(int tracksToAdd);
  private slots:
    virtual void playerPositionChanged(DeckAttributes* pDeck, double position);
    virtual void playerPlayChanged(DeckAttributes* pDeck, bool playing);
    virtual void playerTrackLoaded(DeckAttributes* pDeck, TrackPointer pTrack);
    virtual void playerTrackLoadFailed(DeckAttributes* pDeck, TrackPointer pTrack);
    virtual void playerTrackUnloaded(DeckAttributes* pDeck, TrackPointer pTrack);

    virtual void controlEnable(double value);
    virtual void controlFadeNow(double value);
    virtual void controlShuffle(double value);
    virtual void controlSkipNext(double value);

  private:
    // Gets or sets the crossfader position while normalizing it so that -1 is
    // all the way mixed to the left side and 1 is all the way mixed to the
    // right side. (prevents AutoDJ logic from having to check for hamster mode
    // every time)
    double getCrossfader() const;
    void setCrossfader(double value, bool right);

    TrackPointer getNextTrackFromQueue();
    bool loadNextTrackFromQueue(const DeckAttributes& pDeck, bool play = false);
    void calculateTransition(DeckAttributes* pFromDeck,DeckAttributes* pToDeck);
    DeckAttributes* getOtherDeck(DeckAttributes* pFromDeck,bool playing = false);

    // Removes the track loaded to the player group from the top of the AutoDJ
    // queue if it is present.
    bool removeLoadedTrackFromTopOfQueue(const DeckAttributes& deck);

    // Removes the provided track from the top of the AutoDJ queue if it is
    // present.
    bool removeTrackFromTopOfQueue(TrackPointer pTrack);

    ConfigObject<ConfigValue>* m_pConfig;
    PlayerManager* m_pPlayerManager;
    PlaylistTableModel* m_pAutoDJTableModel;

    AutoDJState m_eState;
    int m_iTransitionTime; // the desired value set by the user
    int m_nextTransitionTime; // the tweaked value actually used

    QList<DeckAttributes*> m_decks;

    ControlObject* m_pCOCrossfader;
    ControlObject* m_pCOCrossfaderReverse;

    ControlPushButton* m_pSkipNext;
    ControlPushButton* m_pFadeNow;
    ControlPushButton* m_pShufflePlaylist;
    ControlPushButton* m_pEnabledAutoDJ;

};
