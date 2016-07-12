#ifndef SKINCONTEXT_H
#define SKINCONTEXT_H

#include <QHash>
#include <QString>
#include <QDomNode>
#include <QDomElement>
#include <QScriptEngine>
#include <QDir>
#include <QScriptEngineDebugger>
#include <QtDebug>
#include <QSharedPointer>
#include <QRegExp>

#include "preferences/usersettings.h"
#include "skin/pixmapsource.h"
#include "widget/wsingletoncontainer.h"
#include "widget/wpixmapstore.h"

#define SKIN_WARNING(node, context) (context).logWarning(__FILE__, __LINE__, (node))

class SvgParser;

// A class for managing the current context/environment when processing a
// skin. Used hierarchically by LegacySkinParser to create new contexts and
// evaluate skin XML nodes while loading the skin.
class SkinContext {
  public:
    SkinContext(UserSettingsPointer pConfig, const QString& xmlPath);
    SkinContext(const SkinContext& parent);
    virtual ~SkinContext();

    // Gets a path relative to the skin path.
    QString getSkinPath(const QString& relativePath) const {
        return m_skinBasePath.filePath(relativePath);
    }
    UserSettingsPointer getConfig() const
    {
        return m_pConfig;
    }
    // Sets the base path used by getSkinPath.
    void setSkinBasePath(const QString& skinBasePath) {
        m_skinBasePath = QDir(skinBasePath);
    }

    // Variable lookup and modification methods.
    QString variable(const QString& name) const;
    const QHash<QString, QString>& variables() const {
        return m_variables;
    }
    void setVariable(const QString& name, const QString& value);
    void setXmlPath(const QString& xmlPath);

    // Returns whether the node has a <SetVariable> node.
    bool hasVariableUpdates(const QDomNode& node) const;
    // Updates the SkinContext with all the <SetVariable> children of node.
    void updateVariables(const QDomNode& node);
    // Updates the SkinContext with 'element', a <SetVariable> node.
    void updateVariable(const QDomElement& element);

    QDomNode selectNode(const QDomNode& node, const QString& nodeName) const {
        QDomNode child = node.firstChild();
        while (!child.isNull()) {
            if (child.nodeName() == nodeName) {
                return child;
            }
            child = child.nextSibling();
        }
        return QDomNode();
    }

    QDomElement selectElement(const QDomNode& node, const QString& nodeName) const {
        QDomNode child = selectNode(node, nodeName);
        return child.toElement();
    }

    QString selectString(const QDomNode& node, const QString& nodeName) const {
        QDomElement child = selectElement(node, nodeName);
        return nodeToString(child);
    }

    float selectFloat(const QDomNode& node, const QString& nodeName) const {
        bool ok = false;
        float conv = nodeToString(selectElement(node, nodeName)).toFloat(&ok);
        return ok ? conv : 0.0f;
    }

    double selectDouble(const QDomNode& node, const QString& nodeName) const {
        bool ok = false;
        double conv = nodeToString(selectElement(node, nodeName)).toDouble(&ok);
        return ok ? conv : 0.0;
    }

    int selectInt(const QDomNode& node, const QString& nodeName,
                         bool* pOk = nullptr) const {
            bool ok = false;
            int conv = nodeToString(selectElement(node, nodeName)).toInt(&ok);
            if (pOk != nullptr) {
                *pOk = ok;
            }
            return ok ? conv : 0;
    }

    bool selectBool(const QDomNode& node, const QString& nodeName,
                           bool defaultValue) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            QString stringValue = nodeToString(child);
            return stringValue.contains("true", Qt::CaseInsensitive);
        }
        return defaultValue;
    }

    bool hasNodeSelectElement(const QDomNode& node, const QString& nodeName,
                                     QDomElement* value) const {
        QDomElement child = selectElement(node, nodeName);
        if (!child.isNull()) {
            *value = child;
            return true;
        }
        return false;
    }

    bool hasNodeSelectString(const QDomNode& node, const QString& nodeName,
                                    QString *value) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            *value = nodeToString(child);
            return true;
        }
        return false;
    }

    bool hasNodeSelectBool(const QDomNode& node, const QString& nodeName,
                                  bool* value) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            QString stringValue = nodeToString(child);
            *value = stringValue.contains("true", Qt::CaseInsensitive);
            return true;
        }
        return false;
    }

    bool hasNodeSelectInt(const QDomNode& node, const QString& nodeName,
                                 int* value) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            bool ok = false;
            double result = nodeToString(child).toInt(&ok);
            if (ok) {
                *value = result;
                return true;
            }
        }
        return false;
    }

    bool hasNodeSelectDouble(const QDomNode& node, const QString& nodeName,
                                    double* value) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            bool ok = false;
            double result = nodeToString(child).toDouble(&ok);
            if (ok) {
                *value = result;
                return true;
            }
        }
        return false;
    }

    bool selectAttributeBool(const QDomElement& element,
                                    const QString& attributeName,
                                    bool defaultValue) const {
        QString stringValue;
        if (hasAttributeSelectString(element, attributeName, &stringValue)) {
            return stringValue.contains("true", Qt::CaseInsensitive);
        }
        return defaultValue;
    }

    bool hasAttributeSelectString(const QDomElement& element,
                                         const QString& attributeName,
                                         QString* result) const {
        *result = element.attribute(attributeName);
        return !result->isNull();
    }

    QString nodeToString(const QDomNode& node) const;
    PixmapSource getPixmapSource(const QDomNode& pixmapNode) const;
    PixmapSource getPixmapSource(const QString& filename) const;

    Paintable::DrawMode selectScaleMode(const QDomElement& element,
                                               Paintable::DrawMode defaultDrawMode) const {
        QString drawModeStr;
        if (hasAttributeSelectString(element, "scalemode", &drawModeStr)) {
            return Paintable::DrawModeFromString(drawModeStr);
        }
        return defaultDrawMode;
    }

    QScriptValue evaluateScript(const QString& expression,
                                const QString& filename=QString(),
                                int lineNumber=1);
    QScriptValue importScriptExtension(const QString& extensionName);
    const QSharedPointer<QScriptEngine> getScriptEngine() const;
    void enableDebugger(bool state) const;

    QDebug logWarning(const char* file, const int line, const QDomNode& node) const;

    void defineSingleton(QString objectName, QWidget* widget) {
        return m_pSingletons->insertSingleton(objectName, widget);
    }

    QWidget* getSingletonWidget(QString objectName) const {
        return m_pSingletons->getSingletonWidget(objectName);
    }

    const QRegExp& getHookRegex() const {
        return m_hookRx;
    }

  private:
    PixmapSource getPixmapSourceInner(const QString& filename,
                                      const SvgParser& svgParser) const;

    QDomElement loadSvg(const QString& filename) const;

    // If our parent global isValid() then we were constructed with a
    // parent. Otherwise we are a root SkinContext.
    bool isRoot() const { return !m_parentGlobal.isValid(); }

    QString variableNodeToText(const QDomElement& element) const;

    QString m_xmlPath;
    QDir m_skinBasePath;
    UserSettingsPointer m_pConfig;

    QHash<QString, QString> m_variables;
    QSharedPointer<QScriptEngine> m_pScriptEngine;
    QSharedPointer<QScriptEngineDebugger> m_pScriptDebugger;
    QScriptValue m_parentGlobal;
    QRegExp m_hookRx;

    QSharedPointer<QHash<QString, QDomElement>> m_pSvgCache;

    // The SingletonContainer map is passed to child SkinContexts, so that all
    // templates in the tree can share a single map.
    QSharedPointer<SingletonMap> m_pSingletons;
};

#endif /* SKINCONTEXT_H */
