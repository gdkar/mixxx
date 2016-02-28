_Pragma("once")
#include <QApplication>
class ScopedOverrideCursor {
  public:
    explicit ScopedOverrideCursor(const QCursor& cursor)
    {
        QApplication::setOverrideCursor(cursor);
        QApplication::processEvents();
    }
    virtual ~ScopedOverrideCursor()
    {
        QApplication::restoreOverrideCursor();
    }
};
class ScopedWaitCursor : public ScopedOverrideCursor {
  public:
    ScopedWaitCursor()
        : ScopedOverrideCursor(Qt::WaitCursor)
    { }
};
