_Pragma("once")
#include <cstdio>
#include <fidlib.hpp>

#include "engine/engineobject.h"
#include "util/sample.h"



#define FIDSPEC_LENGTH 40

class EngineFilterIIR : public EngineObjectConstIn {
    Q_OBJECT;
    Q_PROPERTY(IIRPass pass READ getPass WRITE setPass NOTIFY passChanged)
    Q_PROPERTY(QString spec READ getSpec WRITE setSpec NOTIFY specChanged)
    Q_PROPERTY(QString template READ getTemplate WRITE setTemplate NOTIFY templateChanged)
    Q_PROPERTY(bool startFromDry READ getStartFromDry WRITE setStartFromDry NOTIFY startFromDryChanged)
    Q_PROPERTY(size_t size READ getSize NOTIFY sizeChanged)
    Q_PROPERTY(double sampleRate READ getSampleRate WRITE setSampleRate NOTIFY sampleRateChanged);
    Q_PROPERTY(double freq0 READ getFreq0 NOTIFY freq0Changed)
    Q_PROPERTY(double freq1 READ getFreq1 NOTIFY freq1Changed)
  public:
    enum IIRPass {
        LowPass,
        BandPass,
        HighPass,
    };
    Q_ENUM(IIRPass);
signals:
    void passChanged(IIRPass);
    void specChanged(QString);
    void templateChanged(QString);
    void startFromDryChanged(bool);
    void sizeChanged(size_t);
    void sampleRateChanged(double);
    void freq0Changed(double);
    void freq1Changed(double);
protected:
    Fid m_fid;
    size_t                    m_size;
    IIRPass                   m_pass;
    IIRPass                   m_oldPass;
    double                    m_rate;
    double                    m_freq0;
    double                    m_freq1;
    int                       m_adj;
    QString                   m_spec;
    QString                   m_tmpl;
    std::vector<CSAMPLE>      m_coef;
    std::vector<CSAMPLE>      m_buf;
    std::vector<CSAMPLE>      m_oldCoef;
    std::vector<CSAMPLE>      m_oldBuf;

    bool m_doRamping = false;
    // Flag set to true if old filter is invalid
    bool m_doStart = false;
    // Flag set to true if this is a chained filter
    bool m_startFromDry= false;
public:
    EngineFilterIIR(QObject *pParent, size_t _SIZE, IIRPass _m_pass, QString spec, QString tmpl = QString{});
    EngineFilterIIR(size_t _SIZE, IIRPass _m_pass, QString spec, QString tmpl = QString{});
   ~EngineFilterIIR() ;
    void setStartFromDry(bool sfd);
    bool getStartFromDry() const;
    void setSpec(QString _sp);
    QString getSpec() const;
    void setTemplate(QString _tmpl);
    QString getTemplate() const;
    size_t getSize() const;
    IIRPass getPass() const;
    void setPass(IIRPass _pass);
    void setFreq0(double);
    void setFreq1(double);
    double getFreq0() const;
    double getFreq1() const;
    // this can be called continuously for Filters that have own ramping
    // or need no fade when disabling
    Q_INVOKABLE virtual void pauseFilter();
    // this is can be used instead off a final process() call before pause
    // It fades to dry or 0 according to the m_startFromDry parameter
    // it is an alternative for using pauseFillter() calls
    virtual void processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,int iBufferSize);
    virtual void initBuffers();
    Q_INVOKABLE virtual void setCoefs(double sampleRate, size_t n_coef1, QString spec,double freq0, double freq1 = 0, int adj = 0);
    Q_INVOKABLE virtual void setSampleRate(double rate);
    Q_INVOKABLE virtual double getSampleRate() const;
    Q_INVOKABLE virtual void setFrequencyCorners(double rate, double freq0, double freq1 = 0);
    Q_INVOKABLE virtual void assumeSettled();
    virtual void processBuffer(const CSAMPLE *pIn, CSAMPLE *pOut, CSAMPLE *coef, CSAMPLE *buf, size_t size, int count);
    void process(const CSAMPLE* pIn, CSAMPLE* pOutput,int iBufferSize) override;
  protected:
    virtual void pauseFilterInner();
};
