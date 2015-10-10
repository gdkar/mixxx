_Pragma("once")
#include "library/dao/trackdao.h"
#include "library/dao/playlistdao.h"

// Upgrades your library from 1.7 to the current DB format.
class LegacyLibraryImporter : public QObject {
    Q_OBJECT
    public: 
        LegacyLibraryImporter(TrackDAO& trackDao, PlaylistDAO& playlistDao);
        virtual ~LegacyLibraryImporter();
        void import();
    signals:
        void progress(QString);
    private:
        TrackDAO& m_trackDao;
        PlaylistDAO& m_playlistDao;
};
