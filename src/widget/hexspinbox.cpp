#include "widget/hexspinbox.h"

HexSpinBox::HexSpinBox(QWidget* pParent)
        : QSpinBox(pParent) {
    setRange(0, 255);
}
HexSpinBox::~HexSpinBox() = default;
QString HexSpinBox::textFromValue(int value) const
{
    // Construct a hex string formatted like 0xFF.
    return QString("0x") + QString("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
}
int HexSpinBox::valueFromText(const QString& text) const
{
    auto ok = false;
    return text.toInt(&ok, 16);
}
QValidator::State HexSpinBox::validate(QString& input, int& pos) const
{
    auto regExp = QRegExp("^0(x|X)[0-9A-Fa-f]+");
    QRegExpValidator validator(regExp, NULL);
    return validator.validate(input, pos);
}
