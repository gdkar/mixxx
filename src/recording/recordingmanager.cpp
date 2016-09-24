// Created 03/26/2011 by Tobias Rafreider

#include <QMutex>
#include <QDir>
#include <QtDebug>

#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "engine/enginemaster.h"
#include "engine/sidechain/enginerecord.h"
#include "engine/sidechain/enginesidechain.h"
#include "errordialoghandler.h"
#include "recording/defs_recording.h"
#include "recording/recordingmanager.h"

RecordingManager::RecordingManager(UserSettingsPointer pConfig, EngineMaster* pEngine)
        : m_pConfig(pConfig),
          m_recordingDir(""),
          m_recording_base_file(""),
          m_recordingFile(""),
          m_recordingLocation(""),
          m_bRecording(false),
          m_iNumberOfBytesRecorded(0),
          m_split_size(0),
          m_iNumberSplits(0),
          m_durationRecorded("")
{
    m_pToggleRecording = new ControlPushButton(ConfigKey(RECORDING_PREF_KEY, "toggle_recording"),this);
    connect(m_pToggleRecording, SIGNAL(valueChanged(double)),
            this, SLOT(slotToggleRecording(double)));
    m_recReadyCO = new ControlObject(ConfigKey(RECORDING_PREF_KEY, "status"),this);
    m_recReady = new ControlProxy(m_recReadyCO->getKey(), this);

    m_split_size = getFileSplitSize();


    // Register EngineRecord with the engine sidechain.
    auto pSidechain = pEngine->getSideChain();
    if (pSidechain) {
        auto pEngineRecord = new EngineRecord(m_pConfig);
        connect(pEngineRecord, SIGNAL(isRecording(bool, bool)),
                this, SLOT(slotIsRecording(bool, bool)));
        connect(pEngineRecord, SIGNAL(bytesRecorded(int)),
                this, SLOT(slotBytesRecorded(int)));
        connect(pEngineRecord, SIGNAL(durationRecorded(QString)),
                this, SLOT(slotDurationRecorded(QString)));
        pSidechain->addSideChainWorker(pEngineRecord);
    }
}

RecordingManager::~RecordingManager()
{
    qDebug() << "Delete RecordingManager";
    delete m_recReadyCO;
    delete m_pToggleRecording;
}
QString RecordingManager::formatDateTimeForFilename(QDateTime dateTime) const
{
    // Use a format based on ISO 8601. Windows does not support colons in
    // filenames so we can't use them anywhere.
    return dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");
}
void RecordingManager::slotSetRecording(bool recording)
{
    if (recording && !isRecordingActive()) {
        startRecording();
    } else if (!recording && isRecordingActive()) {
        stopRecording();
    }
}
void RecordingManager::slotToggleRecording(double v)
{
    if (v > 0) {
        if (isRecordingActive()) {
            stopRecording();
        } else {
            startRecording();
        }
    }
}
void RecordingManager::startRecording(bool generateFileName)
{
    m_iNumberOfBytesRecorded = 0;
    m_split_size = getFileSplitSize();
    qDebug() << "Split size is:" << m_split_size;
    auto encodingType = m_pConfig->getValueString(ConfigKey(RECORDING_PREF_KEY, "Encoding"));

    if(generateFileName) {
        m_iNumberSplits = 1;
        // Append file extension.
        auto date_time_str = formatDateTimeForFilename(QDateTime::currentDateTime());
        m_recordingFile = QString("%1.%2")
                .arg(date_time_str, encodingType.toLower());

        // Storing the absolutePath of the recording file without file extension.
        m_recording_base_file = getRecordingDir();
        m_recording_base_file.append("/").append(date_time_str);
        // Appending file extension to get the filelocation.
        m_recordingLocation = m_recording_base_file + "."+ encodingType.toLower();
        m_pConfig->set(ConfigKey(RECORDING_PREF_KEY, "Path"), m_recordingLocation);
        m_pConfig->set(ConfigKey(RECORDING_PREF_KEY, "CuePath"), m_recording_base_file +".cue");
    } else {
        // This is only executed if filesplit occurs.
        ++m_iNumberSplits;
        auto new_base_filename = m_recording_base_file +"part"+QString::number(m_iNumberSplits);
        m_recordingLocation = new_base_filename + "." +encodingType.toLower();

        m_pConfig->set(ConfigKey(RECORDING_PREF_KEY, "Path"), m_recordingLocation);
        m_pConfig->set(ConfigKey(RECORDING_PREF_KEY, "CuePath"), new_base_filename +".cue");
        m_recordingFile = QFileInfo(m_recordingLocation).fileName();
    }
    m_recReady->set(RECORD_READY);
}
void RecordingManager::stopRecording()
{
    qDebug() << "Recording stopped";
    m_recReady->set(RECORD_OFF);
    m_recordingFile = "";
    m_recordingLocation = "";
    m_iNumberOfBytesRecorded = 0;
}
void RecordingManager::setRecordingDir()
{
    QDir recordDir(m_pConfig->getValueString(
        ConfigKey(RECORDING_PREF_KEY, "Directory")));
    // Note: the default ConfigKey for recordDir is set in DlgPrefRecord::DlgPrefRecord.
    if (!recordDir.exists()) {
        if (recordDir.mkpath(recordDir.absolutePath())) {
            qDebug() << "Created folder" << recordDir.absolutePath() << "for recordings";
        } else {
            qDebug() << "Failed to create folder" << recordDir.absolutePath() << "for recordings";
        }
    }
    m_recordingDir = recordDir.absolutePath();
    qDebug() << "Recordings folder set to" << m_recordingDir;
}
QString& RecordingManager::getRecordingDir()
{
    // Update current recording dir from preferences.
    setRecordingDir();
    return m_recordingDir;
}
// Only called when recording is active.
void RecordingManager::slotDurationRecorded(QString durationStr)
{
    if(m_durationRecorded != durationStr)
    {
        m_durationRecorded = durationStr;
        emit(durationRecorded(m_durationRecorded));
    }
}
// Only called when recording is active.
void RecordingManager::slotBytesRecorded(int bytes)
{
    // auto conversion to long
    m_iNumberOfBytesRecorded += bytes;
    if(m_iNumberOfBytesRecorded >= m_split_size)
    {
        stopRecording();
        // Dont generate a new filename.
        // This will reuse the previous filename but append a suffix.
        startRecording(false);
    }
    emit(bytesRecorded(m_iNumberOfBytesRecorded));
}
void RecordingManager::slotIsRecording(bool isRecordingActive, bool error)
{
    //qDebug() << "SlotIsRecording " << isRecording << error;

    // Notify the GUI controls, see dlgrecording.cpp.
    m_bRecording = isRecordingActive;
    emit(isRecording(isRecordingActive));

    if (error) {
        auto props = ErrorDialogHandler::instance()->newDialogProperties();
        props->setType(DLG_WARNING);
        props->setTitle(tr("Recording"));
        props->setText("<html>"+tr("Could not create audio file for recording!")
                       +"<p>"+tr("Ensure there is enough free disk space and you have write permission for the Recordings folder.")
                       +"<p>"+tr("You can change the location of the Recordings folder in Preferences > Recording.")
                       +"</p></html>");
        ErrorDialogHandler::instance()->requestErrorDialog(props);
    }
}
bool RecordingManager::isRecordingActive()
{
    return m_bRecording;
}
QString& RecordingManager::getRecordingFile()
{
    return m_recordingFile;
}
QString& RecordingManager::getRecordingLocation()
{
    return m_recordingLocation;
}
long RecordingManager::getFileSplitSize()
{
     auto fileSizeStr = m_pConfig->getValueString(ConfigKey(RECORDING_PREF_KEY, "FileSize"));
     if(fileSizeStr == SPLIT_650MB)         return SIZE_650MB;
     else if(fileSizeStr == SPLIT_700MB)    return SIZE_700MB;
     else if(fileSizeStr == SPLIT_1024MB)   return SIZE_1GB;
     else if(fileSizeStr == SPLIT_2048MB)   return SIZE_2GB;
     else if(fileSizeStr == SPLIT_4096MB)   return SIZE_4GB;
     else                                   return SIZE_650MB;
}
