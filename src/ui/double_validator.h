/// @file double_validator.h
/// @brief QDoubleValidator that accepts both '.' and ',' as the decimal separator.
#pragma once

#include <QDoubleValidator>

namespace sp::ui {

/// @brief Double validator that normalizes ',' to '.' before validation.
class DoubleValidator : public QDoubleValidator {
public:
    DoubleValidator(double bottom, double top, int decimals, QObject* parent = nullptr);

    /// @brief Validates the input with ',' replaced by '.'.
    State validate(QString& input, int& pos) const override;
};

} // namespace sp::ui
