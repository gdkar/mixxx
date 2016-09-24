// playermanager.h
// Created 6/1/2010 by RJ Ryan (rryan@mit.edu)

#ifndef MIXER_PLAYERMANAGER_H
#define MIXER_PLAYERMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QMutex>

#include "preferences/usersettings.h"
#include "track/track.h"

class AnalyzerQueue;
class Auxiliary;
class TrackPlayer;
class ControlObject;
class EffectsManager;
class EngineMaster;
class Library;
class Microphone;
class SoundManager;
class TrackCollection;


// For mocking PlayerManager.
class PlayerManagerInterface {
  public:
    // Get a TrackPlayer (i.e. a Deck or a Sampler) by its group
    virtual TrackPlayer* getPlayer(QString group) const = 0;
    // Get the deck by its deck number. Decks are numbered starting with 1.
    virtual TrackPlayer* getDeck(uint32_t player) const = 0;
    // Returns the number of decks.
    virtual uint32_t numberOfDecks() const = 0;
    // Get the preview deck by its deck number. Preview decks are numbered
    // starting with 1.
    virtual TrackPlayer* getPreviewDeck(uint32_t libPreviewPlayer) const = 0;
    // Returns the number of preview decks.
    virtual uint32_t numberOfPreviewDecks() const = 0;
    // Get the sampler by its number. Samplers are numbered starting with 1.
    virtual TrackPlayer* getSampler(uint32_t sampler) const = 0;
    // Returns the number of sampler decks.
    virtual uint32_t numberOfSamplers() const = 0;
};

class PlayerManager : public QObject, public PlayerManagerInterface {
    Q_OBJECT
  public:
    PlayerManager(UserSettingsPointer pConfig,
                  SoundManager* pSoundManager,
                  EffectsManager* pEffectsManager,
                  EngineMaster* pEngine);
    virtual ~PlayerManager();
    // Add a deck to the PlayerManager
    void addDeck();
    // Add number of decks according to configuration.
    void addConfiguredDecks();
    // Add a sampler to the PlayerManager
    void addSampler();
    // Add a PreviewDeck to the PlayerManager
    void addPreviewDeck();
    // Add a microphone to the PlayerManager
    void addMicrophone();
    // Add an auxiliary input to the PlayerManager
    void addAuxiliary();
    // Return the number of players. Thread-safe.
    static uint32_t numDecks();
    uint32_t numberOfDecks() const {
        return numDecks();
    }
    // Returns true if the group is a deck group. If index is non-NULL,
    // populates it with the deck number (1-indexed).
    static bool isDeckGroup(const QString& group, int* number=NULL);
    // Returns true if the group is a preview deck group. If index is non-NULL,
    // populates it with the deck number (1-indexed).
    static bool isPreviewDeckGroup(const QString& group, int* number=NULL);
    // Return the number of samplers. Thread-safe.
    static uint32_t numSamplers();
    uint32_t numberOfSamplers() const {
        return numSamplers();
    }
    // Return the number of preview decks. Thread-safe.
    static uint32_t numPreviewDecks();
    uint32_t numberOfPreviewDecks() const {
        return numPreviewDecks();
    }
    // Get a TrackPlayer (i.e. a Deck, Sampler or PreviewDeck) by its
    // group. Auxiliaries and microphones are not players.
    TrackPlayer* getPlayer(QString group) const;
    // Get the deck by its deck number. Decks are numbered starting with 1.
    TrackPlayer* getDeck(uint32_t player) const;
    TrackPlayer* getPreviewDeck(uint32_t libPreviewPlayer) const;
    // Get the sampler by its number. Samplers are numbered starting with 1.
    TrackPlayer* getSampler(uint32_t sampler) const;
    // Get the microphone by its number. Microphones are numbered starting with 1.
    Microphone* getMicrophone(uint32_t microphone) const;
    // Get the auxiliary by its number. Auxiliaries are numbered starting with 1.
    Auxiliary* getAuxiliary(uint32_t auxiliary) const;
    // Binds signals between PlayerManager and Library. Does not store a pointer
    // to the Library.
    void bindToLibrary(Library* pLibrary);

    // Returns the group for the ith sampler where i is zero indexed
    static QString groupForSampler(int i) {
        return QString("[Sampler%1]").arg(i+1);
    }

    // Returns the group for the ith deck where i is zero indexed
    static QString groupForDeck(int i) {
        return QString("[Channel%1]").arg(i+1);
    }

    // Returns the group for the ith PreviewDeck where i is zero indexed
    static QString groupForPreviewDeck(int i) {
        return QString("[PreviewDeck%1]").arg(i+1);
    }

    // Returns the group for the ith Microphone where i is zero indexed
    static QString groupForMicrophone(int i) {
        // Before Mixxx had multiple microphone support the first microphone had
        // the group [Microphone]. For backwards compatibility we keep it that
        // way.
        QString group("[Microphone]");
        if (i > 0) {
            group = QString("[Microphone%1]").arg(i + 1);
        }
        return group;
    }
    // Returns the group for the ith Auxiliary where i is zero indexed
    static QString groupForAuxiliary(int i) {
        return QString("[Auxiliary%1]").arg(i + 1);
    }
  public slots:
    // Slots for loading tracks into a Player, which is either a Sampler or a Deck
    void slotLoadTrackToPlayer(TrackPointer pTrack, QString group, bool play = false);
    void slotLoadToPlayer(QString location, QString group);
    // Slots for loading tracks to decks
    void slotLoadTrackIntoNextAvailableDeck(TrackPointer pTrack);
    // Loads the location to the deck. deckNumber is 1-indexed
    void slotLoadToDeck(QString location, int deckNumber);
    // Loads the location to the preview deck. previewDeckNumber is 1-indexed
    void slotLoadToPreviewDeck(QString location, int previewDeckNumber);
    // Slots for loading tracks to samplers
    void slotLoadTrackIntoNextAvailableSampler(TrackPointer pTrack);
    // Loads the location to the sampler. samplerNumber is 1-indexed
    void slotLoadToSampler(QString location, int samplerNumber);

    void slotNumDecksControlChanged(double v);
    void slotNumSamplersControlChanged(double v);
    void slotNumPreviewDecksControlChanged(double v);
    void slotNumMicrophonesControlChanged(double v);
    void slotNumAuxiliariesControlChanged(double v);

  signals:
    void loadLocationToPlayer(QString location, QString group);

    // Emitted when the user tries to enable a microphone talkover control when
    // there is no input configured.
    void noMicrophoneInputConfigured();

    // Emitted when the user tries to enable deck passthrough when there is no
    // input configured.
    void noDeckPassthroughInputConfigured();

    // Emitted when the user tries to enable vinyl control when there is no
    // input configured.
    void noVinylControlInputConfigured();

    // Emitted when the number of decks changes.
    void numberOfDecksChanged(int decks);

  private:
    TrackPointer lookupTrack(QString location);
    // Must hold m_mutex before calling this method. Internal method that
    // creates a new deck.
    void addDeckInner();
    // Must hold m_mutex before calling this method. Internal method that
    // creates a new sampler.
    void addSamplerInner();
    // Must hold m_mutex before calling this method. Internal method that
    // creates a new preview deck.
    void addPreviewDeckInner();
    // Must hold m_mutex before calling this method. Internal method that
    // creates a new microphone.
    void addMicrophoneInner();
    // Must hold m_mutex before calling this method. Internal method that
    // creates a new auxiliary.
    void addAuxiliaryInner();
    // Used to protect access to PlayerManager state across threads.
    mutable QMutex m_mutex;

    UserSettingsPointer m_pConfig;
    SoundManager* m_pSoundManager;
    EffectsManager* m_pEffectsManager;
    EngineMaster* m_pEngine;
    AnalyzerQueue* m_pAnalyzerQueue;
    ControlObject* m_pCONumDecks;
    ControlObject* m_pCONumSamplers;
    ControlObject* m_pCONumPreviewDecks;
    ControlObject* m_pCONumMicrophones;
    ControlObject* m_pCONumAuxiliaries;

    QList<TrackPlayer*> m_decks;
    QList<TrackPlayer*> m_samplers;
    QList<TrackPlayer*> m_preview_decks;
    QList<Microphone*> m_microphones;
    QList<Auxiliary*> m_auxiliaries;
    QMap<QString, TrackPlayer*> m_players;
};

#endif // MIXER_PLAYERMANAGER_H
