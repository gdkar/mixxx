#ifndef SAMPLERBANK_H
#define SAMPLERBANK_H

#include <QObject>

class ControlObject;
class PlayerManager;

class SamplerBank : public QObject {
    Q_OBJECT

  public:
    SamplerBank(PlayerManager* pPlayerManager);
    virtual ~SamplerBank();

  private slots:
    void onSaveSamplerBank(double v);
    void onLoadSamplerBank(double v);

  private:
    PlayerManager* m_pPlayerManager;
    ControlObject* m_pLoadControl;
    ControlObject* m_pSaveControl;
};

#endif /* SAMPLERBANK_H */
