#ifndef REGEX_H
#define REGEX_H

#include <QRegExp>
#include <QStringList>
#include <QString>
#include <algorithm>
class RegexUtils {
  public:
    static QString fileExtensionsRegex(QStringList extensions) {
        // Escape every extension appropriately
        std::transform(extensions.cbegin(),extensions.cend(),extensions.begin(),
                QRegExp::escape);
        // Turn the list into a "\\.(jpg|gif|etc)$" style regex string
        return QString("\\.(%1)$").arg(extensions.join("|"));
    }
  private:
    RegexUtils() = delete;
};


#endif /* REGEX_H */
