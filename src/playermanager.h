// playermanager.h
// Created 6/1/2010 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QList>
#include <QMutex>

#include "configobject.h"
#include "trackinfoobject.h"

class ControlObject;
class Player;

class Library;
class EngineMaster;
class AnalyserQueue;
class SoundManager;
class EffectsManager;
class TrackCollection;

class PlayerManager : public QObject{
    Q_OBJECT
  public:
    PlayerManager(ConfigObject<ConfigValue>* pConfig,
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
    // Add a Player to the PlayerManager
    void addPreviewDeck();
    // Return the number of players. Thread-safe.
    static unsigned int numDecks();
    unsigned int numberOfDecks() const;
    // Returns true if the group is a deck group. If index is non-NULL,
    // populates it with the deck number (1-indexed).
    static bool isDeckGroup(const QString& group, int* number=nullptr);
    // Returns true if the group is a preview deck group. If index is non-NULL,
    // populates it with the deck number (1-indexed).
    static bool isPreviewDeckGroup(const QString& group, int* number=nullptr);
    // Return the number of samplers. Thread-safe.
    static unsigned int numSamplers();
    unsigned int numberOfSamplers() const;
    // Return the number of preview decks. Thread-safe.
    static unsigned int numPreviewDecks();
    unsigned int numberOfPreviewDecks() const;
    // Get a Player (i.e. a Player or a Player) by its group
    Player* getPlayer(QString group) const;
    // Get the deck by its deck number. Decks are numbered starting with 1.
    Player* getDeck(unsigned int player) const;
    Player* getPreviewDeck(unsigned int libPreviewPlayer) const;
    // Get the sampler by its number. Samplers are numbered starting with 1.
    Player* getSampler(unsigned int sampler) const;
    // Binds signals between PlayerManager and Library. Does not store a pointer
    // to the Library.
    void bindToLibrary(Library* pLibrary);
    // Returns the group for the ith sampler where i is zero indexed
    static QString groupForSampler(int i);
    // Returns the group for the ith deck where i is zero indexed
    static QString groupForDeck(int i);
    // Returns the group for the ith Player where i is zero indexed
    static QString groupForPreviewDeck(int i);
    // Used to determine if the user has configured an input for the given vinyl deck.
    bool hasVinylInput(int inputnum) const;
  public slots:
    // Slots for loading tracks into a Player, which is either a Player or a Player
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
  signals:
    void loadLocationToPlayer(QString location, QString group);
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
    // Used to protect access to PlayerManager state across threads.
    mutable QMutex m_mutex;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    SoundManager* m_pSoundManager = nullptr;
    EffectsManager* m_pEffectsManager = nullptr;
    EngineMaster* m_pEngine = nullptr;
    AnalyserQueue* m_pAnalyserQueue = nullptr;
    ControlObject* m_pCONumDecks = nullptr;
    ControlObject* m_pCONumSamplers = nullptr;
    ControlObject* m_pCONumPreviewDecks = nullptr;
    QList<Player*> m_decks;
    QList<Player*> m_samplers;
    QList<Player*> m_preview_decks;
    QMap<QString, Player*> m_players;
};
