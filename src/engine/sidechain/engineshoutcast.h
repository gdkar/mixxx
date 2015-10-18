/***************************************************************************
                          engineshoutcast.h  -  description
                             -------------------
    copyright            : (C) 2007 by John Sully
                           (C) 2007 by Albert Santoni
                           (C) 2007 by Wesley Stessens
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

_Pragma("once")
#include <QObject>
#include <QMessageBox>
#include <QTextCodec>

#include "configobject.h"
#include "encoder/encodercallback.h"
#include "engine/sidechain/sidechainworker.h"
#include "preferences/errordialoghandler.h"
#include "trackinfoobject.h"
#define SHOUTCAST_DISCONNECTED 0
#define SHOUTCAST_CONNECTING 1
#define SHOUTCAST_CONNECTED 2

class Encoder;
class ControlObject;
class ControlObjectSlave;
// Forward declare libshout structures to prevent leaking shout.h definitions
// beyond where they are needed.
struct shout;
typedef struct shout shout_t;
struct _util_dict;
typedef struct _util_dict shout_metadata_t;

class EngineShoutcast : public QObject, public EncoderCallback, public SideChainWorker {
    Q_OBJECT
  public:
    EngineShoutcast(ConfigObject<ConfigValue>* _config);
    virtual ~EngineShoutcast();
    // This is called by the Engine implementation for each sample. Encode and
    // send the stream, as well as check for metadata changes.
    void process(const CSAMPLE* pBuffer, const int iBufferSize);
    void shutdown() { m_bQuit = true; }
    // Called by the encoder in method 'encodebuffer()' to flush the stream to
    // the server.
    int write(unsigned char *data, int bodyLen);
    /** connects to server **/
    bool serverConnect();
    bool serverDisconnect();
    bool isConnected();
  public slots:
    /** Update the libshout struct with info from Mixxx's shoutcast preferences.*/
    void updateFromPreferences();
    //    static void wrapper2writePage();
    //private slots:
    //    void writePage(unsigned char *header, unsigned char *body,
    //                   int headerLen, int bodyLen, int count);
  private:
    int getActiveTracks();
    // Check if the metadata has changed since the previous check.  We also
    // check when was the last check performed to avoid using too much CPU and
    // as well to avoid changing the metadata during scratches.
    bool metaDataHasChanged();
    // Update shoutcast metadata. This does not work for OGG/Vorbis and Icecast,
    // since the actual OGG/Vorbis stream contains the metadata.
    void updateMetaData();
    // Common error dialog creation code for run-time exceptions. Notify user
    // when connected or disconnected and so on
    void errorDialog(QString text, QString detailedError);
    void infoDialog(QString text, QString detailedError);
    QByteArray encodeString(const QString& string);
    QTextCodec* m_pTextCodec                        = nullptr;
    TrackPointer m_pMetaData;
    shout_t *m_pShout                               = nullptr;
    shout_metadata_t *m_pShoutMetaData              = nullptr;
    int m_iMetaDataLife                             = 0;
    long m_iShoutStatus                             = 0;
    long m_iShoutFailures                           = 0;
    ConfigObject<ConfigValue>* m_pConfig            = nullptr;
    Encoder *m_encoder                              = nullptr;
    ControlObject* m_pShoutcastNeedUpdateFromPrefs  = nullptr;
    ControlObjectSlave* m_pUpdateShoutcastFromPrefs = nullptr;
    ControlObjectSlave* m_pMasterSamplerate         = nullptr;
    ControlObject* m_pShoutcastStatus               = nullptr;
    volatile bool m_bQuit         = false;
    // static metadata according to prefereneces
    bool m_custom_metadata        = false;
    QString m_customArtist;
    QString m_customTitle;
    QString m_metadataFormat;

    // when static metadata is used, we only need calling shout_set_metedata
    // once
    bool m_firstCall              = false;

    bool m_format_is_mp3          = false;
    bool m_format_is_ov           = false;
    bool m_protocol_is_icecast1   = false;
    bool m_protocol_is_icecast2   = false;
    bool m_protocol_is_shoutcast  = false;
    bool m_ogg_dynamic_update     = false;
};
