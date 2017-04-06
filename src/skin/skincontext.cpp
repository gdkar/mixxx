#include <QtDebug>
#include <QStringList>
#include <QScriptValue>
#include <QAction>
#include <QScriptValueIterator>

#include "skin/skincontext.h"
#include "skin/svgparser.h"
#include "util/cmdlineargs.h"
#include "util/math.h"

SkinContext::SkinContext(UserSettingsPointer pConfig,
                         const QString& xmlPath)
        : m_xmlPath(xmlPath),
          m_pConfig(pConfig),
          m_pScriptEngine(new QScriptEngine()),
          m_pScriptDebugger(new QScriptEngineDebugger()),
          m_pSvgCache(new QHash<QString, QDomElement>()),
          m_pSingletons(new SingletonMap()) {
    enableDebugger(true);
    // the extensions are imported once and will be passed to the children
    // global object as properties of the parent's global object.
    importScriptExtension("console");
    importScriptExtension("svg");
    m_pScriptEngine->installTranslatorFunctions();

    // Retrieving hooks pattern from script extension
    auto global = m_pScriptEngine->globalObject();
    auto svg = global.property("svg");
    auto hooksPattern = svg.property("getHooksPattern").call(svg);
    if (!hooksPattern.isNull()) {
        m_hookRx.setPattern(hooksPattern.toString());
    }

#if 1 || (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    // Load the config option once, here, rather than in setupSize.
    m_scaleFactor = m_pConfig->getValue(ConfigKey("[Config]","ScaleFactor"), 1.0);
#else
    m_scaleFactor = 1.0;
#endif
}

SkinContext::SkinContext(const SkinContext& parent)
        : m_xmlPath(parent.m_xmlPath),
          m_skinBasePath(parent.m_skinBasePath),
          m_pConfig(parent.m_pConfig),
          m_variables(parent.variables()),
          m_pScriptEngine(parent.m_pScriptEngine),
          m_pScriptDebugger(parent.m_pScriptDebugger),
          m_parentGlobal(m_pScriptEngine->globalObject()),
          m_hookRx(parent.m_hookRx),
          m_pSvgCache(parent.m_pSvgCache),
          m_pSingletons(parent.m_pSingletons),
          m_scaleFactor(parent.m_scaleFactor) {
    // we generate a new global object to preserve the scope between
    // a context and its children
    auto context = m_pScriptEngine->pushContext()->activationObject();
    auto newGlobal = m_pScriptEngine->newObject();
    QScriptValueIterator it(m_parentGlobal);
    while (it.hasNext()) {
        it.next();
        newGlobal.setProperty(it.name(), it.value());
    }

    for (auto it = m_variables.begin();
         it != m_variables.end(); ++it) {
        newGlobal.setProperty(it.key(), it.value());
    }
    m_pScriptEngine->setGlobalObject(newGlobal);
}

SkinContext::~SkinContext() {
    // Pop the context only if we're a child.
    if (!isRoot()) {
        m_pScriptEngine->popContext();
        m_pScriptEngine->setGlobalObject(m_parentGlobal);
    }
}

QString SkinContext::variable(const QString& name) const {
    return m_variables.value(name, QString());
}

void SkinContext::setVariable(const QString& name, const QString& value) {
    m_variables[name] = value;
    QScriptValue context = m_pScriptEngine->currentContext()->activationObject();
    context.setProperty(name, value);
}

void SkinContext::setXmlPath(const QString& xmlPath) {
    m_xmlPath = xmlPath;
}

bool SkinContext::hasVariableUpdates(const QDomNode& node) const {
    auto child = node.firstChild();
    while (!child.isNull()) {
        if (child.isElement() && child.nodeName() == "SetVariable") {
            return true;
        }
        child = child.nextSibling();
    }
    return false;
}

void SkinContext::updateVariables(const QDomNode& node) {
    auto child = node.firstChild();
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
    auto name = element.attribute("name");
    auto value = variableNodeToText(element);
    setVariable(name, value);
}

QString SkinContext::variableNodeToText(const QDomElement& variableNode) const {
    auto expression = variableNode.attribute("expression");
    if (!expression.isNull()) {
        auto result = m_pScriptEngine->evaluate(
            expression, m_xmlPath, variableNode.lineNumber());
        return result.toString();
    }

    auto variableName = variableNode.attribute("name");
    if (!variableName.isNull()) {
        auto formatString = variableNode.attribute("format");
        if (!formatString.isNull()) {
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
    QString result;
    auto child = node.firstChild();
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
    return result;
}

PixmapSource SkinContext::getPixmapSource(const QDomNode& pixmapNode) const {
    PixmapSource source;

    const SvgParser svgParser(*this);

    if (!pixmapNode.isNull()) {
        auto svgNode = selectNode(pixmapNode, "svg");
        if (!svgNode.isNull()) {
            // inline svg
            auto rslt = svgParser.saveToQByteArray(
                    svgParser.parseSvgTree(svgNode, m_xmlPath));
            source.setSVG(rslt);
        } else {
            // filename.
            source = getPixmapSourceInner(nodeToString(pixmapNode), svgParser);
        }
    }

    return source;
}

PixmapSource SkinContext::getPixmapSource(const QString& filename) const {
    const SvgParser svgParser(*this);
    return getPixmapSourceInner(filename, svgParser);
}

QDomElement SkinContext::loadSvg(const QString& filename) const {
    auto& cachedSvg = (*m_pSvgCache)[filename];
    if (cachedSvg.isNull()) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QDomDocument document;
            if (!document.setContent(&file)) {
                qDebug() << "ERROR: Failed to set content on QDomDocument";
            }
            cachedSvg = document.elementsByTagName("svg").item(0).toElement();
            file.close();
        }
    }
    return cachedSvg;
}

PixmapSource SkinContext::getPixmapSourceInner(const QString& filename,
                                               const SvgParser& svgParser) const {
    PixmapSource source;
    if (!filename.isEmpty()) {
        source.setPath(getSkinPath(filename));
        if (source.isSVG()) {
            auto svgElement = loadSvg(filename);
            auto rslt = svgParser.saveToQByteArray(
                svgParser.parseSvgTree(svgElement, filename));
            source.setSVG(rslt);
        }
    }
    return source;
}

/**
 * All the methods below exist to access some of the scriptEngine features
 * from the svgParser.
 */
QScriptValue SkinContext::evaluateScript(const QString& expression,
                                         const QString& filename,
                                         int lineNumber) {
    return m_pScriptEngine->evaluate(expression, filename, lineNumber);
}

QScriptValue SkinContext::importScriptExtension(const QString& extensionName) {
    auto out = m_pScriptEngine->importExtension(extensionName);
    if (m_pScriptEngine->hasUncaughtException()) {
        qDebug() << out.toString();
    }
    return out;
}

const QSharedPointer<QScriptEngine> SkinContext::getScriptEngine() const {
    return m_pScriptEngine;
}

void SkinContext::enableDebugger(bool state) const {
    if (CmdlineArgs::Instance().getDeveloper() && m_pConfig != NULL &&
            m_pConfig->getValueString(ConfigKey("[ScriptDebugger]", "Enabled")) == "1") {
        if (state) {
            m_pScriptDebugger->attachTo(m_pScriptEngine.data());
        } else {
            m_pScriptDebugger->detach();
        }
    }
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

int SkinContext::scaleToWidgetSize(QString& size) const {
    bool widthOk = false;
    double dSize = size.toDouble(&widthOk);
    if (widthOk && dSize >= 0) {
        return static_cast<int>(round(dSize * m_scaleFactor));
    }
    return -1;
}
