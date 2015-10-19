#include <set>
#include <QtDebug>

#include "waveformmarkset.h"
#include "engine/cuecontrol.h"
#include "controlobject.h"
WaveformMarkSet::WaveformMarkSet() = default;
WaveformMarkSet::~WaveformMarkSet()
{
    clear();
}
void WaveformMarkSet::setup(const QString& group, const QDomNode& node,
                            const SkinContext& context,
                            const WaveformSignalColors& signalColors)
{

    clear();
    std::set<QString> controlItemSet;
    bool hasDefaultMark = false;
    auto child = node.firstChild();
    while (!child.isNull())
    {
        if (child.nodeName() == "DefaultMark")
        {
            m_defaultMark.setup(group, child, context, signalColors);
            hasDefaultMark = true;
        }
        else if (child.nodeName() == "Mark")
        {
            m_marks.push_back(WaveformMark());
            auto& mark = m_marks.back();
            mark.setup(group, child, context, signalColors);
            if (mark.m_pointControl)
            {
                // guarantee uniqueness even if there is a misdesigned skin
                auto item = mark.m_pointControl->getKey().item;
                if (!controlItemSet.insert(item).second)
                {
                    qWarning() << "WaveformRenderMark::setup - redefinition of" << item;
                    m_marks.removeAt(m_marks.size() - 1);
                }
            }
        }
        child = child.nextSibling();
    }
    // check if there is a default mark and compare declared
    // and to create all missing hot_cues
    if (hasDefaultMark)
    {
        for (int i = 1; i < NUM_HOT_CUES; ++i) {
            auto hotCueControlItem = QString{"hotcue_" + QString::number(i) + "_position"};
            auto pHotcue = ControlObject::getControl(ConfigKey(group, hotCueControlItem));
            if (!pHotcue )continue;
            if (controlItemSet.insert(hotCueControlItem).second)
            {
                //qDebug() << "WaveformRenderMark::setup - Automatic mark" << hotCueControlItem;
                m_marks.push_back(m_defaultMark);
                m_marks.back().setKeyAndIndex(pHotcue->getKey(),i);
            }
        }
    }
}
int WaveformMarkSet::size() const
{
  return m_marks.size();
}
WaveformMark& WaveformMarkSet::operator[] ( int i )
{
  return m_marks[i];
}
const WaveformMark& WaveformMarkSet::at(int i )
{
  return m_marks.at(i);
}
const WaveformMark& WaveformMarkSet::getDefaultMark() const
{
  return m_defaultMark;
}
void WaveformMarkSet::clear()
{
    m_defaultMark = WaveformMark();
    m_marks.clear();
}
