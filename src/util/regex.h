_Pragma("once")
#include <QRegExp>
#include <QStringList>
#include <QString>

class RegexUtils {
  public:
    static QString fileExtensionsRegex(QStringList extensions);
  private:
    RegexUtils() = delete;
};
