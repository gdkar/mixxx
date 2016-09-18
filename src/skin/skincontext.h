#ifndef SKINCONTEXT_H
#define SKINCONTEXT_H

#include <QHash>
#include <QString>
#include <QDomNode>
#include <QDomElement>
#include <QJSEngine>
#include <QDir>
#include <QtDebug>
#include <QSharedPointer>
#include <QRegularExpression>

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
    SkinContext(UserSettingsPointer pConfig, QString xmlPath);
    SkinContext(const SkinContext& parent);
    virtual ~SkinContext();

    // Gets a path relative to the skin path.
    QString getSkinPath(QString relativePath) const {
        return m_skinBasePath.filePath(relativePath);
    }

    // Sets the base path used by getSkinPath.
    void setSkinBasePath(QString skinBasePath) {
        m_skinBasePath = QDir(skinBasePath);
    }

    // Variable lookup and modification methods.
    QString variable(QString name) const;
    void setVariable(QString name, QString value);
    void setXmlPath(QString xmlPath);

    // Returns whether the node has a <SetVariable> node.
    bool hasVariableUpdates(QDomNode node) const;
    // Updates the SkinContext with all the <SetVariable> children of node.
    void updateVariables(QDomNode node);
    // Updates the SkinContext with 'element', a <SetVariable> node.
    void updateVariable(QDomElement element);

    QDomNode selectNode(QDomNode node, QString nodeName) const {
        QDomNode child = node.firstChild();
        while (!child.isNull()) {
            if (child.nodeName() == nodeName) {
                return child;
            }
            child = child.nextSibling();
        }
        return QDomNode();
    }

    QDomElement selectElement(QDomNode node, QString nodeName) const {
        QDomNode child = selectNode(node, nodeName);
        return child.toElement();
    }

    QString selectString(QDomNode node, QString nodeName) const {
        QDomElement child = selectElement(node, nodeName);
        return nodeToString(child);
    }

    float selectFloat(QDomNode node, QString nodeName) const {
        bool ok = false;
        float conv = nodeToString(selectElement(node, nodeName)).toFloat(&ok);
        return ok ? conv : 0.0f;
    }

    double selectDouble(QDomNode node, QString nodeName) const {
        bool ok = false;
        double conv = nodeToString(selectElement(node, nodeName)).toDouble(&ok);
        return ok ? conv : 0.0;
    }

    int selectInt(QDomNode node, QString nodeName,
                         bool* pOk = nullptr) const {
            bool ok = false;
            int conv = nodeToString(selectElement(node, nodeName)).toInt(&ok);
            if (pOk != nullptr) {
                *pOk = ok;
            }
            return ok ? conv : 0;
    }

    bool selectBool(QDomNode node, QString nodeName,
                           bool defaultValue) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            QString stringValue = nodeToString(child);
            return stringValue.contains("true", Qt::CaseInsensitive);
        }
        return defaultValue;
    }

    bool hasNodeSelectElement(QDomNode node, QString nodeName,
                                     QDomElement* value) const {
        QDomElement child = selectElement(node, nodeName);
        if (!child.isNull()) {
            *value = child;
            return true;
        }
        return false;
    }

    bool hasNodeSelectString(QDomNode node, QString nodeName,
                                    QString *value) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            *value = nodeToString(child);
            return true;
        }
        return false;
    }

    bool hasNodeSelectBool(QDomNode node, QString nodeName,
                                  bool* value) const {
        QDomNode child = selectNode(node, nodeName);
        if (!child.isNull()) {
            QString stringValue = nodeToString(child);
            *value = stringValue.contains("true", Qt::CaseInsensitive);
            return true;
        }
        return false;
    }

    bool hasNodeSelectInt(QDomNode node, QString nodeName,
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

    bool hasNodeSelectDouble(QDomNode node, QString nodeName,
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

    bool selectAttributeBool(QDomElement element,
                                    QString attributeName,
                                    bool defaultValue) const {
        QString stringValue;
        if (hasAttributeSelectString(element, attributeName, &stringValue)) {
            return stringValue.contains("true", Qt::CaseInsensitive);
        }
        return defaultValue;
    }

    bool hasAttributeSelectString(QDomElement element,
                                         QString attributeName,
                                         QString* result) const {
        *result = element.attribute(attributeName);
        return !result->isNull();
    }

    QString nodeToString(QDomNode node) const;
    PixmapSource getPixmapSource(QDomNode pixmapNode) const;
    PixmapSource getPixmapSource(QString filename) const;

    Paintable::DrawMode selectScaleMode(QDomElement element,
                                               Paintable::DrawMode defaultDrawMode) const {
        QString drawModeStr;
        if (hasAttributeSelectString(element, "scalemode", &drawModeStr)) {
            return Paintable::DrawModeFromString(drawModeStr);
        }
        return defaultDrawMode;
    }

    QJSValue evaluateScript(QString expression,
                                QString filename=QString(),
                                int lineNumber=1);
    QJSValue importScriptExtension(QString extensionName);
    const QSharedPointer<QJSEngine> getScriptEngine() const;

    QDebug logWarning(const char* file, const int line, QDomNode node) const;

    void defineSingleton(QString objectName, QWidget* widget) {
        return m_pSingletons->insertSingleton(objectName, widget);
    }

    QWidget* getSingletonWidget(QString objectName) const {
        return m_pSingletons->getSingletonWidget(objectName);
    }

    QRegularExpression getHookRegex() const
    {
        return m_hookRx;
    }

  private:
    PixmapSource getPixmapSourceInner(QString filename,
                                      const SvgParser& svgParser) const;

    QDomElement loadSvg(QString filename) const;

    // If our parent global isValid() then we were constructed with a
    // parent. Otherwise we are a root SkinContext.
    bool isRoot() const
    {
        return m_context.prototype().strictlyEquals(m_pScriptEngine->globalObject());
    }

    QString variableNodeToText(QDomElement element) const;

    QString m_xmlPath;
    QDir m_skinBasePath;
    UserSettingsPointer m_pConfig;

    QSharedPointer<QJSEngine> m_pScriptEngine;
//    QSharedPointer<QJSEngineDebugger> m_pScriptDebugger;
    QJSValue m_context;
    QRegularExpression m_hookRx;
    QSharedPointer<QHash<QString, QDomElement>> m_pSvgCache;

    // The SingletonContainer map is passed to child SkinContexts, so that all
    // templates in the tree can share a single map.
    QSharedPointer<SingletonMap> m_pSingletons;
};

#endif /* SKINCONTEXT_H */
