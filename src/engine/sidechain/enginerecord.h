/***************************************************************************
                          enginerecord.h  -  description
                             -------------------
    copyright            : (C) 2007 by John Sully
    email                :
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
#include <QDataStream>
#include <QFile>

#include "configobject.h"
#include "encoder/encodercallback.h"
#include "engine/sidechain/sidechainworker.h"
#include "trackinfoobject.h"

class ConfigKey;
class ControlObjectSlave;
class Encoder;

class EngineRecord : public QObject, public EncoderCallback, public SideChainWorker {
    Q_OBJECT
  public:
    EngineRecord(ConfigObject<ConfigValue>* _config,QObject *pParent);
    virtual ~EngineRecord();
    void process(const CSAMPLE* pBuffer, int iBufferSize);
    void shutdown() {}
    // writes compressed audio to file 
    int write(unsigned char *data, int length);
    // creates or opens an audio file
    bool openFile();
    // closes the audio file
    void closeFile();
    void updateFromPreferences();
    bool fileOpen();
    bool openCueFile();
    void closeCueFile();
  signals:
    // emitted to notify RecordingManager
    void bytesRecorded(int);
    void isRecording(bool);
    void durationRecorded(QString);
  private:
    int getActiveTracks();
    // Check if the metadata has changed since the previous check. We also check
    // when was the last check performed to avoid using too much CPU and as well
    // to avoid changing the metadata during scratches.
    bool metaDataHasChanged();
    void writeCueLine();
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    Encoder* m_pEncoder = nullptr;
    QByteArray m_OGGquality;
    QByteArray m_MP3quality;
    QByteArray m_encoding;
    QString m_fileName;
    QByteArray m_baTitle;
    QByteArray m_baAuthor;
    QByteArray m_baAlbum;

    QFile m_file;
    QFile m_cueFile;
    QDataStream m_dataStream;

    ControlObjectSlave* m_pRecReady = nullptr;
    ControlObjectSlave* m_pSamplerate = nullptr;
    quint64 m_frames = 0;
    quint64 m_sampleRate = 0;
    quint64 m_recordedDuration = 0;
    QString getRecordedDurationStr();
    int m_iMetaDataLife = 0;
    TrackPointer m_pCurrentTrack{nullptr};
    QByteArray m_cueFileName;
    quint64 m_cueTrack = 0;
    bool m_bCueIsEnabled = false;
};
