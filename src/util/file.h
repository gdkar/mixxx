_Pragma("once")
#include <QFile>
#include <QDir>

#include "util/sandbox.h"

class MFile {
  public:
    MFile();
    MFile(const QString& name);
    MFile(const MFile& other);
    virtual ~MFile();
    QFile& file();
    const QFile& file() const;
    SecurityTokenPointer token();
    bool canAccess();
    MFile& operator=(const MFile& other);
  private:
    QString m_fileName;
    QFile m_file;
    SecurityTokenPointer m_pSecurityToken;
};
class MDir {
  public:
    MDir();
    MDir(const QString& name);
    MDir(const MDir& other);
    virtual ~MDir();
    QDir& dir();
    const QDir& dir() const;
    SecurityTokenPointer token();
    bool canAccess();
    MDir& operator=(const MDir& other);
  private:
    QString m_dirPath;
    QDir m_dir;
    SecurityTokenPointer m_pSecurityToken;
};
