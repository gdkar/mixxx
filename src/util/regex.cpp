#include "util/regex.h"

QString RegexUtils::fileExtensionsRegex(QStringList extensions)
{
    // Escape every extension appropriately
    for (auto i = 0; i < extensions.size(); ++i) extensions[i] = QRegExp::escape(extensions[i]);
    // Turn the list into a "\\.(jpg|gif|etc)$" style regex string
    return QString("\\.(%1)$").arg(extensions.join("|"));
}

