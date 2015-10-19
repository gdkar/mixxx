#include <QtDebug>
#include <QStringList>
#include <QJSValue>
#include <QAction>
#include <QJSValueIterator>

#include "skin/skincontext.h"
#include "util/cmdlineargs.h"
#include "widget/wsingletoncontainer.h"

SkinContext::SkinContext(ConfigObject<ConfigValue>* pConfig, QString xmlPath)
        : m_xmlPath(xmlPath),
          m_pConfig(pConfig),
          m_pSingletons(new SingletonMap)
{
}
void SkinContext::defineSingleton(QString objectName, QWidget* widget)
{
  m_pSingletons->insertSingleton(objectName,widget);
}
QWidget* SkinContext::getSingletonWidget(QString objectName) const
{
    return m_pSingletons->getSingletonWidget(objectName);
}
QString SkinContext::getSkinPath(QString relativePath) const
{
    return QDir(m_skinBasePath).filePath(relativePath);
}
SkinContext::SkinContext(const SkinContext& parent)
        : m_xmlPath(parent.m_xmlPath),
          m_skinBasePath(parent.m_skinBasePath),
          m_pConfig(parent.m_pConfig),
          m_variables(parent.m_variables),
          m_pSingletons(parent.m_pSingletons)
{
}
SkinContext::~SkinContext() = default;
QString SkinContext::variable(QString name) const
{
  if ( m_variables.contains(name) ) return m_variables.value(name);
  else                              return QString{};
}
void SkinContext::setVariable(QString name, QString value)
{
      m_variables.insert(name,value);
}
void SkinContext::setXmlPath(QString xmlPath)
{
  m_xmlPath = xmlPath;
}
void SkinContext::updateVariables(QDomNode node)
{
    auto child = node.firstChildElement(QString{"SetVariable"});
    while (!child.isNull())
    {
        updateVariable(child.toElement());
        child = child.nextSiblingElement(QString{"SetVariable"});
    }
}
void SkinContext::updateVariable(QDomElement element)
{
    if (!element.hasAttribute("name"))
    {
        qDebug() << "Can't update variable without name:" << element.text();
        return;
    }
    auto name = element.attribute("name");
    auto value = variableNodeToText(element);
    setVariable(name, value);
}
bool SkinContext::hasNode(QDomNode node, QString nodeName) const
{
    return !selectNode(node, nodeName).isNull();
}
QDomNode SkinContext::selectNode(QDomNode node,QString nodeName) const
{
    auto child = node.firstChild();
    while (!child.isNull())
    {
        if (child.nodeName() == nodeName) return child;
        child = child.nextSibling();
    }
    return child;
}
QDomElement SkinContext::selectElement(QDomNode node,QString nodeName) const
{
    return node.firstChildElement(nodeName);
}
QString SkinContext::selectString(QDomNode node,QString nodeName) const
{
    return nodeToString(selectElement(node, nodeName));
}
float SkinContext::selectFloat(QDomNode node,QString nodeName) const
{
    auto ok = false;
    auto  conv = nodeToString(selectElement(node, nodeName)).toFloat(&ok);
    return ok ? conv : 0.0f;
}
double SkinContext::selectDouble(QDomNode node,QString nodeName) const
{
    auto ok = false;
    auto conv = nodeToString(selectElement(node, nodeName)).toDouble(&ok);
    return ok ? conv : 0.0;
}
int SkinContext::selectInt(QDomNode node,QString nodeName,bool* pOk) const
{
    auto ok = false;
    auto conv = nodeToString(selectElement(node, nodeName)).toInt(&ok);
    if (pOk) {*pOk = ok;}
    return ok ? conv : 0;
}
bool SkinContext::selectBool(QDomNode node,QString nodeName,bool defaultValue) const
{
    auto child = selectNode(node, nodeName);
    if (!child.isNull()) return nodeToString(child).contains("true",Qt::CaseInsensitive);
    return defaultValue;
}
bool SkinContext::hasNodeSelectString(QDomNode node,QString nodeName, QString *value) const
{
    auto child = selectNode(node, nodeName);
    if (!child.isNull())
    {
        *value = nodeToString(child);
        return true;
    }
    return false;
}
bool SkinContext::hasNodeSelectBool(QDomNode node,QString nodeName, bool *value) const {
    auto child = selectNode(node, nodeName);
    if (!child.isNull())
    {
        *value =  nodeToString(child).contains("true",Qt::CaseInsensitive);;
        return true;
    }
    return false;
}
bool SkinContext::selectAttributeBool(QDomElement element,QString attributeName,bool defaultValue) const
{
    if (element.hasAttribute(attributeName)) return element.attribute(attributeName).contains("true",Qt::CaseInsensitive);
    return defaultValue;
}
QString SkinContext::selectAttributeString(QDomElement element,QString attributeName,QString defaultValue) const
{
    if (element.hasAttribute(attributeName))
    {
        auto value = element.attribute(attributeName);
        if ( !value.isEmpty() ) return value;
    }
    return defaultValue;
}
QString SkinContext::variableNodeToText(QDomElement variableNode) const
{
    if (variableNode.hasAttribute("name"))
    {
        auto variableName = variableNode.attribute("name");
        if (variableNode.hasAttribute("format"))
        {
            auto formatString = variableNode.attribute("format");
            return formatString.arg(variable(variableName));
        }
        else if (variableNode.nodeName() == "SetVariable")
        {
            return nodeToString(variableNode);
        }
        else return variable(variableName);
    }
    return nodeToString(variableNode);
}
QString SkinContext::nodeToString(QDomNode node) const
{
    QStringList result;
    auto child = node.firstChild();
    while (!child.isNull())
    {
        if (child.isElement())
        {
            if (child.nodeName() == "Variable") result.append(variableNodeToText(child.toElement()));
            else                                qDebug() << "Unhandled tag in node:" << child.nodeName();
        }
        else if (child.isText()) result.append(child.nodeValue());
        // Ignore all other node types.
        child = child.nextSibling();
    }
    return result.join("");
}
PixmapSource SkinContext::getPixmapSource(QDomNode pixmapNode) const
{
    PixmapSource source;
    if (!pixmapNode.isNull())
    {
        // filename
        auto pixmapName = nodeToString(pixmapNode);
        if (!pixmapName.isEmpty()) source.setPath(getSkinPath(pixmapName));
    }
    return source;
}
Paintable::DrawMode SkinContext::selectScaleMode(QDomElement element,Paintable::DrawMode defaultDrawMode) const
{
    auto drawModeStr = selectAttributeString( element, "scalemode", Paintable::DrawModeToString(defaultDrawMode));
    return Paintable::DrawModeFromString(drawModeStr);
}
QDebug SkinContext::logWarning(const char* file, const int line,QDomNode node) const
{
    return qWarning() << QString("%1:%2 SKIN ERROR at %3:%4 <%5>:")
                             .arg(file, QString::number(line), m_xmlPath,
                                  QString::number(node.lineNumber()),
                                  node.nodeName())
                             .toUtf8()
                             .constData();
}
