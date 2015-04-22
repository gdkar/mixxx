#include <QtDebug>
#include <QStringList>
#include <QJSValue>
#include <QAction>
#include <QJSValueIterator>

#include "skin/skincontext.h"
#include "skin/svgparser.h"
#include "util/cmdlineargs.h"

SkinContext::SkinContext(ConfigObject<ConfigValue>* pConfig,
                         const QString& xmlPath)
        : m_xmlPath(xmlPath),
          m_pConfig(pConfig),
          m_pScriptEngine(new QQmlEngine()),
          m_pSingletons(new SingletonMap) {
    QQmlContext *newContext = new QQmlContext(m_pScriptEngine.data(),this);
    QQmlEngine::setContextForObject(this,newContext);
    QJSValueIterator it(m_pScriptEngine->globalObject());
    while (it.hasNext()) {
        it.next();
        newContext->setContextProperty(it.name(), it.value());
    }
    //m_pScriptEngine->setGlobalObject(newGlobal);

    for (QHash<QString, QString>::const_iterator it = m_variables.begin();
         it != m_variables.end(); ++it) {
        newContext->setContextProperty(it.key(), it.value());
    }

    enableDebugger(true);
    // the extensions are imported once and will be passed to the children
    // global object as properties of the parent's global object.
//    importScriptExtension("console");
//    importScriptExtension("svg");
    m_pScriptEngine->installTranslatorFunctions();
}

SkinContext::SkinContext(const SkinContext& parent)
        : m_xmlPath(parent.m_xmlPath),
          m_skinBasePath(parent.m_skinBasePath),
          m_pConfig(parent.m_pConfig),
          m_variables(parent.variables()),
          m_pScriptEngine(parent.m_pScriptEngine),
          m_pScriptDebugger(parent.m_pScriptDebugger),
          m_parentGlobal(m_pScriptEngine->globalObject()),
          m_pSingletons(parent.m_pSingletons) {

    // we generate a new global object to preserve the scope between
    // a context and its children
//    QJSValue context = m_pScriptEngine->pushContext()->activationObject();
//    while (it.hasNext()) {
//        it.next();
//        newGlobal.setProperty(it.name(), it.value());
//    }
//    m_pScriptEngine->setGlobalObject(newGlobal);

    for (QHash<QString, QString>::const_iterator it = m_variables.begin();
         it != m_variables.end(); ++it) {
        m_pScriptEngine->globalObject().setProperty(it.key(), it.value());
    }
}

SkinContext::~SkinContext() {
}

QString SkinContext::variable(const QString& name) const {
    return m_variables.value(name, QString());
}

void SkinContext::setVariable(const QString& name, const QString& value) {
    m_variables[name] = value;
    m_pContext->setContextProperty(name, value);
}

void SkinContext::setXmlPath(const QString& xmlPath) {
    m_xmlPath = xmlPath;
}

void SkinContext::updateVariables(const QDomNode& node) {
    QDomNode child = node.firstChild();
    while (!child.isNull()) {
        if (child.isElement() && child.nodeName() == "SetVariable") {
            updateVariable(child.toElement());
        }
        child = child.nextSibling();
    }
}

void SkinContext::updateVariable(const QDomElement& element) {
    if (!element.hasAttribute("name")) {
        qDebug() << "Can't update variable without name:" << element.text();
        return;
    }
    QString name = element.attribute("name");
    QString value = variableNodeToText(element);
    setVariable(name, value);
}

bool SkinContext::hasNode(const QDomNode& node, const QString& nodeName) const {
    return !selectNode(node, nodeName).isNull();
}

QDomNode SkinContext::selectNode(const QDomNode& node,
                                 const QString& nodeName) const {
    QDomNode child = node.firstChild();
    while (!child.isNull()) {
        if (child.nodeName() == nodeName) {
            return child;
        }
        child = child.nextSibling();
    }
    return child;
}

QDomElement SkinContext::selectElement(const QDomNode& node,
                                       const QString& nodeName) const {
    QDomNode child = selectNode(node, nodeName);
    return child.toElement();
}

QString SkinContext::selectString(const QDomNode& node,
                                  const QString& nodeName) const {
    QDomElement child = selectElement(node, nodeName);
    return nodeToString(child);
}

float SkinContext::selectFloat(const QDomNode& node,
                               const QString& nodeName) const {
    bool ok = false;
    float conv = nodeToString(selectElement(node, nodeName)).toFloat(&ok);
    return ok ? conv : 0.0f;
}

double SkinContext::selectDouble(const QDomNode& node,
                                 const QString& nodeName) const {
    bool ok = false;
    double conv = nodeToString(selectElement(node, nodeName)).toDouble(&ok);
    return ok ? conv : 0.0;
}

int SkinContext::selectInt(const QDomNode& node,
                           const QString& nodeName,
                           bool* pOk) const {
    bool ok = false;
    int conv = nodeToString(selectElement(node, nodeName)).toInt(&ok);
    if (pOk != NULL) {
        *pOk = ok;
    }
    return ok ? conv : 0;
}

bool SkinContext::selectBool(const QDomNode& node,
                             const QString& nodeName,
                             bool defaultValue) const {
    QDomNode child = selectNode(node, nodeName);
    if (!child.isNull()) {
         QString stringValue = nodeToString(child);
        return stringValue.contains("true", Qt::CaseInsensitive);
    }
    return defaultValue;
}

bool SkinContext::hasNodeSelectString(const QDomNode& node,
                                      const QString& nodeName, QString *value) const {
    QDomNode child = selectNode(node, nodeName);
    if (!child.isNull()) {
        *value = nodeToString(child);
        return true;
    }
    return false;
}

bool SkinContext::hasNodeSelectBool(const QDomNode& node,
                                    const QString& nodeName, bool *value) const {
    QDomNode child = selectNode(node, nodeName);
    if (!child.isNull()) {
         QString stringValue = nodeToString(child);
        *value = stringValue.contains("true", Qt::CaseInsensitive);
        return true;
    }
    return false;
}

bool SkinContext::selectAttributeBool(const QDomElement& element,
                                      const QString& attributeName,
                                      bool defaultValue) const {
    if (element.hasAttribute(attributeName)) {
        QString stringValue = element.attribute(attributeName);
        return stringValue.contains("true", Qt::CaseInsensitive);
    }
    return defaultValue;
}

QString SkinContext::selectAttributeString(const QDomElement& element,
                                           const QString& attributeName,
                                           QString defaultValue) const {
    if (element.hasAttribute(attributeName)) {
        QString stringValue = element.attribute(attributeName);
        return stringValue == "" ? defaultValue :stringValue;
    }
    return defaultValue;
}

QString SkinContext::variableNodeToText(const QDomElement& variableNode) const {
    if (variableNode.hasAttribute("expression")) {
        QJSValue result = m_pScriptEngine->evaluate(
            variableNode.attribute("expression"), m_xmlPath,
            variableNode.lineNumber());
        return result.toString();
    } else if (variableNode.hasAttribute("name")) {
        QString variableName = variableNode.attribute("name");
        if (variableNode.hasAttribute("format")) {
            QString formatString = variableNode.attribute("format");
            return formatString.arg(variable(variableName));
        } else if (variableNode.nodeName() == "SetVariable") {
            // If we are setting the variable name and we didn't get a format
            // string then return the node text. Use nodeToString to translate
            // embedded variable references.
            return nodeToString(variableNode);
        } else {
            return variable(variableName);
        }
    }
    return nodeToString(variableNode);
}

QString SkinContext::nodeToString(const QDomNode& node) const {
    QStringList result;
    QDomNode child = node.firstChild();
    while (!child.isNull()) {
        if (child.isElement()) {
            if (child.nodeName() == "Variable") {
                result.append(variableNodeToText(child.toElement()));
            } else {
                qDebug() << "Unhandled tag in node:" << child.nodeName();
            }
        } else if (child.isText()) {
            result.append(child.nodeValue());
        }
        // Ignore all other node types.
        child = child.nextSibling();
    }
    return result.join("");
}

PixmapSource SkinContext::getPixmapSource(const QDomNode& pixmapNode) const {
    PixmapSource source;

    const SvgParser svgParser(*this);

    if (!pixmapNode.isNull()) {
        QDomNode svgNode = selectNode(pixmapNode, "svg");
        if (!svgNode.isNull()) {
            // inline svg
            const QByteArray rslt = svgParser.saveToQByteArray(
                svgParser.parseSvgTree(svgNode, m_xmlPath));
            source.setSVG(rslt);
        } else {
            // filename
            QString pixmapName = nodeToString(pixmapNode);
            if (!pixmapName.isEmpty()) {
                source.setPath(getSkinPath(pixmapName));
                if (source.isSVG()) {
                    const QByteArray rslt = svgParser.saveToQByteArray(
                            svgParser.parseSvgFile(source.getPath()));
                    source.setSVG(rslt);
                }
            }
        }
    }

    return source;
}

Paintable::DrawMode SkinContext::selectScaleMode(
        const QDomElement& element,
        Paintable::DrawMode defaultDrawMode) const {
    QString drawModeStr = selectAttributeString(
            element, "scalemode", Paintable::DrawModeToString(defaultDrawMode));
    return Paintable::DrawModeFromString(drawModeStr);
}

/**
 * All the methods below exist to access some of the scriptEngine features
 * from the svgParser.
 */
QJSValue  SkinContext::evaluateScript(const QString& expression,
                                         const QString& filename,
                                         int lineNumber) {
    return m_pScriptEngine->evaluate(expression, filename, lineNumber);
}

QJSValue SkinContext::importScriptExtension(const QString& extensionName) {
    //QJSValue out = m_pScriptEngine->importExtension(extensionName);
//    if (m_pScriptEngine->hasUncaughtException()) {
//        qDebug() << out.toString();
//    }
    return QJSValue(false);
}

const QSharedPointer<QQmlEngine> SkinContext::getScriptEngine() const {
    return m_pScriptEngine;
}

void SkinContext::enableDebugger(bool state) const {
}

QDebug SkinContext::logWarning(const char* file, const int line,
                               const QDomNode& node) const {
    return qWarning() << QString("%1:%2 SKIN ERROR at %3:%4 <%5>:")
                             .arg(file, QString::number(line), m_xmlPath,
                                  QString::number(node.lineNumber()),
                                  node.nodeName())
                             .toUtf8()
                             .constData();
}
