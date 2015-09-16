#include "util/compatibility.h"
#include <QGuiApplication>
#include <QInputMethod>
QLocale inputLocale()
{
  auto pInputMethod = QGuiApplication::inputMethod();
  return pInputMethod ? pInputMethod->locale() : QLocale{QLocale::English};
}
