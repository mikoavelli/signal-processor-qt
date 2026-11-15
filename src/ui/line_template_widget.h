/// @file line_template_widget.h
/// @brief Widget that renders a line-name template with editable “{}” fields.
#pragma once

#include <QWidget>
#include <QList>

class QHBoxLayout;
class QLineEdit;

namespace sp::ui {

/// @brief Renders a template like “ЛВС, пара {} - {}” as labels and edits.
///
/// The template is invalid when, after removing every “{}” placeholder, any
/// brace remains (e.g. “Line {param”): such templates refuse validation.
class LineTemplateWidget : public QWidget {
    Q_OBJECT

public:
    explicit LineTemplateWidget(QWidget* parent = nullptr);

    /// @brief Sets the template to render.
    /// @param value Template string; “{}” marks a field for user input.
    void setTemplate(const QString& value);

    /// @brief Clears all input fields.
    void clearInputs();

    /// @brief @c true if the template is valid and every field is filled.
    [[nodiscard]] bool isValid() const;

    /// @brief @c true if the template text had unbalanced braces.
    [[nodiscard]] bool hasTemplateError() const;

    /// @brief Returns the template with the fields substituted.
    [[nodiscard]] QString fullName() const;

    /// @brief Moves focus to the first empty field, if any.
    void focusFirstEmpty();

private:
    void clearLayout();

    QString m_template;              ///< Raw template text.
    bool m_templateError = false;    ///< @c true if the template had unbalanced braces.
    QList<QLineEdit*> m_inputs;      ///< Edits created for the “{}” fields.
    QHBoxLayout* m_layout = nullptr; ///< Content layout.
};

} // namespace sp::ui
