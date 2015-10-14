// wlibrarytextbrowser.h
// Created 10/23/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QTextBrowser>

#include "library/libraryview.h"

class WLibraryTextBrowser : public QTextBrowser, public LibraryView {
    Q_OBJECT
  public:
    WLibraryTextBrowser(QWidget* parent = nullptr);
    virtual ~WLibraryTextBrowser();
    virtual void onShow();
};
