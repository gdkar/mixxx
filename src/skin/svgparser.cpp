#include <QtDebug>
#include <QStringList>
#include <QScriptValue>

#include "skin/svgparser.h"

SvgParser::SvgParser(const SkinContext& parent)
        : m_context(parent)
{
}
SvgParser::~SvgParser() = default;
QDomNode SvgParser::parseSvgFile(const QString& svgFileName) const
{
    m_currentFile = svgFileName;
    QFile file(svgFileName);
    QDomNode svgNode;
    if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        QDomDocument document;
        if (!document.setContent(&file)) qDebug() << "ERROR: Failed to set content on QDomDocument";
        svgNode = document.elementsByTagName("svg").item(0);
        scanTree(&svgNode);
        file.close();
    }
    return svgNode;
}
QDomNode SvgParser::parseSvgTree(const QDomNode& svgSkinNode,const QString& sourcePath) const
{
    m_currentFile = sourcePath;
    // clone svg to don't alter xml input
    auto svgNode = svgSkinNode.cloneNode(true);
    scanTree(&svgNode);
    return svgNode;
}
void SvgParser::scanTree(QDomNode* node) const
{
    parseElement(node);
    auto children = node->childNodes();
    for (int i = 0; i < children.count(); ++i) {
        auto child = children.at(i);
        if (child.isElement()) scanTree(&child);
    }
}
void SvgParser::parseElement(QDomNode* node) const
{
    auto element = node->toElement();
    parseAttributes(*node);
    if (element.tagName() == "text")
    {
        if (element.hasAttribute("value"))
        {
            auto expression = element.attribute("value");
            auto result = evaluateTemplateExpression(expression, node->lineNumber()).toString();
            if (!result.isNull())
            {
                auto children = node->childNodes();
                for (int i = 0; i < children.count(); ++i) {node->removeChild(children.at(i));}
                auto newChild = node->ownerDocument().createTextNode(result);
                node->appendChild(newChild);
            }
        }

    }
    else if (element.tagName() == "Variable")
    {
        QString value;
        if (element.hasAttribute("expression"))
        {
            auto expression = element.attribute("expression");
            value = evaluateTemplateExpression(expression, node->lineNumber()).toString();
        }
        else if (element.hasAttribute("name"))
        {
            value = m_context.variable(element.attribute("name"));
        }
        if (!value.isNull())
        {
            // replace node by its value
            auto varParentNode = node->parentNode();
            auto varValueNode = node->ownerDocument().createTextNode(value);
            auto oldChild = varParentNode.replaceChild(varValueNode, *node);
            if (oldChild.isNull()) qDebug() << "SVG : unable to replace dom node changed. \n";
        }
    }
    else if (element.tagName() == "script")
    {
        // Look for a filepath in the "src" attribute
        // QString scriptPath = node->toElement().attribute("src");
        auto scriptPath = element.attribute("src");
        if (!scriptPath.isNull()) {
            QFile scriptFile(m_context.getSkinPath(scriptPath));
            if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) qDebug() << "ERROR: Failed to open script file";
            QTextStream in(&scriptFile);
            auto result = m_context.evaluateScript(in.readAll(),scriptPath);
        }
        // Evaluates the content of the script element
        // QString expression = m_context.nodeToString(*node);
        auto result = m_context.evaluateScript(element.text(), m_currentFile, node->lineNumber());
    }
}
void SvgParser::parseAttributes(const QDomNode& node) const
{
    auto attributes = node.attributes();
    auto element = node.toElement();
    // Retrieving hooks pattern from script extension
    auto global = m_context.getScriptEngine()->globalObject();
    auto hooksPattern = global.property("svg")
        .property("getHooksPattern").call(global.property("svg"));
    QRegExp hookRx;
    if (!hooksPattern.isNull()) hookRx.setPattern(hooksPattern.toString());
    // expr-attribute_name="var_name";
    QRegExp nameRx("^expr-([^=\\s]+)$");
    // TODO (jclaveau) : move this pattern definition to the script extension?
    for (auto i = 0; i < attributes.count(); i++) {
        auto attribute = attributes.item(i).toAttr();
        auto attributeValue = attribute.value();
        auto attributeName = attribute.name();
        if (nameRx.indexIn(attributeName) != -1)
        {
            auto varValue = evaluateTemplateExpression(attributeValue, node.lineNumber()).toString();
            if (varValue.length()) element.setAttribute(nameRx.cap(1), varValue);
            continue;
        }
        if (!hookRx.isEmpty())
        {
            // searching hooks in the attribute value
            auto pos = 0;
            while ((pos = hookRx.indexIn(attributeValue, pos)) != -1)
            {
                auto captured = hookRx.capturedTexts();
                auto match = hookRx.cap(0);
                auto tmp = QString{"svg.templateHooks."} + match;
                auto replacement = evaluateTemplateExpression(tmp, node.lineNumber()).toString();
                attributeValue.replace(pos, match.length(), replacement);
                pos += replacement.length();
            }
        }
        attribute.setValue(attributeValue);
    }
}
QByteArray SvgParser::saveToQByteArray(const QDomNode& svgNode) const
{
    // TODO (jclaveau) : a way to look the svg after the parsing would be nice!
    QByteArray out;
    QTextStream textStream(&out);
    // cloning avoid segfault during save()
    svgNode.cloneNode().save(textStream, 2);
    return out;
}
QScriptValue SvgParser::evaluateTemplateExpression(const QString& expression,int lineNumber) const
{
    auto out = m_context.evaluateScript(expression, m_currentFile, lineNumber);
    if (m_context.getScriptEngine()->hasUncaughtException()) return QScriptValue();
    else return out;
}
