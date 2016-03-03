_Pragma("once")
#include <QObject>
#include <QString>

#include "preferences/usersettings.h"

class SettingsManager : public QObject {
    Q_OBJECT
  public:
    SettingsManager(QObject* pParent, const QString& settingsPath);
    virtual ~SettingsManager();
    UserSettingsPointer settings() const;
    void save();
    bool shouldRescanLibrary();
  private:
    void initializeDefaults();
    UserSettingsPointer m_pSettings;
    bool m_bShouldRescanLibrary;
};
