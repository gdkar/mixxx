#include "engine/enginefilteriir.h"

EngineFilterIIRBase::EngineFilterIIRBase(QObject *pParent):EngineObject(pParent),
m_doRamping(true),
m_doStart(true){}
EngineFilterIIRBase::~EngineFilterIIRBase(){}
void EngineFilterIIRBase::pauseFilterInner() {
    // Set the current buffers to 0
    memset(m_buf1, 0, sizeof(m_buf1));
    memset(m_buf2, 0, sizeof(m_buf2));
    m_doRamping = true;
    m_doStart = true;
}
void EngineFilterIIRBase::pauseFilter(){
  if(!m_doStart)pauseFilterInner();
}
void EngineFilterIIRBase::setCoefs(int size, double sampleRate,const char* spec,
            double freq0, double freq1, int adj) {
        char spec_d[FIDSPEC_LENGTH];
        if (strlen(spec) < sizeof(spec_d)) {
            // Copy to dynamic-ish memory to prevent fidlib API breakage.
            strcpy(spec_d, spec);
            // Copy the old coefficients into m_oldCoef
            memcpy(m_oldCoef, m_coef, sizeof(m_coef));
            double coef[MAX_SIZE+1];
            coef[0] = fid_design_coef(coef + 1, size, spec_d, sampleRate, freq0, freq1, adj);
            for(auto i = 0; i < size+1;i++) m_coef[i] = coef[i];
            initBuffers();
#if(IIR_ANALYSIS)
            char* desc;
            FidFilter* filt = fid_design(spec_d, sampleRate, freq0, freq1, adj, &desc);
            int delay = fid_calc_delay(filt);
            qDebug() << QString().fromAscii(desc) << "delay:" << delay;
            double resp0, phase0;
            resp0 = fid_response_pha(filt, freq0 / sampleRate, &phase0);
            qDebug() << "freq0:" << freq0 << resp0 << phase0;
            if (freq1) {
                double resp1, phase1;
                resp1 = fid_response_pha(filt, freq1 / sampleRate, &phase1);
                qDebug() << "freq1:" << freq1 << resp1 << phase1;
            }
            double resp2, phase2;
            resp2 = fid_response_pha(filt, freq0 / sampleRate / 2, &phase2);
            qDebug() << "freq2:" << freq0 / 2 << resp2 << phase0;
            double resp3, phase3;
            resp3 = fid_response_pha(filt, freq0 / sampleRate * 2, &phase3);
            qDebug() << "freq3:" << freq0 * 2 << resp3 << phase0;
            double resp4, phase4;
            resp4 = fid_response_pha(filt, freq0 / sampleRate / 2.2, &phase4);
            qDebug() << "freq4:" << freq0 / 2.2 << resp2 << phase0;
            double resp5, phase5;
            resp5 = fid_response_pha(filt, freq0 / sampleRate * 2.2, &phase5);
            qDebug() << "freq5:" << freq0 * 2.2 << resp3 << phase0;
            free(filt);
#endif
        }
    }

void EngineFilterIIRBase::setCoefs2(int size,double sampleRate, int n_coef1,
            const char* spec1, double freq01, double freq11, int adj1,
            const char* spec2, double freq02, double freq12, int adj2) {
        char spec1_d[FIDSPEC_LENGTH];
        char spec2_d[FIDSPEC_LENGTH];
        if (strlen(spec1) < sizeof(spec1_d) && strlen(spec2) < sizeof(spec2_d)) {
            // Copy to dynamic-ish memory to prevent fidlib API breakage.
            strcpy(spec1_d, spec1);
            strcpy(spec2_d, spec2);

            // Copy the old coefficients into m_oldCoef
            memcpy(m_oldCoef, m_coef, sizeof(m_coef));
            double coef[MAX_SIZE+1];
            coef[0] = fid_design_coef(coef + 1, n_coef1,spec1, 
                        sampleRate, freq01, freq11, adj1) *
                    fid_design_coef(coef + 1 + n_coef1, size - n_coef1,spec2, 
                        sampleRate, freq02, freq12, adj2);
            for(auto i = 0; i < size+1;i++) m_coef[i] = coef[i];
            initBuffers();

#if(IIR_ANALYSIS)
            char* desc1;
            char* desc2;
            FidFilter* filt1 = fid_design(spec1, sampleRate, freq01, freq11, adj1, &desc1);
            FidFilter* filt2 = fid_design(spec2, sampleRate, freq02, freq12, adj2, &desc2);
            FidFilter* filt = fid_cat(1, filt1, filt2, nullptr);
            int delay = fid_calc_delay(filt);
            qDebug() << QString().fromAscii(desc1) << "X" << QString().fromAscii(desc2) << "delay:" << delay;
            double resp0, phase0;
            resp0 = fid_response_pha(filt, freq01 / sampleRate, &phase0);
            qDebug() << "freq01:" << freq01 << resp0 << phase0;
            resp0 = fid_response_pha(filt, freq01 / sampleRate, &phase0);
            qDebug() << "freq02:" << freq02 << resp0 << phase0;
            if (freq11) {
                double resp1, phase1;
                resp1 = fid_response_pha(filt, freq11 / sampleRate, &phase1);
                qDebug() << "freq1:" << freq11 << resp1 << phase1;
            }
            if (freq12) {
                double resp1, phase1;
                resp1 = fid_response_pha(filt, freq12 / sampleRate, &phase1);
                qDebug() << "freq1:" << freq12 << resp1 << phase1;
            }
            double resp2, phase2;
            resp2 = fid_response_pha(filt, freq01 / sampleRate / 2, &phase2);
            qDebug() << "freq2:" << freq01 / 2 << resp2 << phase0;
            double resp3, phase3;
            resp3 = fid_response_pha(filt, freq01 / sampleRate * 2, &phase3);
            qDebug() << "freq3:" << freq01 * 2 << resp3 << phase0;
            double resp4, phase4;
            resp4 = fid_response_pha(filt, freq01 / sampleRate / 2.2, &phase4);
            qDebug() << "freq4:" << freq01 / 2.2 << resp2 << phase0;
            double resp5, phase5;
            resp5 = fid_response_pha(filt, freq01 / sampleRate * 2.2, &phase5);
            qDebug() << "freq5:" << freq01 * 2.2 << resp3 << phase0;
            free(filt);
#endif
        }
    }
void EngineFilterIIRBase::initBuffers() {
        // Copy the current buffers into the old buffers
        memcpy(m_oldBuf1, m_buf1, sizeof(m_buf1));
        memcpy(m_oldBuf2, m_buf2, sizeof(m_buf2));
        // Set the current buffers to 0
        memset(m_buf1, 0, sizeof(m_buf1));
        memset(m_buf2, 0, sizeof(m_buf2));
        m_doRamping = true;
    }
void EngineFilterIIRBase::processAndPauseFilter(const CSAMPLE* pIn, CSAMPLE* pOutput,const int iBufferSize) {
        process(pIn, pOutput, iBufferSize);
        SampleUtil::copy2WithRampingGain(pOutput,pOutput,1.f,0.f,pIn,0.f,m_startFromDry?1.f:0.f,iBufferSize);
        pauseFilterInner();
    }
void EngineFilterIIRBase::assumeSettled(){
  m_doRamping = false;
  m_doStart   = false;
}
