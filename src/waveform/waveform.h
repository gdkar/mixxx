_Pragma("once")
#include <QMutex>
#include <QByteArray>
#include <QString>
#include <QSharedPointer>
#include <QMutexLocker>
#include <vector>
#include <atomic>

#include "util.h"

enum FilterIndex { Low = 0, Mid = 1, High = 2, FilterCount = 3};
enum ChannelIndex { Left = 0, Right = 1, ChannelCount = 2};

union WaveformData {
    struct {
        unsigned char low;
        unsigned char mid;
        unsigned char high;
        unsigned char all;
    } filtered;
    int m_i;
    WaveformData();
    WaveformData(int i);
};
class Waveform {
  public:
    explicit Waveform(const QByteArray pData = QByteArray());
    Waveform(int audioSampleRate, int audioSamples,int desiredVisualSampleRate, int maxVisualSamples);
    virtual ~Waveform();
    Waveform(const Waveform &)=delete;
    Waveform&operator=(const Waveform&)=delete;
    Waveform(Waveform&&)=default;
    Waveform&operator=(Waveform&&)=default;
    int getId() const;
    void setId(int id);
    QString getVersion() const;
    void setVersion(QString version);
    QString getDescription() const;
    void setDescription(QString description);
    QByteArray toByteArray() const;
    // We do not lock the mutex since m_dataSize and m_visualSampleRate are not
    // changed after the constructor runs.
    bool isValid() const; 
    bool isDirty() const;
    // AnalysisDAO needs to be able to set the waveform as clean so we mark this
    // as const and m_bDirty mutable.
    void setDirty(bool bDirty) const;
    // We do not lock the mutex since m_audioVisualRatio is not changed after
    // the constructor runs.
    double getAudioVisualRatio() const;
    // Atomically lookup the completion of the waveform. Represents the number
    // of data elements that have been processed out of dataSize.
    int getCompletion() const;
    void setCompletion(int completion);
    // We do not lock the mutex since m_textureStride is not changed after
    // the constructor runs.
    int getTextureStride() const;

    // We do not lock the mutex since m_data is not resized after the
    // constructor runs.
    int getTextureSize() const;
    // Atomically get the number of data elements in this Waveform. We do not
    // lock the mutex since m_dataSize is not changed after the constructor
    // runs.
    int getDataSize() const;
    const WaveformData& get(int i) const;
    unsigned char getLow(int i) const;
    unsigned char getMid(int i) const;
    unsigned char getHigh(int i) const;
    unsigned char getAll(int i) const;
    // We do not lock the mutex since m_data is not resized after the
    // constructor runs.
    WaveformData* data();
    // We do not lock the mutex since m_data is not resized after the
    // constructor runs.
    const WaveformData* data() const;
    void dump() const;
    WaveformData& at(int i);
    unsigned char& low(int i);
    unsigned char& mid(int i);
    unsigned char& high(int i);
    unsigned char& all(int i);
    double getVisualSampleRate() const;
  private:
    void readByteArray(const QByteArray& data);
    void resize(int size);
    void assign(int size, int value = 0);
    // If stored in the database, the ID of the waveform.
    std::atomic<int> m_id{-1};
    // AnalysisDAO needs to be able to set the waveform as clean.
    mutable std::atomic<bool> m_bDirty{false};
    QString m_version;
    QString m_description;
    // The size of the waveform data stored in m_data. Not allowed to change
    // after the constructor runs.
    std::atomic<int> m_dataSize{0};
    // The vector storing the waveform data. It is potentially larger than
    // m_dataSize since it includes padding for uploading the entire waveform as
    // a texture in the GLSL renderer. The size is not allowed to change after
    // the constructor runs. We use a std::vector to avoid the cost of bounds
    // checking when accessing the vector.
    // TODO(XXX): In the future we should switch to QVector and use the raw data
    // pointer when performance matters.
    std::vector<WaveformData> m_data;
    // Not allowed to change after the constructor runs.
    std::atomic<double> m_visualSampleRate{0};
    // Not allowed to change after the constructor runs.
    std::atomic<double> m_audioVisualRatio{0};
    // We create an NxN texture out of m_data's buffer in the GLSL renderer. The
    // stride is N. Not allowed to change after the constructor runs.
    std::atomic<int> m_textureStride{0};
    // For performance, completion is shared as a QAtomicInt and does not lock
    // the mutex. The completion of the waveform calculation.
    std::atomic<double> m_completion{0};
    mutable QMutex m_mutex;
};
typedef QSharedPointer<Waveform> WaveformPointer;
typedef QSharedPointer<const Waveform> ConstWaveformPointer;
