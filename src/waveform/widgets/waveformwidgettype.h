_Pragma("once")
class WaveformWidgetType {
  public:
    enum Type {
        EmptyWaveform = 0,
        SoftwareSimpleWaveform, //TODO
        SoftwareWaveform,
        QtSimpleWaveform,
        QtWaveform,
        HSVWaveform,
        RGBWaveform,
        Count_WaveformwidgetType // Also used as invalid value
    };
};
