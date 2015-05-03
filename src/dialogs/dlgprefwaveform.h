#ifndef DLGPREFWAVEFORM_H
#define DLGPREFWAVEFORM_H

#include <QWidget>

#include "dialogs/ui_dlgprefwaveformdlg.h"
#include "configobject.h"
#include "preferences/dlgpreferencepage.h"

class MixxxMainWindow;

class DlgPrefWaveform : public DlgPreferencePage, public Ui::DlgPrefWaveformDlg {
    Q_OBJECT
  public:
    DlgPrefWaveform(QWidget* pParent, MixxxMainWindow* pMixxx,
                    ConfigObject<ConfigValue> *pConfig);
    virtual ~DlgPrefWaveform();

  public slots:
    void onUpdate();
    void onApply();
    void onResetToDefaults();
    void onSetWaveformEndRender(int endTime);

  private slots:
    void onSetFrameRate(int frameRate);
    void onSetWaveformType(int index);
    void onSetWaveformOverviewType(int index);
    void onSetDefaultZoom(int index);
    void onSetZoomSynchronization(bool checked);
    void onSetVisualGainAll(double gain);
    void onSetVisualGainLow(double gain);
    void onSetVisualGainMid(double gain);
    void onSetVisualGainHigh(double gain);
    void onSetNormalizeOverview(bool normalize);
    void onWaveformMeasured(float frameRate, int droppedFrames);

  private:
    void initWaveformControl();

    ConfigObject<ConfigValue>* m_pConfig;
    MixxxMainWindow* m_pMixxx;
};


#endif /* DLGPREFWAVEFORM_H */
