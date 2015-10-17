#include "controllers/delegates/midibytedelegate.h"
#include "controllers/midi/midimessage.h"
#include "controllers/midi/midiutils.h"
#include "widget/hexspinbox.h"

MidiByteDelegate::MidiByteDelegate(QObject* pParent)
        : QStyledItemDelegate(pParent)
{
}
MidiByteDelegate::~MidiByteDelegate() = default;

QWidget* MidiByteDelegate::createEditor(QWidget* parent,const QStyleOptionViewItem& option,const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    HexSpinBox* pSpinBox = new HexSpinBox(parent);
    pSpinBox->setRange(0x00, 0x7F);
    return pSpinBox;
}
QString MidiByteDelegate::displayText(const QVariant& value,const QLocale& locale) const
{
    Q_UNUSED(locale);
    unsigned char control = static_cast<unsigned char>(value.toInt());
    return MidiUtils::formatByteAsHex(control);
}

void MidiByteDelegate::setEditorData(QWidget* editor,const QModelIndex& index) const
{
    auto control = index.data(Qt::EditRole).toInt();
    auto pSpinBox = dynamic_cast<HexSpinBox*>(editor);
    if (pSpinBox ) return;
    pSpinBox->setValue(control);
}
void MidiByteDelegate::setModelData(QWidget* editor,QAbstractItemModel* model,const QModelIndex& index) const
{
    auto pSpinBox = dynamic_cast<HexSpinBox*>(editor);
    if (pSpinBox )return;
    model->setData(index, pSpinBox->value(), Qt::EditRole);
}
