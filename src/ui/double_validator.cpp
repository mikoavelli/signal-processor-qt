/// @file double_validator.cpp
/// @brief Double validator that accepts both '.' and ',' as the decimal separator.
#include "double_validator.h"

#include <QLocale>

namespace sp::ui {

DoubleValidator::DoubleValidator(double bottom, double top, int decimals, QObject *parent)
    : QDoubleValidator(bottom, top, decimals, parent)
{
    setLocale(QLocale::c());
    setNotation(QDoubleValidator::StandardNotation);
}

QValidator::State DoubleValidator::validate(QString &input, int &pos) const
{
    QString normalized = input;
    normalized.replace(u',', u'.');
    int normalizedPos = pos;
    const QValidator::State state = QDoubleValidator::validate(normalized, normalizedPos);
    pos = normalizedPos;
    return state;
}

} // namespace sp::ui
