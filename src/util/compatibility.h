#ifndef COMPATABILITY_H
#define COMPATABILITY_H

#include <QAtomicInt>
#include <QStringList>

#include <QLocale>
#include <QGuiApplication>
#include <QInputMethod>

inline QLocale inputLocale() {
    // Use the default config for local keyboard
    auto pInputMethod = QGuiApplication::inputMethod();
    return pInputMethod ? pInputMethod->locale() : QLocale(QLocale::English);
}

#endif /* COMPATABILITY_H */
