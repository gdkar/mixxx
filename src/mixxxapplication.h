_Pragma("once")
#include <QApplication>
class ControlObject;
class MixxxApplication : public QApplication {
    Q_OBJECT
  public:
    MixxxApplication(int& argc, char** argv);
    virtual ~MixxxApplication();
  private:
    bool touchIsRightButton();
    int m_fakeMouseSourcePointId = 0;
    QWidget* m_fakeMouseWidget = nullptr;
    enum Qt::MouseButton m_activeTouchButton = Qt::NoButton;
    ControlObject* m_pTouchShift = nullptr;
};

