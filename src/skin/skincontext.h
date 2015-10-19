_Pragma("once")
#include <QHash>
#include <QString>
#include <QDomNode>
#include <QDomElement>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueList>
#include <QDir>
#include <QtDebug>

#include "configobject.h"
#include "skin/pixmapsource.h"
#include "widget/wpixmapstore.h"
#define SKIN_WARNING(node, context) (context).logWarning(__FILE__, __LINE__, (node))
class SingletonMap;
// A class for managing the current context/environment when processing a
// skin. Used hierarchically by LegacySkinParser to create new contexts and
// evaluate skin XML nodes while loading the skin.
class SkinContext {
  public:
    SkinContext(ConfigObject<ConfigValue>* pConfig, QString xmlPath);
    SkinContext(const SkinContext& parent);
    virtual ~SkinContext();
    // Gets a path relative to the skin path.
    QString getSkinPath(QString relativePath) const;
    // Sets the base path used by getSkinPath.
    void setSkinBasePath(QString skinBasePath) { m_skinBasePath = skinBasePath;}
    // Variable lookup and modification methods.
    QString variable(QString name) const;
    void setVariable(QString name, QString value);
    void setXmlPath(QString xmlPath);
    // Updates the SkinContext with all the <SetVariable> children of node.
    void updateVariables(QDomNode node);
    // Updates the SkinContext with 'element', a <SetVariable> node.
    void updateVariable(QDomElement element);
    // Methods for evaluating nodes given the context.
    bool hasNode(QDomNode node, QString nodeName) const;
    QDomNode selectNode(QDomNode node, QString nodeName) const;
    QDomElement selectElement(QDomNode node, QString nodeName) const;
    QString selectString(QDomNode node, QString nodeName) const;
    float selectFloat(QDomNode node, QString nodeName) const;
    double selectDouble(QDomNode node, QString nodeName) const;
    int selectInt(QDomNode node, QString nodeName, bool* pOk=nullptr) const;
    bool selectBool(QDomNode node, QString nodeName, bool defaultValue) const;
    bool hasNodeSelectString(QDomNode node, QString nodeName, QString *value) const;
    bool hasNodeSelectBool(QDomNode node, QString nodeName, bool *value) const;
    bool selectAttributeBool(QDomElement element,QString attributeName,bool defaultValue) const;
    QString selectAttributeString(QDomElement element,QString attributeName,QString defaultValue) const;
    QString nodeToString(QDomNode node) const;
    PixmapSource getPixmapSource(QDomNode pixmapNode) const;
    Paintable::DrawMode selectScaleMode(QDomElement element,Paintable::DrawMode defaultDrawMode) const;
    QDebug logWarning(const char* file, const int line, QDomNode node) const;
    void defineSingleton(QString objectName, QWidget* widget);
    QWidget* getSingletonWidget(QString objectName) const;
  private:
    QString variableNodeToText(QDomElement element) const;
    QString m_xmlPath;
    QString m_skinBasePath;
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    QHash<QString,QString>    m_variables;
    // The SingletonContainer map is passed to child SkinContexts, so that all
    // templates in the tree can share a single map.
    QSharedPointer<SingletonMap> m_pSingletons;
};
