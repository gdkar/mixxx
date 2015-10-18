/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

_Pragma("once")
class FFT  
{
public:
    FFT(unsigned int nsamples);
    ~FFT();

    void process(bool inverse,
                 const float *realIn, const float *imagIn,
                 float *realOut, float *imagOut);
    
private:
    unsigned int m_n;
    void *m_private;
};

class FFTReal
{
public:
    FFTReal(unsigned int nsamples);
    ~FFTReal();

    void process(bool inverse,
                 const float *realIn,
                 float *realOut, float *imagOut);

private:
    unsigned int m_n;
    void *m_private;
};    
