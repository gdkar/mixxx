#include "util/urls.h"

QUrl MixxxUrls::website()
{
  return QUrl{"http://www.mixxx.org"};
}
QUrl MixxxUrls::support()
{
  return QUrl{"http://www.mixxx.org/support/"};
}
QUrl MixxxUrls::feedback()
{
  return QUrl{"https://docs.google.com/a/mixxx.org/forms/d/1raQ0WnrdVJYSE5r2tsV4jIRd90oZVrF-TRKWeW2kfEU/viewform"};

}
QUrl MixxxUrls::translation()
{
  return QUrl{"https://www.transifex.com/projects/p/mixxxdj/"};
}
QUrl MixxxUrls::manual()
{
  return QUrl{"http://mixxx.org/manual/1.12/"};
}
QUrl MixxxUrls::shortcuts()
{
  return QUrl{"http://mixxx.org/manual/1.12/chapters/appendix.html#keyboard-mapping-table"};;
}
QString MixxxUrls::manual_filename()
{
  return QString{"Mixxx-Manual.pdf"};
}
