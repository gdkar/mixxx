_Pragma("once")
#include <QFontDatabase>
#include <QString>
#include <QtDebug>

#include "util/cmdlineargs.h"

class FontUtils {
  public:
    static bool addFont(const QString& path) {
        int result = QFontDatabase::addApplicationFont(path);
        if (result == -1) {
            qWarning() << "Failed to add font:" << path;
            return false;
        }
        // In developer mode, spit out all the families / styles / sizes
        // supported by the new font.
        if (CmdlineArgs::Instance().getDeveloper()) {
            QFontDatabase database;
            auto families = QFontDatabase::applicationFontFamilies(result);
            for(auto & family: families) {
                auto styles = database.styles(family);
                for(auto& style: styles) {
                    auto pointSizes = database.pointSizes(family, style);
                    QStringList pointSizesStr;
                    for(auto point: pointSizes) {pointSizesStr.append(QString::number(point));}
                    qDebug() << "FONT LOADED family:" << family
                             << "style:" << style
                             << "point sizes:" << pointSizesStr.join(",");
                }
            }
        }
        return true;
    }
  private:
    FontUtils() = delete;
};
