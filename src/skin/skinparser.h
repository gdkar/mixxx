_Pragma("once")
#include <QString>
#include <QWidget>

class SkinParser {
  public:
    SkinParser();
    virtual ~SkinParser();
    virtual bool canParse(QString skinPath) = 0;
    virtual QWidget* parseSkin(QString skinPath, QWidget* pParent) = 0;
};
