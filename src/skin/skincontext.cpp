#include <QtDebug>
#include <QStringList>
#include <QJSValue>
#include <QAction>
#include <QJSValueIterator>

#include "skin/skincontext.h"
#include "util/cmdlineargs.h"
#include "widget/wsingletoncontainer.h"

SkinContext::SkinContext(ConfigObject<ConfigValue>* pConfig, const QString& xmlPath)
        : m_xmlPath(xmlPath),
          m_pConfig(pConfig),
          m_pScriptEngine(new QJSEngine()),
          m_context(m_pScriptEngine->newObject()),
          m_pSingletons(new SingletonMap)
{
    enableDebugger(true);
    // the extensions are imported once and will be passed to the children
    // global object as properties of the parent's global object.
    importScriptExtension("console");
    importScriptExtension("svg");
    m_pScriptEngine->installTranslatorFunctions();
    auto context = m_pScriptEngine->globalObject();
    m_context.setPrototype(context);
}
void SkinContext::defineSingleton(QString objectName, QWidget* widget)
{
  m_pSingletons->insertSingleton(objectName,widget);
}
QWidget* SkinContext::getSingletonWidget(QString objectName) const
{
    return m_pSingletons->getSingletonWidget(objectName);
}
QString SkinContext::getSkinPath(const QString& relativePath) const
{
    return QDir(m_skinBasePath).filePath(relativePath);
}
SkinContext::SkinContext(const SkinContext& parent)
        : m_xmlPath(parent.m_xmlPath),
          m_skinBasePath(parent.m_skinBasePath),
          m_pConfig(parent.m_pConfig),
          m_pScriptEngine(parent.m_pScriptEngine),
          m_context(m_pScriptEngine->newObject()),
          m_pSingletons(parent.m_pSingletons)
{
    // we generate a new global object to preserve the scope between
    // a context and its children
    m_context.setPrototype(parent.m_context);
}
SkinContext::~SkinContext() = default;
QString SkinContext::variable(const QString& name) const
{
  auto value = m_context.property(name);
  if ( value.isNull() || value.isUndefined() ) return QString{};
  else return value.toString();
}
void SkinContext::setVariable(const QString& name, const QString& value)
{
    m_context.setProperty(name,value);
}
void SkinContext::setXmlPath(const QString& xmlPath)
{
  m_xmlPath = xmlPath;
}
void SkinContext::updateVariables(const QDomNode& node)
{
    auto child = node.firstChildElement(QString{"SetVariable"});
    while (!child.isNull())
    {
        updateVariable(child.toElement());
        child = child.nextSiblingElement(QString{"SetVariable"});
    }
}
void SkinContext::updateVariable(const QDomElement& element)
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
bool SkinContext::hasNode(const QDomNode& node, const QString& nodeName) const
{
    return !selectNode(node, nodeName).isNull();
}
QDomNode SkinContext::selectNode(const QDomNode& node,const QString& nodeName) const
{
    auto child = node.firstChild();
    while (!child.isNull())
    {
        if (child.nodeName() == nodeName) return child;
        child = child.nextSibling();
    }
    return child;
}
QDomElement SkinContext::selectElement(const QDomNode& node,const QString& nodeName) const
{
    return node.firstChildElement(nodeName);
}
QString SkinContext::selectString(const QDomNode& node,const QString& nodeName) const
{
    return nodeToString(selectElement(node, nodeName));
}
float SkinContext::selectFloat(const QDomNode& node,const QString& nodeName) const
{
    auto ok = false;
    auto  conv = nodeToString(selectElement(node, nodeName)).toFloat(&ok);
    return ok ? conv : 0.0f;
}
double SkinContext::selectDouble(const QDomNode& node,const QString& nodeName) const
{
    auto ok = false;
    auto conv = nodeToString(selectElement(node, nodeName)).toDouble(&ok);
    return ok ? conv : 0.0;
}
int SkinContext::selectInt(const QDomNode& node,const QString& nodeName,bool* pOk) const
{
    auto ok = false;
    auto conv = nodeToString(selectElement(node, nodeName)).toInt(&ok);
    if (pOk) {*pOk = ok;}
    return ok ? conv : 0;
}
bool SkinContext::selectBool(const QDomNode& node,const QString& nodeName,bool defaultValue) const
{
    auto child = selectNode(node, nodeName);
    if (!child.isNull()) return nodeToString(child).contains("true",Qt::CaseInsensitive);
    return defaultValue;
}
bool SkinContext::hasNodeSelectString(const QDomNode& node,const QString& nodeName, QString *value) const
{
    auto child = selectNode(node, nodeName);
    if (!child.isNull())
    {
        *value = nodeToString(child);
        return true;
    }
    return false;
}
bool SkinContext::hasNodeSelectBool(const QDomNode& node,const QString& nodeName, bool *value) const {
    auto child = selectNode(node, nodeName);
    if (!child.isNull())
    {
        *value =  nodeToString(child).contains("true",Qt::CaseInsensitive);;
        return true;
    }
    return false;
}
bool SkinContext::selectAttributeBool(const QDomElement& element,const QString& attributeName,bool defaultValue) const
{
    if (element.hasAttribute(attributeName))
    {
        return element.attribute(attributeName).contains("true",Qt::CaseInsensitive);
    }
    return defaultValue;
}
QString SkinContext::selectAttributeString(const QDomElement& element,const QString& attributeName,QString defaultValue) const
{
    if (element.hasAttribute(attributeName)) {
        auto value = element.attribute(attributeName);
        if ( !value.isEmpty() ) return value;
    }
    return defaultValue;
}
QString SkinContext::variableNodeToText(const QDomElement& variableNode) const
{
    if (variableNode.hasAttribute("expression"))
    {
        auto result = m_pScriptEngine->evaluate(variableNode.attribute("expression"), m_xmlPath,variableNode.lineNumber());
        return result.toString();
    }
    else if (variableNode.hasAttribute("name"))
    {
        auto variableName = variableNode.attribute("name");
        if (variableNode.hasAttribute("format"))
        {
            auto formatString = variableNode.attribute("format");
            return formatString.arg(variable(variableName));
        }
        else if (variableNode.nodeName() == "SetVariable")
        {
            // If we are setting the variable name and we didn't get a format
            // string then return the node text. Use nodeToString to translate
            // embedded variable references.
            return nodeToString(variableNode);
        }
        else return variable(variableName);
    }
    return nodeToString(variableNode);
}
QString SkinContext::nodeToString(const QDomNode& node) const
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
PixmapSource SkinContext::getPixmapSource(const QDomNode& pixmapNode) const
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
Paintable::DrawMode SkinContext::selectScaleMode(const QDomElement& element,Paintable::DrawMode defaultDrawMode) const
{
    auto drawModeStr = selectAttributeString( element, "scalemode", Paintable::DrawModeToString(defaultDrawMode));
    return Paintable::DrawModeFromString(drawModeStr);
}
/**
 * All the methods below exist to access some of the scriptEngine features
 * from the svgParser.
 */
QJSValue SkinContext::evaluateScript(const QString& expression,const QString& filename,int lineNumber)
{
    qDebug() << "Evaluating script expression " << expression << " from file " << filename << " line " << lineNumber;
    return m_pScriptEngine->evaluate(expression, filename, lineNumber);
}
QJSValue SkinContext::importScriptExtension(const QString& extensionName)
{
/*    auto out = m_pScriptEngine->importExtension(extensionName);
    if ( out.isError() )
    {
      qDebug() << "Error importing extension " << out.property("name").toString() << " 
    }
    if (m_pScriptEngine->hasUncaughtException()) qDebug() << out.toString();
    return out;*/
    return QJSValue{};
}
QSharedPointer<QJSEngine> SkinContext::getScriptEngine() const
{
    return m_pScriptEngine;
}
void SkinContext::enableDebugger(bool state) const
{
    if (CmdlineArgs::Instance().getDeveloper() && m_pConfig  && m_pConfig->getValueString(ConfigKey("ScriptDebugger", "Enabled")) == "1")
    {
/*        if (state) m_pScriptDebugger->attachTo(m_pScriptEngine.data());
        else       m_pScriptDebugger->detach();*/
    }
}
QDebug SkinContext::logWarning(const char* file, const int line,const QDomNode& node) const
{
    return qWarning() << QString("%1:%2 SKIN ERROR at %3:%4 <%5>:")
                             .arg(file, QString::number(line), m_xmlPath,
                                  QString::number(node.lineNumber()),
                                  node.nodeName())
                             .toUtf8()
                             .constData();
}
