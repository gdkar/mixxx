#include <QtDebug>
#include <QStringList>
#include <QJSValue>

#include "skin/svgparser.h"

SvgParser::SvgParser(const SkinContext& parent)
        : m_parentContext(parent) {
}

SvgParser::~SvgParser() {
}

QDomNode SvgParser::parseSvgTree(QDomNode svgSkinNode,
                                 QString sourcePath) const
{
    m_currentFile = sourcePath;
    // clone svg to don't alter xml input
    auto svgNode = svgSkinNode.cloneNode(true).toElement();
    scanTree(&svgNode);
    return svgNode;
}

void SvgParser::scanTree(QDomElement* node) const
{
    parseElement(node);
    auto children = node->childNodes();
    for (auto i = 0; i < children.count(); ++i) {
        auto child = children.at(i).toElement();
        if (!child.isNull()) {
            scanTree(&child);
        }
    }
}

void SvgParser::parseElement(QDomElement* element) const {
    parseAttributes(element);

    auto tagName = element->tagName();
    if (tagName == "text") {
        if (element->hasAttribute("value")) {
            auto expression = element->attribute("value");
            auto result = evaluateTemplateExpression(
                expression, element->lineNumber()).toString();

            if (!result.isNull()) {
                auto children = element->childNodes();
                for (int i = 0; i < children.count(); ++i) {
                    element->removeChild(children.at(i));
                }
                auto newChild = element->ownerDocument().createTextNode(result);
                element->appendChild(newChild);
            }
        }
    } else if (tagName == "Variable") {
        auto value = QString{};
        if (element->hasAttribute("expression")) {
            auto expression = element->attribute("expression");
            value = evaluateTemplateExpression(
                expression, element->lineNumber()).toString();
        } else if (element->hasAttribute("name")) {
            /* TODO (jclaveau) : Getting the variable from the context or the
             * script engine have the same result here (in the skin context two)
             * Isn't it useless?
             * m_context.variable(name) <=> m_scriptEngine.evaluate(name)
             */
            value = m_parentContext.variable(element->attribute("name"));
        }

        if (!value.isNull()) {
            // replace node by its value
            auto varParentNode = element->parentNode();
            auto varValueNode = element->ownerDocument().createTextNode(value);
            auto oldChild = varParentNode.replaceChild(varValueNode, *element);
            if (oldChild.isNull()) {
                // replaceChild has a really weird behaviour so I add this check
                qDebug() << "SVG : unable to replace dom node changed. \n";
            }
        }
    } else if (tagName == "script") {
        // Look for a filepath in the "src" attribute
        // QString scriptPath = element->toElement().attribute("src");

        auto childContext = lazyChildContext();

        auto scriptPath = element->attribute("src");
        if (!scriptPath.isNull()) {
            QFile scriptFile(childContext.getSkinPath(scriptPath));
            if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
                qDebug() << "ERROR: Failed to open script file";
            }
            QTextStream in(&scriptFile);
            auto result = childContext.evaluateScript(in.readAll(), scriptPath);
        } else {
            // Evaluates the content of the script element
            // QString expression = m_context.nodeToString(*element);
            auto result = childContext.evaluateScript(
                element->text(), m_currentFile, element->lineNumber());
        }
    }
}
void SvgParser::parseAttributes(QDomElement* element) const {
    auto attributes = element->attributes();

    // expr-attribute_name="var_name";
    QRegularExpression nameRx("^expr-([^=\\s]+)$");
    // TODO (jclaveau) : move this pattern definition to the script extension?
    for (int i = 0; i < attributes.count(); i++) {
        auto attribute = attributes.item(i).toAttr();
        auto attributeValue = attribute.value();
        auto attributeName = attribute.name();

        // searching variable attributes :
        // expr-attribute_name="variable_name|expression"
        auto match = nameRx.match(attributeName);
        if (match.hasMatch()) {
            auto varValue = evaluateTemplateExpression(attributeValue, element->lineNumber()).toString();
            if (!varValue.isEmpty()) {
                element->setAttribute(match.captured(1), varValue);
            }
            continue;
        }

        auto hookRx = m_parentContext.getHookRegex();
        if (hookRx.isValid()) {
            auto matches = hookRx.globalMatch(attributeValue);
            while(matches.hasNext()) {
                auto match = matches.next();
//                auto captured = match.capturedTexts();
                auto tmp = "svg.templateHooks." + match.captured(0);
                auto replacement = evaluateTemplateExpression(tmp, element->lineNumber()).toString();
                attributeValue.replace(match.capturedStart(), match.capturedLength(), replacement);
            }
            attribute.setValue(attributeValue);
        }
    }
}

QByteArray SvgParser::saveToQByteArray(QDomNode svgNode) const
{
    // TODO (jclaveau) : a way to look the svg after the parsing would be nice!
    QByteArray out;
    QTextStream textStream(&out);
    // cloning avoid segfault during save()
    svgNode.cloneNode().save(textStream, 2);
    return out;
}

QJSValue SvgParser::evaluateTemplateExpression(QString expression,
                                                   int lineNumber) const
{
    auto childContext = lazyChildContext();
    auto out = childContext.evaluateScript(expression, m_currentFile, lineNumber);
    if(out.isError()){
        // return an empty string as replacement for the in-attribute expression
        return QJSValue();
    } else {
        return out;
    }
}
