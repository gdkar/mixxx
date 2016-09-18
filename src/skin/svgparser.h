#ifndef SVGPARSER_H
#define SVGPARSER_H

#include <QRegularExpression>
#include <QString>
#include <QDomNode>
#include <QDomElement>
#include <QScopedPointer>

#include "skin/skincontext.h"


// A class for managing the svg files
class SvgParser {
  public:
    // Assumes SkinContext lives for the lifetime of SvgParser.
    SvgParser(const SkinContext& parent);
    virtual ~SvgParser();

    QDomNode parseSvgTree(QDomNode svgSkinNode,
                          QString sourcePath) const;
    QByteArray saveToQByteArray(QDomNode svgNode) const;

  private:
    const SkinContext& lazyChildContext() const
    {
        if (m_pLazyContext.isNull()) {
            m_pLazyContext.reset(new SkinContext(m_parentContext));
        }
        return *m_pLazyContext;
    }
    void scanTree(QDomElement* node) const;
    void parseElement(QDomElement* svgNode) const;
    void parseAttributes(QDomElement* element) const;
    QJSValue evaluateTemplateExpression(QString expression,int lineNumber) const;

    const SkinContext& m_parentContext;
    mutable QScopedPointer<SkinContext> m_pLazyContext;
    mutable QString m_currentFile;
    mutable QRegularExpression m_hookRx;
};

#endif /* SVGPARSER_H */
