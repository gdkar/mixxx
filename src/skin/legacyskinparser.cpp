// legacyskinparser.cpp
// Created 9/19/2010 by RJ Ryan (rryan@mit.edu)

#include "skin/legacyskinparser.h"

#include <QDir>
#include <QGridLayout>
#include <QLabel>
#include <QMutexLocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QtDebug>
#include <QtGlobal>

#include "controlobject.h"
#include "controlobjectslave.h"

#include "mixxxkeyboard.h"
#include "playermanager.h"
#include "player.h"
#include "library/library.h"
#include "util/xml.h"
#include "controllers/controllerlearningeventfilter.h"
#include "controllers/controllermanager.h"

#include "skin/colorschemeparser.h"
#include "skin/skincontext.h"
#include "skin/launchimage.h"

#include "effects/effectsmanager.h"

#include "widget/controlwidgetconnection.h"
#include "widget/wbasewidget.h"
#include "widget/wcoverart.h"
#include "widget/wwidget.h"
#include "widget/wknob.h"
#include "widget/wknobcomposed.h"
#include "widget/wslidercomposed.h"
#include "widget/wpushbutton.h"
#include "widget/weffectpushbutton.h"
#include "widget/wdisplay.h"
#include "widget/wvumeter.h"
#include "widget/wstatuslight.h"
#include "widget/wlabel.h"
#include "widget/wtime.h"
#include "widget/wtracktext.h"
#include "widget/wtrackproperty.h"
#include "widget/wstarrating.h"
#include "widget/wnumber.h"
#include "widget/wnumberdb.h"
#include "widget/wnumberpos.h"
#include "widget/wnumberrate.h"
#include "widget/weffectchain.h"
#include "widget/weffect.h"
#include "widget/weffectparameter.h"
#include "widget/weffectbuttonparameter.h"
#include "widget/weffectparameterbase.h"
#include "widget/woverviewlmh.h"
#include "widget/woverviewhsv.h"
#include "widget/woverviewrgb.h"
#include "widget/wspinny.h"
#include "widget/wwaveformviewer.h"
#include "waveform/waveformwidgetfactory.h"
#include "widget/wsearchlineedit.h"
#include "widget/wlibrary.h"
#include "widget/wlibrarysidebar.h"
#include "widget/wskincolor.h"
#include "widget/wpixmapstore.h"
#include "widget/wwidgetstack.h"
#include "widget/wsizeawarestack.h"
#include "widget/wwidgetgroup.h"
#include "widget/wkey.h"
#include "widget/wcombobox.h"
#include "widget/wsplitter.h"
#include "widget/wsingletoncontainer.h"
#include "util/cmdlineargs.h"

using mixxx::skin::SkinManifest;

QList<const char*> LegacySkinParser::s_channelStrs;
QMutex LegacySkinParser::s_safeStringMutex;

static bool sDebug = false;

ControlObject* controlFromConfigKey(ConfigKey key, bool bPersist,
                                    bool* created) {
    auto pControl = ControlObject::getControl(key,true);
    if (pControl) {
        if (created) { *created = false; }
        return pControl;
    }
    // TODO(rryan): Make this configurable by the skin.
    qWarning() << "Requested control does not exist:"
               << QString("%1,%2").arg(key.group, key.item)
               << "Creating it.";
    // Since the usual behavior here is to create a skin-defined push
    // button, actually make it a push button and set it to toggle.
    auto controlButton = new ControlPushButton(key, bPersist);
    controlButton->setButtonMode(ControlPushButton::TOGGLE);
    if (created) { *created = true; }
    return controlButton;
}

ControlObject* LegacySkinParser::controlFromConfigNode(QDomElement element,const QString& nodeName,bool* created) {
    if (element.isNull() || !m_pContext->hasNode(element, nodeName)) {return nullptr;}
    auto keyElement = m_pContext->selectElement(element, nodeName);
    auto name = m_pContext->nodeToString(keyElement);
    auto key = ConfigKey::parseCommaSeparated(name);
    auto bPersist = m_pContext->selectAttributeBool(keyElement, "persist", false);
    return controlFromConfigKey(key, bPersist, created);
}
LegacySkinParser::LegacySkinParser()
        : m_pConfig(nullptr),
          m_pKeyboard(nullptr),
          m_pPlayerManager(nullptr),
          m_pControllerManager(nullptr),
          m_pLibrary(nullptr),
          m_pVCManager(nullptr),
          m_pEffectsManager(nullptr),
          m_pParent(nullptr),
          m_pContext(nullptr) {
}
LegacySkinParser::LegacySkinParser(ConfigObject<ConfigValue>* pConfig,
                                   MixxxKeyboard* pKeyboard,
                                   PlayerManager* pPlayerManager,
                                   ControllerManager* pControllerManager,
                                   Library* pLibrary,
                                   VinylControlManager* pVCMan,
                                   EffectsManager* pEffectsManager)
        : m_pConfig(pConfig),
          m_pKeyboard(pKeyboard),
          m_pPlayerManager(pPlayerManager),
          m_pControllerManager(pControllerManager),
          m_pLibrary(pLibrary),
          m_pVCManager(pVCMan),
          m_pEffectsManager(pEffectsManager),
          m_pParent(nullptr),
          m_pContext(nullptr) {
}
LegacySkinParser::~LegacySkinParser() { delete m_pContext; }
bool LegacySkinParser::canParse(QString skinPath) {
    QDir skinDir(skinPath);
    if (!skinDir.exists()) { return false;}
    if (!skinDir.exists("skin.xml")) return false;
    // TODO check skin.xml for compliance
    return true;
}
// static
QDomElement LegacySkinParser::openSkin(QString skinPath) {
    QDir skinDir(skinPath);
    if (!skinDir.exists()) {
        qDebug() << "LegacySkinParser::openSkin - skin dir do not exist:" << skinPath;
        return QDomElement();
    }
    auto skinXmlPath = skinDir.filePath("skin.xml");
    QFile skinXmlFile(skinXmlPath);
    if (!skinXmlFile.open(QIODevice::ReadOnly)) {
        qDebug() << "LegacySkinParser::openSkin - can't open file:" << skinXmlPath
                 << "in directory:" << skinDir.path();
        return QDomElement();
    }
    QDomDocument skin("skin");
    auto errorMessage = QString{};
    auto errorLine   = 0;
    auto errorColumn = 0;
    if (!skin.setContent(&skinXmlFile,&errorMessage,&errorLine,&errorColumn)) {
        qDebug() << "LegacySkinParser::openSkin - setContent failed see"
                 << "line:" << errorLine << "column:" << errorColumn;
        qDebug() << "LegacySkinParser::openSkin - message:" << errorMessage;
        return QDomElement();
    }
    skinXmlFile.close();
    return skin.documentElement();
}
// static
QList<QString> LegacySkinParser::getSchemeList(QString qSkinPath) {
    auto docElem = openSkin(qSkinPath);
    auto schlist = QList<QString>{};
    auto colsch = docElem.namedItem("Schemes");
    if (!colsch.isNull() && colsch.isElement()) {
        auto sch = colsch.firstChild();
        while (!sch.isNull()) {
            auto thisname = XmlParse::selectNodeQString(sch, "Name");
            schlist.append(thisname);
            sch = sch.nextSibling();
        }
    }
    return schlist;
}
// static
void LegacySkinParser::freeChannelStrings() {
    QMutexLocker lock(&s_safeStringMutex);
    for (auto i = 0; i < s_channelStrs.length(); ++i) {
        if (s_channelStrs[i]) {delete [] s_channelStrs[i];}
        s_channelStrs[i] = nullptr;
    }
}
bool LegacySkinParser::compareConfigKeys(QDomNode node, QString key)
{
    auto n = node;
    // Loop over each <Connection>, check if it's ConfigKey matches key
    while (!n.isNull())
    {
        n = m_pContext->selectNode(n, "Connection");
        if (!n.isNull())
        {
            if  (m_pContext->selectString(n, "ConfigKey").contains(key)) return true;
        }
    }
    return false;
}
SkinManifest LegacySkinParser::getSkinManifest(QDomElement skinDocument) {
    auto manifest_node = skinDocument.namedItem("manifest");
    SkinManifest manifest;
    if (manifest_node.isNull() || !manifest_node.isElement()) {return manifest;}
    manifest.set_title(XmlParse::selectNodeQString(manifest_node, "title").toStdString());
    manifest.set_author(XmlParse::selectNodeQString(manifest_node, "author").toStdString());
    manifest.set_version(XmlParse::selectNodeQString(manifest_node, "version").toStdString());
    manifest.set_language(XmlParse::selectNodeQString(manifest_node, "language").toStdString());
    manifest.set_description(XmlParse::selectNodeQString(manifest_node, "description").toStdString());
    manifest.set_license(XmlParse::selectNodeQString(manifest_node, "license").toStdString());
    auto attributes_node = manifest_node.namedItem("attributes");
    if (!attributes_node.isNull() && attributes_node.isElement()) {
        auto attribute_nodes = attributes_node.toElement().elementsByTagName("attribute");
        for (int i = 0; i < attribute_nodes.count(); ++i) {
            auto attribute_node = attribute_nodes.item(i);
            if (attribute_node.isElement()) {
                auto attribute_element = attribute_node.toElement();
                QString configKey = attribute_element.attribute("config_key");
                QString persist = attribute_element.attribute("persist");
                QString value = attribute_element.text();
                SkinManifest::Attribute* attr = manifest.add_attribute();
                attr->set_config_key(configKey.toStdString());
                attr->set_persist(persist.toLower() == "true");
                attr->set_value(value.toStdString());
            }
        }
    }
    return manifest;
}
// static
Qt::MouseButton LegacySkinParser::parseButtonState(QDomNode node,const SkinContext& context) {
    if (context.hasNode(node, "ButtonState")) {
        if (context.selectString(node, "ButtonState").contains("LeftButton", Qt::CaseInsensitive)) {
            return Qt::LeftButton;
        } else if (context.selectString(node, "ButtonState").contains("RightButton", Qt::CaseInsensitive)) {
            return Qt::RightButton;
        }
    }
    return Qt::NoButton;
}
QWidget* LegacySkinParser::parseSkin(QString skinPath, QWidget* pParent) {
    qDebug() << "LegacySkinParser loading skin:" << skinPath;
    if (m_pParent) {
        qDebug() << "ERROR: Somehow a parent already exists -- you are probably re-using a LegacySkinParser which is not advisable!";
    }
    auto skinDocument = openSkin(skinPath);
    if (skinDocument.isNull()) {
        qDebug() << "LegacySkinParser::parseSkin - failed for skin:" << skinPath;
        return nullptr;
    }
    auto manifest = getSkinManifest(skinDocument);
    // Keep track of created attribute controls so we can parent them.
    auto created_attributes = QList<ControlObject*>{};
    // Apply SkinManifest attributes by looping through the proto.
    for (int i = 0; i < manifest.attribute_size(); ++i) {
        const auto& attribute = manifest.attribute(i);
        if (!attribute.has_config_key()) {continue;}
        auto ok = false;
        auto  value = QString::fromStdString(attribute.value()).toDouble(&ok);
        if (ok) {ConfigKey configKey = ConfigKey::parseCommaSeparated(
                    QString::fromStdString(attribute.config_key()));
            // Set the specified attribute, possibly creating the control
            // object in the process.
            auto created = false;
            // If there is no existing value for this CO in the skin,
            // update the config with the specified value. If the attribute
            // is set to persist, the value will be read when the control is created.
            // TODO: This is a hack, but right now it's the cleanest way to
            // get a CO with a specified initial value.  We should have a better
            // mechanism to provide initial default values for COs.
            if (attribute.persist() && m_pConfig->getValueString(configKey).isEmpty()) {
                m_pConfig->set(configKey, ConfigValue(QString::number(value)));
            }
            auto pControl = controlFromConfigKey(configKey,attribute.persist(),&created);
            if (created) {created_attributes.append(pControl);}
            if (!attribute.persist()) {
                // Only set the value if the control wasn't set up through
                // the persist logic.  Skin attributes are always
                // set on skin load.
                pControl->set(value);
            }
        } else {
            SKIN_WARNING(skinDocument, *m_pContext)
                    << "Error reading double value from skin attribute: "
                    << QString::fromStdString(attribute.value());
        }
    }
    ColorSchemeParser::setupLegacyColorSchemes(skinDocument, m_pConfig);
    auto skinPaths = QStringList(skinPath);
    QDir::setSearchPaths("skin", skinPaths);
    // don't parent till here so the first opengl waveform doesn't screw
    // up --bkgood
    // I'm disregarding this return value because I want to return the
    // created parent so MixxxMainWindow can use it for nefarious purposes (
    // fullscreen mostly) --bkgood
    m_pParent = pParent;
    delete m_pContext;
    m_pContext = new SkinContext(m_pConfig, skinPath + "/skin.xml");
    m_pContext->setSkinBasePath(skinPath.append("/"));
    auto widgets = parseNode(skinDocument);
    if (widgets.empty()) {
        SKIN_WARNING(skinDocument, *m_pContext) << "Skin produced no widgets!";
        return nullptr;
    } else if (widgets.size() > 1) {
        SKIN_WARNING(skinDocument, *m_pContext) << "Skin produced more than 1 widget!";
    }
    // Because the config is destroyed before MixxxMainWindow, we need to
    // parent the attributes to some other widget.  Otherwise they won't
    // be able to persist because the config will have already been deleted.
    for(auto  pControl: created_attributes) {pControl->setParent(widgets[0]);}
    return widgets[0];
}
LaunchImage* LegacySkinParser::parseLaunchImage(QString skinPath, QWidget* pParent) {
    auto skinDocument = openSkin(skinPath);
    if (skinDocument.isNull()) {return nullptr;}
    auto nodeName = skinDocument.nodeName();
    if (nodeName != "skin") {return nullptr;}
    // This allows image urls like
    // url(skin:/style/mixxx-icon-logo-symbolic.png);
    QStringList skinPaths(skinPath);
    QDir::setSearchPaths("skin", skinPaths);
    auto styleSheet = parseLaunchImageStyle(skinDocument);
    auto pLaunchImage = new LaunchImage(pParent, styleSheet);
    setupSize(skinDocument, pLaunchImage);
    return pLaunchImage;
}
QList<QWidget*> wrapWidget(QWidget* pWidget) {
    QList<QWidget*> result;
    if (pWidget) {result.append(pWidget);}
    return result;
}
QList<QWidget*> LegacySkinParser::parseNode(QDomElement node) {
    QList<QWidget*> result;
    auto nodeName = node.nodeName();
    //qDebug() << "parseNode" << node.nodeName();
    // TODO(rryan) replace with a map to function pointers?
    if (sDebug) {
        qDebug() << "BEGIN PARSE NODE" << nodeName;
    }
    // Root of the document
    if (nodeName == "skin") {
        // Parent all the skin widgets to an inner QWidget (this was MixxxView
        // in <=1.8, MixxxView was a subclass of QWidget), and then wrap it in
        // an outer widget. The Background parser parents the background image
        // to the inner widget but then sets the fill color of the outer widget
        // so that fullscreen will expand with the right color to fill in the
        // non-background areas. We put the inner widget in a layout inside the
        // outer widget so that it stays centered in fullscreen mode.

        // If the root widget has a layout we are loading a "new style" skin.
        QString layout = m_pContext->selectString(node, "Layout");
        bool newStyle = !layout.isEmpty();

        qDebug() << "Skin is a" << (newStyle ? ">=1.12.0" : "<1.12.0") << "style skin.";


        if (newStyle) {
            // New style skins are just a WidgetGroup at the root.
            result.append(parseWidgetGroup(node));
        } else {
            // From here on is loading for legacy skins only.
            QWidget* pOuterWidget = new QWidget(m_pParent);
            QWidget* pInnerWidget = new QWidget(pOuterWidget);

            // <Background> is only valid for old-style skins.
            QDomElement background = m_pContext->selectElement(node, "Background");
            if (!background.isNull()) {
                parseBackground(background, pOuterWidget, pInnerWidget);
            }

            // Interpret <Size>, <SizePolicy>, <Style>, etc. tags for the root node.
            setupWidget(node, pInnerWidget, false);

            m_pParent = pInnerWidget;

            // Legacy skins do not use a <Children> block.
            QDomNodeList children = node.childNodes();
            for (int i = 0; i < children.count(); ++i) {
                QDomNode node = children.at(i);
                if (node.isElement()) {
                    parseNode(node.toElement());
                }
            }

            // Keep innerWidget centered (for fullscreen).
            pOuterWidget->setLayout(new QHBoxLayout(pOuterWidget));
            pOuterWidget->layout()->setContentsMargins(0, 0, 0, 0);
            pOuterWidget->layout()->addWidget(pInnerWidget);
            result.append(pOuterWidget);
        }
    } else if (nodeName == "SliderComposed") {
        result = wrapWidget(parseStandardWidget<WSliderComposed>(node));
    } else if (nodeName == "PushButton") {
        result = wrapWidget(parseStandardWidget<WPushButton>(node));
    } else if (nodeName == "EffectPushButton") {
        result = wrapWidget(parseEffectPushButton(node));
    } else if (nodeName == "ComboBox") {
        result = wrapWidget(parseStandardWidget<WComboBox>(node));
    } else if (nodeName == "Overview") {
        result = wrapWidget(parseOverview(node));
    } else if (nodeName == "Visual") {
        result = wrapWidget(parseVisual(node));
    } else if (nodeName == "Text") {
        result = wrapWidget(parseText(node));
    } else if (nodeName == "TrackProperty") {
        result = wrapWidget(parseTrackProperty(node));
    } else if (nodeName == "StarRating") {
        result = wrapWidget(parseStarRating(node));
    } else if (nodeName == "VuMeter") {
        result = wrapWidget(parseStandardWidget<WVuMeter>(node, true));
    } else if (nodeName == "StatusLight") {
        result = wrapWidget(parseStandardWidget<WStatusLight>(node));
    } else if (nodeName == "Display") {
        result = wrapWidget(parseStandardWidget<WDisplay>(node));
    } else if (nodeName == "NumberRate") {
        result = wrapWidget(parseNumberRate(node));
    } else if (nodeName == "NumberPos") {
        result = wrapWidget(parseNumberPos(node));
    } else if (nodeName == "Number" || nodeName == "NumberBpm") {
        // NumberBpm is deprecated, and is now the same as a Number
        result = wrapWidget(parseLabelWidget<WNumber>(node));
    } else if (nodeName == "NumberDb") {
        result = wrapWidget(parseLabelWidget<WNumberDb>(node));
    } else if (nodeName == "Label") {
        result = wrapWidget(parseLabelWidget<WLabel>(node));
    } else if (nodeName == "Knob") {
        result = wrapWidget(parseStandardWidget<WKnob>(node));
    } else if (nodeName == "KnobComposed") {
        result = wrapWidget(parseStandardWidget<WKnobComposed>(node));
    } else if (nodeName == "TableView") {
        result = wrapWidget(parseTableView(node));
    } else if (nodeName == "CoverArt") {
        result = wrapWidget(parseCoverArt(node));
    } else if (nodeName == "SearchBox") {
        result = wrapWidget(parseSearchBox(node));
    } else if (nodeName == "WidgetGroup") {
        result = wrapWidget(parseWidgetGroup(node));
    } else if (nodeName == "WidgetStack") {
        result = wrapWidget(parseWidgetStack(node));
    } else if (nodeName == "SizeAwareStack") {
        result = wrapWidget(parseSizeAwareStack(node));
    } else if (nodeName == "EffectChainName") {
        result = wrapWidget(parseEffectChainName(node));
    } else if (nodeName == "EffectName") {
        result = wrapWidget(parseEffectName(node));
    } else if (nodeName == "EffectParameterName") {
        result = wrapWidget(parseEffectParameterName(node));
    } else if (nodeName == "EffectButtonParameterName") {
        result = wrapWidget(parseEffectButtonParameterName(node));
    } else if (nodeName == "Spinny") {
        result = wrapWidget(parseSpinny(node));
    } else if (nodeName == "Time") {
        result = wrapWidget(parseLabelWidget<WTime>(node));
    } else if (nodeName == "Splitter") {
        result = wrapWidget(parseSplitter(node));
    } else if (nodeName == "LibrarySidebar") {
        result = wrapWidget(parseLibrarySidebar(node));
    } else if (nodeName == "Library") {
        result = wrapWidget(parseLibrary(node));
    } else if (nodeName == "Key") {
        result = wrapWidget(parseEngineKey(node));
    } else if (nodeName == "SetVariable") {
        m_pContext->updateVariable(node);
    } else if (nodeName == "Template") {
        result = parseTemplate(node);
    } else if (nodeName == "SingletonDefinition") {
        parseSingletonDefinition(node);
    } else if (nodeName == "SingletonContainer") {
        result = wrapWidget(parseStandardWidget<WSingletonContainer>(node));
    } else {
        SKIN_WARNING(node, *m_pContext) << "Invalid node name in skin:"<< nodeName;
    }
    if (sDebug) {qDebug() << "END PARSE NODE" << nodeName;}
    return result;
}

QWidget* LegacySkinParser::parseSplitter(QDomElement node) {
    auto pSplitter = new WSplitter(m_pParent, m_pConfig);
    commonWidgetSetup(node, pSplitter);
    auto childrenNode = m_pContext->selectNode(node, "Children");
    auto pOldParent = m_pParent;
    m_pParent = pSplitter;
    if (!childrenNode.isNull()) {
        // Descend chilren
        for (auto child = childrenNode.firstChildElement();
             !child.isNull();child=child.nextSiblingElement()) {
              auto children = parseNode(child);
              for(auto  pChild: children) {
                  if (!pChild) continue;
                  pSplitter->addWidget(pChild);
            }
        }
    }
    pSplitter->setup(node, *m_pContext);
    pSplitter->Init();
    m_pParent = pOldParent;
    return pSplitter;
}

QWidget* LegacySkinParser::parseWidgetGroup(QDomElement node) {
    auto pGroup = new WWidgetGroup(m_pParent);
    commonWidgetSetup(node, pGroup);
    pGroup->setup(node, *m_pContext);
    pGroup->Init();
    auto childrenNode = m_pContext->selectNode(node, "Children");
    auto pOldParent = m_pParent;
    m_pParent = pGroup;
    if (!childrenNode.isNull()) {
        // Descend children
        auto children = childrenNode.childNodes();
        for (auto child = childrenNode.firstChildElement(); !child.isNull(); child=child.nextSiblingElement()) {
            for(auto pChild: parseNode(child)) { if (pChild) {pGroup->addWidget(pChild);}}
        }
    }
    m_pParent = pOldParent;
    return pGroup;
}
QWidget* LegacySkinParser::parseWidgetStack(QDomElement node) {
    auto createdNext = false;
    auto pNextControl = controlFromConfigNode( node.toElement(), "NextControl", &createdNext);
    auto createdPrev = false;
    auto pPrevControl = controlFromConfigNode( node.toElement(), "PrevControl", &createdPrev);
    auto createdCurrentPage = false;
    auto pCurrentPageControl = static_cast<ControlObject*>(nullptr);;
    auto currentpage_co = node.attribute("currentpage");
    if (currentpage_co.length() > 0) {
        auto configKey = ConfigKey::parseCommaSeparated(currentpage_co);
        auto persist_co = node.attribute("persist");
        auto persist = m_pContext->selectAttributeBool(node, "persist", false);
        pCurrentPageControl = controlFromConfigKey(configKey, persist,&createdCurrentPage);
    }
    auto pStack = new WWidgetStack(m_pParent, pNextControl, pPrevControl, pCurrentPageControl);
    pStack->setObjectName("WidgetStack");
    pStack->setContentsMargins(0, 0, 0, 0);
    commonWidgetSetup(node, pStack);
    if (createdNext && pNextControl) {pNextControl->setParent(pStack);}
    if (createdPrev && pPrevControl) {pPrevControl->setParent(pStack);}
    if (createdCurrentPage) {pCurrentPageControl->setParent(pStack);}
    auto pOldParent = m_pParent;
    m_pParent = pStack;
    auto childrenNode = m_pContext->selectNode(node, "Children");
    if (!childrenNode.isNull()) {
        // Descend chilren
        for(auto element = childrenNode.firstChildElement();
            !element.isNull();element=element.nextSiblingElement()){
            auto child_widgets = parseNode(element);
            if (child_widgets.empty()) {
                SKIN_WARNING(node, *m_pContext)
                        << "WidgetStack child produced no widget.";
                continue;
            }
            if (child_widgets.size() > 1) {
                SKIN_WARNING(node, *m_pContext)
                        << "WidgetStack child produced multiple widgets."
                        << "All but the first are ignored.";
            }
            auto pChild = child_widgets[0];
            if (!pChild ) {continue;}
            ControlObject* pControl = NULL;
            auto trigger_configkey = element.attribute("trigger");
            if (trigger_configkey.length() > 0) {
                auto configKey = ConfigKey::parseCommaSeparated(trigger_configkey);
                auto created = false;
                pControl = controlFromConfigKey(configKey, false, &created);
                if (created) {
                    // If we created the control, parent it to the child widget so
                    // it doesn't leak.
                    pControl->setParent(pChild);
                }
            }
            auto on_hide_select = -1;
            auto on_hide_attr = element.attribute("on_hide_select");
            if (on_hide_attr.length() > 0) {
                auto ok = false;
                on_hide_select = on_hide_attr.toInt(&ok);
                if (!ok) {on_hide_select = -1;}
            }
            pStack->addWidgetWithControl(pChild, pControl, on_hide_select);
        }
    }
    // Init the widget last now that all the children have been created,
    // so if the current page was saved we can switch to the correct page.
    pStack->Init();
    m_pParent = pOldParent;
    return pStack;
}
QWidget* LegacySkinParser::parseSizeAwareStack(QDomElement node) {
    WSizeAwareStack* pStack = new WSizeAwareStack(m_pParent);
    pStack->setObjectName("SizeAwareStack");
    pStack->setContentsMargins(0, 0, 0, 0);
    commonWidgetSetup(node, pStack);
    auto  pOldParent = m_pParent;
    m_pParent = pStack;
    auto childrenNode = m_pContext->selectNode(node, "Children");
    if (!childrenNode.isNull()) {
        // Descend chilren
        for ( auto element = childrenNode.firstChildElement();!element.isNull();
                   element = element.nextSiblingElement()){
            auto children = parseNode(element);
            if (children.empty()) {
                SKIN_WARNING(node, *m_pContext)
                        << "SizeAwareStack child produced no widget.";
                continue;
            }
            if (children.size() > 1) {
                SKIN_WARNING(node, *m_pContext)
                        << "SizeAwareStack child produced multiple widgets."
                        << "All but the first are ignored.";
            }
            if(auto pChild = children[0])
            {
              pStack->addWidget(pChild);
            }
        }
    }

    m_pParent = pOldParent;
    return pStack;
}

QWidget* LegacySkinParser::parseBackground(QDomElement node,
                                           QWidget* pOuterWidget,
                                           QWidget* pInnerWidget) {
    auto bg = new QLabel(pInnerWidget);
    auto filename = m_pContext->selectString(node, "Path");
    auto background = WPixmapStore::getPixmapNoCache(m_pContext->getSkinPath(filename));
    bg->move(0, 0);
    if (background && !background->isNull()) {bg->setPixmap(*background);}
    bg->lower();
    pInnerWidget->move(0,0);
    if (background && !background->isNull()) {
        pInnerWidget->setFixedSize(background->width(), background->height());
        pOuterWidget->setMinimumSize(background->width(), background->height());
    }
    // Default background color is now black, if people want to do <invert/>
    // filters they'll have to figure something out for this.
    auto c = QColor(0,0,0);
    if (m_pContext->hasNode(node, "BgColor")) {
        c.setNamedColor(m_pContext->selectString(node, "BgColor"));
    }
    QPalette palette;
    palette.setBrush(QPalette::Window, WSkinColor::getCorrectColor(c));
    pOuterWidget->setBackgroundRole(QPalette::Window);
    pOuterWidget->setPalette(palette);
    pOuterWidget->setAutoFillBackground(true);

    // WPixmapStore::getPixmapNoCache() allocated background and gave us
    // ownership. QLabel::setPixmap makes a copy, so we have to delete this.
    delete background;

    return bg;
}

template <class T>
QWidget* LegacySkinParser::parseStandardWidget(QDomElement element,bool timerListener) {
    auto pWidget = new T(m_pParent);
    if (timerListener) {WaveformWidgetFactory::instance()->addTimerListener(pWidget);}
    commonWidgetSetup(element, pWidget);
    pWidget->setup(element, *m_pContext);
    pWidget->installEventFilter(m_pKeyboard);
    pWidget->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    pWidget->Init();
    return pWidget;
}

template <class T>
QWidget* LegacySkinParser::parseLabelWidget(QDomElement element) {
    auto pLabel = new T(m_pParent);
    setupLabelWidget(element, pLabel);
    return pLabel;
}
void LegacySkinParser::setupLabelWidget(QDomElement element, WLabel* pLabel) {
    // NOTE(rryan): To support color schemes, the WWidget::setup() call must
    // come first. This is because WLabel derivatives change the palette based
    // on the node and setupWidget() will set the widget style. If the style is
    // set before the palette is set then the custom palette will not take
    // effect which breaks color scheme support.
    pLabel->setup(element, *m_pContext);
    commonWidgetSetup(element, pLabel);
    pLabel->installEventFilter(m_pKeyboard);
    pLabel->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    pLabel->Init();
}
QWidget* LegacySkinParser::parseOverview(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto pPlayer = m_pPlayerManager->getPlayer(channelStr);
    if (!pPlayer) return nullptr;
    WOverview* overviewWidget = nullptr;
    // "RGB" = "2", "HSV" = "1" or "Filtered" = "0" (LMH) waveform overview type
    auto type = m_pConfig->getValueString(ConfigKey("[Waveform]","WaveformOverviewType"), "0").toInt();
    if (type == 0) {
        overviewWidget = new WOverviewLMH(pSafeChannelStr, m_pConfig, m_pParent);
    } else if (type == 1) {
        overviewWidget = new WOverviewHSV(pSafeChannelStr, m_pConfig, m_pParent);
    } else {
        overviewWidget = new WOverviewRGB(pSafeChannelStr, m_pConfig, m_pParent);
    }
    connect(overviewWidget, SIGNAL(trackDropped(QString, QString)),
            m_pPlayerManager, SLOT(slotLoadToPlayer(QString, QString)));
    commonWidgetSetup(node, overviewWidget);
    overviewWidget->setup(node, *m_pContext);
    overviewWidget->installEventFilter(m_pKeyboard);
    overviewWidget->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    overviewWidget->Init();
    // Connect the player's load and unload signals to the overview widget.
    connect(pPlayer, SIGNAL(loadTrack(TrackPointer)),overviewWidget, SLOT(slotLoadNewTrack(TrackPointer)));
    connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),overviewWidget, SLOT(slotTrackLoaded(TrackPointer)));
    connect(pPlayer, SIGNAL(loadTrackFailed(TrackPointer)),overviewWidget, SLOT(slotUnloadTrack(TrackPointer)));
    connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),overviewWidget, SLOT(slotUnloadTrack(TrackPointer)));
    //just in case track already loaded
    overviewWidget->slotLoadNewTrack(pPlayer->getLoadedTrack());
    return overviewWidget;
}
QWidget* LegacySkinParser::parseVisual(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pPlayer = m_pPlayerManager->getPlayer(channelStr);
    auto pSafeChannelStr = safeChannelString(channelStr);
    if (!pPlayer) return nullptr;
    auto viewer = new WWaveformViewer(pSafeChannelStr, m_pConfig, m_pParent);
    viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();
    factory->setWaveformWidget(viewer, node, *m_pContext);
    //qDebug() << "::parseVisual: parent" << m_pParent << m_pParent->size();
    //qDebug() << "::parseVisual: viewer" << viewer << viewer->size();
    viewer->installEventFilter(m_pKeyboard);
    viewer->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    commonWidgetSetup(node, viewer);
    viewer->Init();
    // connect display with loading/unloading of tracks
    QObject::connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),viewer, SLOT(onTrackLoaded(TrackPointer)));
    QObject::connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),viewer, SLOT(onTrackUnloaded(TrackPointer)));
    connect(viewer, SIGNAL(trackDropped(QString, QString)),m_pPlayerManager, SLOT(slotLoadToPlayer(QString, QString)));
    // if any already loaded
    viewer->onTrackLoaded(pPlayer->getLoadedTrack());
    return viewer;
}
QWidget* LegacySkinParser::parseText(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto pPlayer = m_pPlayerManager->getPlayer(channelStr);
    if (!pPlayer)return nullptr;
    auto p = new WTrackText(pSafeChannelStr, m_pConfig, m_pParent);
    setupLabelWidget(node, p);
    connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),p, SLOT(slotTrackLoaded(TrackPointer)));
    connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),p, SLOT(slotTrackUnloaded(TrackPointer)));
    connect(p, SIGNAL(trackDropped(QString,QString)),m_pPlayerManager, SLOT(slotLoadToPlayer(QString,QString)));
    auto pTrack = pPlayer->getLoadedTrack();
    if (pTrack) { p->slotTrackLoaded(pTrack);}
    return p;
}
QWidget* LegacySkinParser::parseTrackProperty(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto pPlayer = m_pPlayerManager->getPlayer(channelStr);
    if (!pPlayer) return nullptr;
    auto p = new WTrackProperty(pSafeChannelStr, m_pConfig, m_pParent);
    setupLabelWidget(node, p);
    connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),p, SLOT(slotTrackLoaded(TrackPointer)));
    connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),p, SLOT(slotTrackUnloaded(TrackPointer)));
    connect(p, SIGNAL(trackDropped(QString,QString)),m_pPlayerManager, SLOT(slotLoadToPlayer(QString,QString)));
    if(auto pTrack = pPlayer->getLoadedTrack()){p->slotTrackLoaded(pTrack);}
    return p;
}
QWidget* LegacySkinParser::parseStarRating(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto pPlayer = m_pPlayerManager->getPlayer(channelStr);
    if (!pPlayer)return nullptr;
    auto p = new WStarRating(pSafeChannelStr, m_pParent);
    p->setup(node, *m_pContext);
    connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),p, SLOT(slotTrackLoaded(TrackPointer)));
    connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),p, SLOT(slotTrackUnloaded(TrackPointer)));
    if(auto pTrack = pPlayer->getLoadedTrack()){p->slotTrackLoaded(pTrack);}
    return p;
}
QWidget* LegacySkinParser::parseNumberRate(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto c = QColor(255,255,255);
    if (m_pContext->hasNode(node, "BgColor")) {
        c.setNamedColor(m_pContext->selectString(node, "BgColor"));
    }
    QPalette palette;
    //palette.setBrush(QPalette::Background, WSkinColor::getCorrectColor(c));
    palette.setBrush(QPalette::Button, Qt::NoBrush);
    auto p = new WNumberRate(pSafeChannelStr, m_pParent);
    setupLabelWidget(node, p);
    // TODO(rryan): Let's look at removing this palette change in 1.12.0. I
    // don't think it's needed anymore.
    p->setPalette(palette);
    return p;
}
QWidget* LegacySkinParser::parseNumberPos(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto p = new WNumberPos(pSafeChannelStr, m_pParent);
    setupLabelWidget(node, p);
    return p;
}
QWidget* LegacySkinParser::parseEngineKey(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    auto pSafeChannelStr = safeChannelString(channelStr);
    auto  pEngineKey = new WKey(pSafeChannelStr, m_pParent);
    setupLabelWidget(node, pEngineKey);
    return pEngineKey;
}
QWidget* LegacySkinParser::parseSpinny(QDomElement node) {
    auto channelStr = lookupNodeGroup(node);
    if (CmdlineArgs::Instance().getSafeMode()) {
        auto dummy = new WLabel(m_pParent);
        //: Shown when Mixxx is running in safe mode.
        dummy->setText(tr("Safe Mode Enabled"));
        return dummy;
    }
    auto spinny = new WSpinny(m_pParent, channelStr, m_pConfig,m_pVCManager);
    if (!spinny->isValid()) {
        delete spinny;
        auto dummy = new WLabel(m_pParent);
        //: Shown when Spinny can not be displayed. Please keep \n unchanged
        dummy->setText(tr("No OpenGL\nsupport."));
        return dummy;
    }
    commonWidgetSetup(node, spinny);
    WaveformWidgetFactory::instance()->addTimerListener(spinny);
    connect(spinny, SIGNAL(trackDropped(QString, QString)),m_pPlayerManager, SLOT(slotLoadToPlayer(QString, QString)));
    if(auto pPlayer = m_pPlayerManager->getPlayer(channelStr)){
        connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),spinny, SLOT(slotLoadTrack(TrackPointer)));
        connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),spinny, SLOT(slotReset()));
        // just in case a track is already loaded
        spinny->slotLoadTrack(pPlayer->getLoadedTrack());
    }
    spinny->setup(node, *m_pContext);
    spinny->installEventFilter(m_pKeyboard);
    spinny->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    spinny->Init();
    return spinny;
}
QWidget* LegacySkinParser::parseSearchBox(QDomElement node) {
    auto pLineEditSearch = new WSearchLineEdit(m_pParent);
    commonWidgetSetup(node, pLineEditSearch, false);
    pLineEditSearch->setup(node, *m_pContext);
    // Connect search box signals to the library
    connect(pLineEditSearch, SIGNAL(search(const QString&)),m_pLibrary, SIGNAL(search(const QString&)));
    connect(pLineEditSearch, SIGNAL(searchCleared()),m_pLibrary, SIGNAL(searchCleared()));
    connect(pLineEditSearch, SIGNAL(searchStarting()),m_pLibrary, SIGNAL(searchStarting()));
    connect(m_pLibrary, SIGNAL(restoreSearch(const QString&)),pLineEditSearch, SLOT(restoreSearch(const QString&)));
    return pLineEditSearch;
}
QWidget* LegacySkinParser::parseCoverArt(QDomElement node) {
    auto channel = lookupNodeGroup(node);
    auto pPlayer = m_pPlayerManager->getPlayer(channel);
    auto pCoverArt = new WCoverArt(m_pParent, m_pConfig, channel);
    commonWidgetSetup(node, pCoverArt);
    pCoverArt->setup(node, *m_pContext);
    // If no group was provided, hook the widget up to the Library.
    if (channel.isEmpty()) {
        // Connect cover art signals to the library
        connect(m_pLibrary, SIGNAL(switchToView(const QString&)),pCoverArt, SLOT(slotReset()));
        connect(m_pLibrary, SIGNAL(enableCoverArtDisplay(bool)),pCoverArt, SLOT(slotEnable(bool)));
        connect(m_pLibrary, SIGNAL(trackSelected(TrackPointer)),pCoverArt, SLOT(slotLoadTrack(TrackPointer)));
    } else if (pPlayer) {
        connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),pCoverArt, SLOT(slotLoadTrack(TrackPointer)));
        connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),pCoverArt, SLOT(slotReset()));
        connect(pCoverArt, SIGNAL(trackDropped(QString, QString)),m_pPlayerManager, SLOT(slotLoadToPlayer(QString, QString)));
        // just in case a track is already loaded
        pCoverArt->slotLoadTrack(pPlayer->getLoadedTrack());
    }
    return pCoverArt;
}
void LegacySkinParser::parseSingletonDefinition(QDomElement node) {
    auto objectName = m_pContext->selectString(node, "ObjectName");
    if (objectName.isEmpty()) {
        SKIN_WARNING(node, *m_pContext) << "SingletonDefinition requires an ObjectName";
    }
    auto childrenNode = m_pContext->selectNode(node, "Children");
    if (childrenNode.isNull()) {
        SKIN_WARNING(node, *m_pContext)
                << "SingletonDefinition requires a Children tag with one child";
    }
    // Descend chilren, taking the first valid element.
    auto element = childrenNode.firstChildElement();
    if (element.isNull()) {
        SKIN_WARNING(node, *m_pContext)
                << "SingletonDefinition Children node is empty";
        return;
    }
    auto  child_widgets = parseNode(element);
    if (child_widgets.empty()) {
        SKIN_WARNING(node, *m_pContext) << "SingletonDefinition child produced no widget.";
        return;
    } else if (child_widgets.size() > 1) {
        SKIN_WARNING(node, *m_pContext)
                << "SingletonDefinition child produced multiple widgets."
                << "All but the first are ignored.";
    }
    if(auto pChild = child_widgets[0])
    {
      pChild->setObjectName(objectName);
      m_pContext->defineSingleton(objectName, pChild);
      pChild->hide();
    }
    else {
        SKIN_WARNING(node, *m_pContext) << "SingletonDefinition child widget is NULL";
    }
}

QWidget* LegacySkinParser::parseLibrary(QDomElement node) {
    auto pLibraryWidget = new WLibrary(m_pParent);
    pLibraryWidget->installEventFilter(m_pKeyboard);
    pLibraryWidget->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    // Connect Library search signals to the WLibrary
    connect(m_pLibrary, SIGNAL(search(const QString&)),pLibraryWidget, SLOT(search(const QString&)));
    m_pLibrary->bindWidget(pLibraryWidget, m_pKeyboard);
    // This must come after the bindWidget or we will not style any of the
    // LibraryView's because they have not been added yet.
    commonWidgetSetup(node, pLibraryWidget, false);
    return pLibraryWidget;
}
QWidget* LegacySkinParser::parseLibrarySidebar(QDomElement node) {
    auto pLibrarySidebar = new WLibrarySidebar(m_pParent);
    pLibrarySidebar->installEventFilter(m_pKeyboard);
    pLibrarySidebar->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    m_pLibrary->bindSidebarWidget(pLibrarySidebar);
    commonWidgetSetup(node, pLibrarySidebar, false);
    return pLibrarySidebar;
}
QWidget* LegacySkinParser::parseTableView(QDomElement node) {
    auto pTabWidget = new QStackedWidget(m_pParent);
    setupPosition(node, pTabWidget);
    setupSize(node, pTabWidget);
    // set maximum width to prevent growing to qSplitter->sizeHint()
    // Note: sizeHint() may be greater in skins for tiny screens
    auto width = pTabWidget->minimumWidth();
    if (width == 0) {width = m_pParent->minimumWidth();}
    pTabWidget->setMaximumWidth(width);
    auto pLibraryPage = new QWidget(pTabWidget);
    auto pLibraryPageLayout = new QGridLayout(pLibraryPage);
    pLibraryPageLayout->setContentsMargins(0, 0, 0, 0);
    pLibraryPage->setLayout(pLibraryPageLayout);
    auto pSplitter = new QSplitter(pLibraryPage);
    auto oldParent = m_pParent;
    m_pParent = pSplitter;
    auto pLibraryWidget = parseLibrary(node);
    auto pLibrarySidebarPage = new QWidget(pSplitter);
    m_pParent = pLibrarySidebarPage;
    auto pLibrarySidebar = parseLibrarySidebar(node);
    auto pLineEditSearch = parseSearchBox(node);
    m_pParent = oldParent;
    auto pCoverArt = parseCoverArt(node);
    auto vl = new QVBoxLayout(pLibrarySidebarPage);
    vl->setContentsMargins(0,0,0,0); //Fill entire space
    vl->addWidget(pLineEditSearch);
    vl->addWidget(pLibrarySidebar);
    vl->addWidget(pCoverArt);
    pLibrarySidebarPage->setLayout(vl);
    pSplitter->addWidget(pLibrarySidebarPage);
    pSplitter->addWidget(pLibraryWidget);
    // TODO(rryan) can we make this more elegant?
    QList<int> splitterSizes;
    splitterSizes.push_back(50);
    splitterSizes.push_back(500);
    pSplitter->setSizes(splitterSizes);
    // Add the splitter to the library page's layout, so it's
    // positioned/sized automatically
    pLibraryPageLayout->addWidget(pSplitter,
                                  1, 0, //From row 1, col 0,
                                  1,    //Span 1 row
                                  3,    //Span 3 cols
                                  0);   //Default alignment

    pTabWidget->addWidget(pLibraryPage);
    pTabWidget->setStyleSheet(getLibraryStyle(node));
    return pTabWidget;
}
QString LegacySkinParser::getLibraryStyle(QDomNode node) {
    auto style = getStyleFromNode(node);
    // Workaround to support legacy color styling
    auto color = QColor(0,0,0);

    // Style the library preview button with a default image.
    QString styleHack = (
        "#LibraryPreviewButton { background: transparent; border: 0; }"
        "#LibraryPreviewButton:checked {"
        "  image: url(:/images/library/ic_library_preview_pause.png);"
        "}"
        "#LibraryPreviewButton:!checked {"
        "  image: url(:/images/library/ic_library_preview_play.png);"
        "}");
    // Style the library BPM Button with a default image
    styleHack.append(QString(
        "QPushButton#LibraryBPMButton { background: transparent; border: 0; }"
        "QPushButton#LibraryBPMButton:checked {image: url(:/images/library/ic_library_checked.png);}"
        "QPushButton#LibraryBPMButton:!checked {image: url(:/images/library/ic_library_unchecked.png);}"));

    if (m_pContext->hasNode(node, "FgColor")) {
        color.setNamedColor(m_pContext->selectString(node, "FgColor"));
        color = WSkinColor::getCorrectColor(color);
        styleHack.append(QString("WLibraryTableView { color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("WLibrarySidebar { color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("WSearchLineEdit { color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("QTextBrowser { color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("QLabel { color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("QRadioButton { color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("QSpinBox { color: %1; }\n ").arg(color.name()));
    }
    if (m_pContext->hasNode(node, "BgColor")) {
        color.setNamedColor(m_pContext->selectString(node, "BgColor"));
        color = WSkinColor::getCorrectColor(color);
        styleHack.append(QString("WLibraryTableView {  background-color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("WLibrarySidebar {  background-color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("WSearchLineEdit {  background-color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("QTextBrowser {  background-color: %1; }\n ").arg(color.name()));
        styleHack.append(QString("QSpinBox {  background-color: %1; }\n ").arg(color.name()));
    }
    if (m_pContext->hasNode(node, "BgColorRowEven")) {
        color.setNamedColor(m_pContext->selectString(node, "BgColorRowEven"));
        color = WSkinColor::getCorrectColor(color);
        styleHack.append(QString("WLibraryTableView { background: %1; }\n ").arg(color.name()));
    }
    if (m_pContext->hasNode(node, "BgColorRowUneven")) {
        color.setNamedColor(m_pContext->selectString(node, "BgColorRowUneven"));
        color = WSkinColor::getCorrectColor(color);
        styleHack.append(QString("WLibraryTableView { alternate-background-color: %1; }\n ").arg(color.name()));
    }
    style.prepend(styleHack);
    return style;
}
QDomElement LegacySkinParser::loadTemplate(const QString& path) {
    QFileInfo templateFileInfo(path);
    auto absolutePath = templateFileInfo.absoluteFilePath();
    if ( m_templateCache.contains(absolutePath ) ) return m_templateCache.value(absolutePath);
    QFile templateFile(absolutePath);
    if (!templateFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open template file:" << absolutePath;
    }
    QDomDocument tmpl("template");
    auto errorMessage = QString{};
    auto errorLine   = 0;
    auto errorColumn = 0;
    if (!tmpl.setContent(&templateFile, &errorMessage, &errorLine, &errorColumn)) {
        qWarning() << "LegacySkinParser::loadTemplate - setContent failed see"
                   << absolutePath << "line:" << errorLine << "column:" << errorColumn;
        qWarning() << "LegacySkinParser::loadTemplate - message:" << errorMessage;
        return QDomElement();
    }
    m_templateCache.insert(absolutePath,tmpl.documentElement());
    return tmpl.documentElement();
}
QList<QWidget*> LegacySkinParser::parseTemplate(QDomElement node) {
    if (!node.hasAttribute("src")) {
        SKIN_WARNING(node, *m_pContext)
                << "Template instantiation without src attribute:"
                << node.text();
        return QList<QWidget*>();
    }
    auto path = node.attribute("src");
    auto templateNode = loadTemplate(path);
    if (templateNode.isNull()) {
        SKIN_WARNING(node, *m_pContext) << "Template instantiation for template failed:" << path;
        return QList<QWidget*>();
    }
    if (sDebug) {qDebug() << "BEGIN TEMPLATE" << path;}
    auto pOldContext = m_pContext;
    m_pContext = new SkinContext(*pOldContext);
    // Take any <SetVariable> elements from this node and update the context
    // with them.
    m_pContext->updateVariables(node);
    m_pContext->setXmlPath(path);
    QList<QWidget*> widgets;
    QDomNode child = templateNode.firstChildElement();
    while (!child.isNull()) {
        auto childWidgets = parseNode(child.toElement());
        widgets.append(childWidgets);
        child = child.nextSiblingElement();
    }
    delete m_pContext;
    m_pContext = pOldContext;
    if (sDebug) {qDebug() << "END TEMPLATE" << path;}
    return widgets;
}
QString LegacySkinParser::lookupNodeGroup(QDomElement node) {
    auto group = m_pContext->selectString(node, "Group");
    // If the group is not present, then check for a Channel, since legacy skins
    // will specify the channel as either 1 or 2.
    if (group.size() == 0) {
        auto channel = m_pContext->selectInt(node, "Channel");
        if (channel > 0) {
            // groupForDeck is 0-indexed
            group = PlayerManager::groupForDeck(channel - 1);
        }
    }
    return group;
}
// static
const char* LegacySkinParser::safeChannelString(QString channelStr) {
    QMutexLocker lock(&s_safeStringMutex);
    for(auto s: s_channelStrs) {if (channelStr == QString{s}) {return s;}}
    auto qba = channelStr.toLocal8Bit();
    auto safe = new char[qba.size() + 1]; // +1 for \0
    auto i = 0;
    // Copy string
    while ((safe[i] = qba[i])) ++i;
    s_channelStrs.append(safe);
    return safe;
}
QWidget* LegacySkinParser::parseEffectChainName(QDomElement node) {
    auto pEffectChain = new WEffectChain(m_pParent, m_pEffectsManager);
    setupLabelWidget(node, pEffectChain);
    return pEffectChain;
}
QWidget* LegacySkinParser::parseEffectName(QDomElement node) {
    auto pEffect = new WEffect(m_pParent, m_pEffectsManager);
    setupLabelWidget(node, pEffect);
    return pEffect;
}
QWidget* LegacySkinParser::parseEffectPushButton(QDomElement element) {
    auto pWidget = new WEffectPushButton(m_pParent, m_pEffectsManager);
    commonWidgetSetup(element, pWidget);
    pWidget->setup(element, *m_pContext);
    pWidget->installEventFilter(m_pKeyboard);
    pWidget->installEventFilter(m_pControllerManager->getControllerLearningEventFilter());
    pWidget->Init();
    return pWidget;
}
QWidget* LegacySkinParser::parseEffectParameterName(QDomElement node) {
    auto pEffectParameter = new WEffectParameter(m_pParent, m_pEffectsManager);
    setupLabelWidget(node, pEffectParameter);
    return pEffectParameter;
}
QWidget* LegacySkinParser::parseEffectButtonParameterName(QDomElement node) {
    auto pEffectButtonParameter = new WEffectButtonParameter(m_pParent, m_pEffectsManager);
    setupLabelWidget(node, pEffectButtonParameter);
    return pEffectButtonParameter;
}
void LegacySkinParser::setupPosition(QDomNode node, QWidget* pWidget) {
    if (m_pContext->hasNode(node, "Pos")) {
        auto pos = m_pContext->selectString(node, "Pos");
        auto x = pos.left(pos.indexOf(",")).toInt();
        auto y = pos.mid(pos.indexOf(",")+1).toInt();
        pWidget->move(x,y);
    }
}
bool parseSizePolicy(QString* input, QSizePolicy::Policy* policy) {
    if (input->endsWith("me")) {
        *policy = QSizePolicy::MinimumExpanding;
        *input = input->left(input->size()-2);
    } else if (input->endsWith("e")) {
        *policy = QSizePolicy::Expanding;
        *input = input->left(input->size()-1);
    } else if (input->endsWith("i")) {
        *policy = QSizePolicy::Ignored;
        *input = input->left(input->size()-1);
    } else if (input->endsWith("min")) {
        *policy = QSizePolicy::Minimum;
        *input = input->left(input->size()-3);
    } else if (input->endsWith("max")) {
        *policy = QSizePolicy::Maximum;
        *input = input->left(input->size()-3);
    } else if (input->endsWith("p")) {
        *policy = QSizePolicy::Preferred;
        *input = input->left(input->size()-1);
    } else if (input->endsWith("f")) {
        *policy = QSizePolicy::Fixed;
        *input = input->left(input->size()-1);
    } else { return false;
    }
    return true;
}

void LegacySkinParser::setupSize(QDomNode node, QWidget* pWidget) {
    if (m_pContext->hasNode(node, "MinimumSize")) {
        QString size = m_pContext->selectString(node, "MinimumSize");
        int comma = size.indexOf(",");
        QString xs = size.left(comma);
        QString ys = size.mid(comma+1);

        bool widthOk = false;
        int x = xs.toInt(&widthOk);

        bool heightOk = false;
        int y = ys.toInt(&heightOk);

        // -1 means do not set.
        if (widthOk && heightOk && x >= 0 && y >= 0) {
            pWidget->setMinimumSize(x, y);
        } else if (widthOk && x >= 0) {
            pWidget->setMinimumWidth(x);
        } else if (heightOk && y >= 0) {
            pWidget->setMinimumHeight(y);
        } else if (!widthOk && !heightOk) {
            SKIN_WARNING(node, *m_pContext)
                    << "Could not parse widget MinimumSize:" << size;
        }
    }

    if (m_pContext->hasNode(node, "MaximumSize")) {
        QString size = m_pContext->selectString(node, "MaximumSize");
        int comma = size.indexOf(",");
        QString xs = size.left(comma);
        QString ys = size.mid(comma+1);

        bool widthOk = false;
        int x = xs.toInt(&widthOk);

        bool heightOk = false;
        int y = ys.toInt(&heightOk);

        // -1 means do not set.
        if (widthOk && heightOk && x >= 0 && y >= 0) {
            pWidget->setMaximumSize(x, y);
        } else if (widthOk && x >= 0) {
            pWidget->setMaximumWidth(x);
        } else if (heightOk && y >= 0) {
            pWidget->setMaximumHeight(y);
        } else if (!widthOk && !heightOk) {
            SKIN_WARNING(node, *m_pContext)
                    << "Could not parse widget MaximumSize:" << size;
        }
    }

    bool hasSizePolicyNode = false;
    if (m_pContext->hasNode(node, "SizePolicy")) {
        QString size = m_pContext->selectString(node, "SizePolicy");
        int comma = size.indexOf(",");
        QString xs = size.left(comma);
        QString ys = size.mid(comma+1);

        QSizePolicy sizePolicy = pWidget->sizePolicy();

        QSizePolicy::Policy horizontalPolicy;
        if (parseSizePolicy(&xs, &horizontalPolicy)) {
            sizePolicy.setHorizontalPolicy(horizontalPolicy);
        } else if (!xs.isEmpty()) {
            SKIN_WARNING(node, *m_pContext)
                    << "Could not parse horizontal size policy:" << xs;
        }

        QSizePolicy::Policy verticalPolicy;
        if (parseSizePolicy(&ys, &verticalPolicy)) {
            sizePolicy.setVerticalPolicy(verticalPolicy);
        } else if (!ys.isEmpty()) {
            SKIN_WARNING(node, *m_pContext)
                    << "Could not parse vertical size policy:" << ys;
        }

        hasSizePolicyNode = true;
        pWidget->setSizePolicy(sizePolicy);
    }

    if (m_pContext->hasNode(node, "Size")) {
        auto size = m_pContext->selectString(node, "Size");
        auto comma = size.indexOf(",");
        auto xs = size.left(comma);
        auto ys = size.mid(comma+1);
        auto sizePolicy = pWidget->sizePolicy();
        auto hasHorizontalPolicy = false;
        QSizePolicy::Policy horizontalPolicy;
        if (parseSizePolicy(&xs, &horizontalPolicy)) {
            sizePolicy.setHorizontalPolicy(horizontalPolicy);
            hasHorizontalPolicy = true;
        }
        auto hasVerticalPolicy = false;
        QSizePolicy::Policy verticalPolicy;
        if (parseSizePolicy(&ys, &verticalPolicy)) {
            sizePolicy.setVerticalPolicy(verticalPolicy);
            hasVerticalPolicy = true;
        }
        auto widthOk = false;
        auto x = xs.toInt(&widthOk);
        if (widthOk) {
            if (hasHorizontalPolicy && sizePolicy.horizontalPolicy() == QSizePolicy::Fixed) {
                //qDebug() << "setting width fixed to" << x;
                pWidget->setFixedWidth(x);
            } else {pWidget->setMinimumWidth(x);}
        }

        auto heightOk = false;
        auto y = ys.toInt(&heightOk);
        if (heightOk) {
            if (hasVerticalPolicy && sizePolicy.verticalPolicy() == QSizePolicy::Fixed) {
                //qDebug() << "setting height fixed to" << x;
                pWidget->setFixedHeight(y);
            } else {
                //qDebug() << "setting height to" << y;
                pWidget->setMinimumHeight(y);
            }
        }
        if (!hasSizePolicyNode) {pWidget->setSizePolicy(sizePolicy);}
    }
}
QString LegacySkinParser::getStyleFromNode(QDomNode node) {
    auto styleElement = m_pContext->selectElement(node, "Style");
    if (styleElement.isNull()) {return QString();}
    auto style = QString{};
    if (styleElement.hasAttribute("src")) {
        auto styleSrc = styleElement.attribute("src");
        QFile file(styleSrc);
        if (file.open(QIODevice::ReadOnly)) {
            auto fileBytes = file.readAll();
            style = QString::fromLocal8Bit(fileBytes.constData(),fileBytes.length());
        }
    } else {style = styleElement.text();}
    // Legacy fixes: In Mixxx <1.12.0 we used QGroupBox for WWidgetGroup. Some
    // skin writers used QGroupBox for styling. In 1.12.0 onwards, we have
    // switched to QFrame and there should be no reason we would ever use a
    // QGroupBox in a skin. To support legacy skins, we rewrite QGroupBox
    // selectors to use WWidgetGroup directly.
    style = style.replace("QGroupBox", "WWidgetGroup");
    return style;
}
void LegacySkinParser::commonWidgetSetup(QDomNode node,WBaseWidget* pBaseWidget,bool allowConnections) {
    setupBaseWidget(node, pBaseWidget);
    setupWidget(node, pBaseWidget->toQWidget());
    // NOTE(rryan): setupConnections should come after setupBaseWidget and
    // setupWidget since a BindProperty connection to the display parameter can
    // cause the widget to be polished (i.e. style computed) before it is
    // ready. The most common case is that the object name has not yet been set
    // in setupWidget. See Bug #1285836.
    if (allowConnections) setupConnections(node, pBaseWidget);
}
void LegacySkinParser::setupBaseWidget(QDomNode node,WBaseWidget* pBaseWidget) {
    // Tooltip
    if (m_pContext->hasNode(node, "Tooltip")) {
        auto toolTip = m_pContext->selectString(node, "Tooltip");
        pBaseWidget->prependBaseTooltip(toolTip);
    } else if (m_pContext->hasNode(node, "TooltipId")) {
        auto toolTipId = m_pContext->selectString(node, "TooltipId");
        auto toolTip = m_tooltips.tooltipForId(toolTipId);
        if (!toolTip.isEmpty()) {
            pBaseWidget->prependBaseTooltip(toolTip);
        } else if (!toolTipId.isEmpty()) {
            // Only warn if there was a tooltip ID specified and no tooltip for
            // that ID.
            SKIN_WARNING(node, *m_pContext) << "Invalid <TooltipId> in skin.xml:" << toolTipId;
        }
    }
}
void LegacySkinParser::setupWidget(QDomNode node,QWidget* pWidget,bool setPosition) {
    // Override the widget object name.
    auto objectName = m_pContext->selectString(node, "ObjectName");
    if (!objectName.isEmpty()) pWidget->setObjectName(objectName);
    if (setPosition) setupPosition(node, pWidget);
    setupSize(node, pWidget);
    auto style = getStyleFromNode(node);
    // Check if we should apply legacy library styling to this node.
    if (m_pContext->selectBool(node, "LegacyTableViewStyle", false)) style = getLibraryStyle(node);
    if (!style.isEmpty()) pWidget->setStyleSheet(style);
}

void LegacySkinParser::setupConnections(QDomNode node, WBaseWidget* pWidget) {
    // For each connection
    auto con = m_pContext->selectNode(node, "Connection");
    auto pLastLeftOrNoButtonConnection = static_cast<ControlParameterWidgetConnection*>(nullptr);
    for (auto con = m_pContext->selectNode(node, "Connection");!con.isNull();con = con.nextSibling()) {
        // Check that the control exists
        auto created = false;
        auto control = controlFromConfigNode(con.toElement(), "ConfigKey", &created);
        if (!control) {continue;}
        auto bInvert = false;
        if (m_pContext->hasNode(con, "Transform")) {
            auto element = m_pContext->selectElement(con, "Transform");
            bInvert = true;
        }
        if (m_pContext->hasNode(con, "BindProperty")) {
            auto property = m_pContext->selectString(con, "BindProperty");
            //qDebug() << "Making property connection for" << property;
            auto pControlWidget =new ControlObjectSlave(control->getKey(),pWidget->toQWidget());
            auto pConnection = new ControlWidgetPropertyConnection(pWidget, pControlWidget, property);
            pConnection->setInvert(bInvert);
            pWidget->addPropertyConnection(pConnection);
            // If we created this control, bind it to the
            // ControlWidgetConnection so that it is deleted when the connection
            // is deleted.
            if (created) {control->setParent(pConnection);}
        } else {
            auto nodeValue = false;
            Qt::MouseButton state = parseButtonState(con, *m_pContext);
            auto directionOptionSet = false;
            auto directionOption = static_cast<int>(ControlParameterWidgetConnection::DIR_FROM_AND_TO_WIDGET);
            if(m_pContext->hasNodeSelectBool(con, "ConnectValueFromWidget", &nodeValue)) {
                if (nodeValue) {
                    directionOption = directionOption | ControlParameterWidgetConnection::DIR_FROM_WIDGET;
                } else {
                    directionOption = directionOption & ~ControlParameterWidgetConnection::DIR_FROM_WIDGET;
                }
                directionOptionSet = true;
            }
            if(m_pContext->hasNodeSelectBool(con, "ConnectValueToWidget", &nodeValue)) {
                if (nodeValue) {
                    directionOption = directionOption | ControlParameterWidgetConnection::DIR_TO_WIDGET;
                } else {
                    directionOption = directionOption & ~ControlParameterWidgetConnection::DIR_TO_WIDGET;
                }
                directionOptionSet = true;
            }
            if (!directionOptionSet) {
                // default:
                // no direction option is explicite set
                // Set default flag to allow the widget to change this during setup
                directionOption |= ControlParameterWidgetConnection::DIR_DEFAULT;
            }
            auto emitOption = static_cast<int>(ControlParameterWidgetConnection::EMIT_ON_PRESS);
            if(m_pContext->hasNodeSelectBool(con, "EmitOnDownPress", &nodeValue)) {
                if (nodeValue) {
                    emitOption = ControlParameterWidgetConnection::EMIT_ON_PRESS;
                } else {
                    emitOption = ControlParameterWidgetConnection::EMIT_ON_RELEASE;
                }
            } else if(m_pContext->hasNodeSelectBool(con, "EmitOnPressAndRelease", &nodeValue)) {
                if (nodeValue) {
                    emitOption = ControlParameterWidgetConnection::EMIT_ON_PRESS_AND_RELEASE;
                } else {
                    SKIN_WARNING(con, *m_pContext)
                            << "LegacySkinParser::setupConnections(): EmitOnPressAndRelease must not set false";
                }
            } else {
                // default:
                // no emit option is set
                // Allow to change the emitOption from Widget
                emitOption |= ControlParameterWidgetConnection::EMIT_DEFAULT;
            }
            // Connect control proxy to widget. Parented to pWidget so it is not
            // leaked.
            auto pControlWidget = new ControlObjectSlave(control->getKey(), pWidget->toQWidget());
            auto pConnection = new ControlParameterWidgetConnection(pWidget, pControlWidget,
                    static_cast<ControlParameterWidgetConnection::DirectionOption>(directionOption),
                    static_cast<ControlParameterWidgetConnection::EmitOption>(emitOption));
            pConnection->setInvert(bInvert);
            // If we created this control, bind it to the
            // ControlWidgetConnection so that it is deleted when the connection
            // is deleted.
            if (created) {control->setParent(pConnection);}
            switch (state) {
            case Qt::NoButton:
                pWidget->addConnection(pConnection);
                if (directionOption & ControlParameterWidgetConnection::DIR_TO_WIDGET) 
                    pLastLeftOrNoButtonConnection = pConnection;
                break;
            case Qt::LeftButton:
                pWidget->addLeftConnection(pConnection);
                if (directionOption & ControlParameterWidgetConnection::DIR_TO_WIDGET)
                    pLastLeftOrNoButtonConnection = pConnection;
                break;
            case Qt::RightButton:
                pWidget->addRightConnection(pConnection);
                break;
            default:
                break;
            }
            // We only add info for controls that this widget affects, not
            // controls that only affect the widget.
            if (directionOption & ControlParameterWidgetConnection::DIR_FROM_WIDGET) {
                m_pControllerManager->getControllerLearningEventFilter()
                        ->addWidgetClickInfo(pWidget->toQWidget(), state, control,
                                static_cast<ControlParameterWidgetConnection::EmitOption>(emitOption));
                // Add keyboard shortcut info to tooltip string
                auto key = m_pContext->selectString(con, "ConfigKey");
                auto configKey = ConfigKey::parseCommaSeparated(key);
                // do not add Shortcut string for feedback connections
                auto shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(configKey);
                addShortcutToToolTip(pWidget, shortcut, QString(""));
                const WSliderComposed* pSlider;
                if (qobject_cast<const  WPushButton*>(pWidget->toQWidget())) {
                    // check for "_activate", "_toggle"
                    auto subkey = ConfigKey{};
                    auto shortcut = QString{};
                    subkey = configKey;
                    subkey.item += "_activate";
                    shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                    addShortcutToToolTip(pWidget, shortcut, tr("activate"));

                    subkey = configKey;
                    subkey.item += "_toggle";
                    shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                    addShortcutToToolTip(pWidget, shortcut, tr("toggle"));
                } else if ((pSlider = qobject_cast<const WSliderComposed*>(pWidget->toQWidget()))) {
                    // check for "_up", "_down", "_up_small", "_down_small"
                    ConfigKey subkey;
                    QString shortcut;

                    if (pSlider->isHorizontal()) {
                        subkey = configKey;
                        subkey.item += "_up";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("right"));

                        subkey = configKey;
                        subkey.item += "_down";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("left"));

                        subkey = configKey;
                        subkey.item += "_up_small";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("right small"));

                        subkey = configKey;
                        subkey.item += "_down_small";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("left small"));
                    } else {
                        subkey = configKey;
                        subkey.item += "_up";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("up"));

                        subkey = configKey;
                        subkey.item += "_down";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("down"));

                        subkey = configKey;
                        subkey.item += "_up_small";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("up small"));

                        subkey = configKey;
                        subkey.item += "_down_small";
                        shortcut = m_pKeyboard->getKeyboardConfig()->getValueString(subkey);
                        addShortcutToToolTip(pWidget, shortcut, tr("down small"));
                    }
                }
            }
        }
    }
    // Legacy behavior: The last left-button or no-button connection with
    // connectValueToWidget is the display connection. If no left-button or
    // no-button connection exists, use the last right-button connection as the
    // display connection.
    if (pLastLeftOrNoButtonConnection) { pWidget->setDisplayConnection(pLastLeftOrNoButtonConnection);}
}
void LegacySkinParser::addShortcutToToolTip(WBaseWidget* pWidget, const QString& shortcut, const QString& cmd) {
    if (shortcut.isEmpty()) {return;}
    auto tooltip = QString{};
    // translate shortcut to native text
    auto nativeShortcut = QKeySequence(shortcut, QKeySequence::PortableText).toString(QKeySequence::NativeText);
    tooltip += "\n";
    tooltip += tr("Shortcut");
    if (!cmd.isEmpty()) {
        tooltip += " ";
        tooltip += cmd;
    }
    tooltip += ": ";
    tooltip += nativeShortcut;
    pWidget->appendBaseTooltip(tooltip);
}
QString LegacySkinParser::parseLaunchImageStyle(QDomNode node) {return m_pContext->selectString(node, "LaunchImageStyle");}
