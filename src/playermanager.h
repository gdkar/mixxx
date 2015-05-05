// playermanager.h
// Created 6/1/2010 by RJ Ryan (rryan@mit.edu)

#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <QList>
#include <QMutex>

#include "configobject.h"
#include "track/trackinfoobject.h"

class ControlObject;
class Deck;
class TrackPlayer;

class Library;
class EngineMaster;
class AnalyserQueue;
class SoundManager;
class EffectsManager;
class TrackCollection;

class PlayerManager : public QObject{
    Q_OBJECT
  public:
    explicit PlayerManager( ConfigObject<ConfigValue>* pConfig,
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
    // Add a TrackPlayer to the PlayerManager
    void addPreviewDeck();
    // Return the number of players. Thread-safe.
    static unsigned int numDecks();
    unsigned int numberOfDecks() const {return numDecks();}
    // Returns true if the group is a deck group. If index is non-NULL,
    // populates it with the deck number (1-indexed).
    static bool isDeckGroup(const QString& group, int* number=NULL);
    // Returns true if the group is a preview deck group. If index is non-NULL,
    // populates it with the deck number (1-indexed).
    static bool isPreviewDeckGroup(const QString& group, int* number=NULL);
    // Return the number of samplers. Thread-safe.
    static unsigned int numSamplers();
    unsigned int numberOfSamplers() const {return numSamplers();}
    // Return the number of preview decks. Thread-safe.
    static unsigned int numPreviewDecks();
    unsigned int numberOfPreviewDecks() const {return numPreviewDecks();}
    // Get a TrackPlayer (i.e. a Deck or a Sampler) by its group
    TrackPlayer* getPlayer(QString group) const;
    // Get the deck by its deck number. Decks are numbered starting with 1.
    TrackPlayer* getDeck(unsigned int player) const;
    TrackPlayer* getPreviewDeck(unsigned int libPreviewPlayer) const;
    // Get the sampler by its number. Samplers are numbered starting with 1.
    TrackPlayer* getSampler(unsigned int sampler) const;
    // Binds signals between PlayerManager and Library. Does not store a pointer
    // to the Library.
    void bindToLibrary(Library* pLibrary);
    // Returns the group for the ith sampler where i is zero indexed
    static QString groupForSampler(int i) {return QString("[Sampler%1]").arg(i+1);}
    // Returns the group for the ith deck where i is zero indexed
    static QString groupForDeck(int i) {return QString("[Channel%1]").arg(i+1);}
    // Returns the group for the ith TrackPlayer where i is zero indexed
    static QString groupForPreviewDeck(int i) {return QString("[PreviewDeck%1]").arg(i+1);}
    // Used to determine if the user has configured an input for the given vinyl deck.
    bool hasVinylInput(int inputnum) const;
  public slots:
    // ons for loading tracks into a Player, which is either a Sampler or a Deck
    void onLoadTrackToPlayer(TrackPointer pTrack, QString group, bool play = false);
    void onLoadToPlayer(QString location, QString group);
    // ons for loading tracks to decks
    void onLoadTrackIntoNextAvailableDeck(TrackPointer pTrack);
    // Loads the location to the deck. deckNumber is 1-indexed
    void onLoadToDeck(QString location, int deckNumber);
    // Loads the location to the preview deck. previewDeckNumber is 1-indexed
    void onLoadToPreviewDeck(QString location, int previewDeckNumber);
    // ons for loading tracks to samplers
    void onLoadTrackIntoNextAvailableSampler(TrackPointer pTrack);
    // Loads the location to the sampler. samplerNumber is 1-indexed
    void onLoadToSampler(QString location, int samplerNumber);
    void onNumDecksControlChanged(double v);
    void onNumSamplersControlChanged(double v);
    void onNumPreviewDecksControlChanged(double v);
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
    ConfigObject<ConfigValue>* m_pConfig;
    SoundManager* m_pSoundManager;
    EffectsManager* m_pEffectsManager;
    EngineMaster  *m_pEngine;
    AnalyserQueue* m_pAnalyserQueue;
    ControlObject* m_pCONumDecks;
    ControlObject* m_pCONumSamplers;
    ControlObject* m_pCONumPreviewDecks;
    QList<TrackPlayer*> m_decks;
    QList<TrackPlayer*> m_samplers;
    QList<TrackPlayer*> m_preview_decks;
    QMap<QString, TrackPlayer*> m_players;
};

#endif // PLAYERMANAGER_H
