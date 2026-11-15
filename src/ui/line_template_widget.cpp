/// @file line_template_widget.cpp
/// @brief Widget that renders a line-name template with editable “{}” fields.
#include "line_template_widget.h"

#include "i18n.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QList>
#include <QWidget>
#include <utility>

namespace sp::ui {

LineTemplateWidget::LineTemplateWidget(QWidget* parent)
    : QWidget(parent), m_layout(new QHBoxLayout(this)) {
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(4);
    m_layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void LineTemplateWidget::setTemplate(const QString& value) {
    m_template = value;
    m_templateError = false;
    clearLayout();

    const QStringList parts = value.split(QStringLiteral("{}"));
    // After removing every valid “{}” placeholder, no brace may remain.
    for (const QString& part : parts) {
        if (part.contains(u'{') || part.contains(u'}')) {
            m_templateError = true;
            break;
        }
    }
    const int placeholderCount = static_cast<int>(parts.size()) - 1;
    if (placeholderCount <= 0 || m_templateError) {
        setVisible(false);
        return;
    }

    setVisible(true);
    m_layout->addWidget(new QLabel(tr_ns::tr("window.label.params"), this));

    QLineEdit* previousInput = nullptr;
    for (int index = 0; index < parts.size(); ++index) {
        const QString cleanText = parts.at(index).trimmed();
        if (!cleanText.isEmpty()) {
            m_layout->addWidget(new QLabel(cleanText, this));
        }
        if (index < placeholderCount) {
            QLineEdit* input = new QLineEdit(this);
            input->setFixedSize(46, 26);
            input->setAlignment(Qt::AlignCenter);
            input->setPlaceholderText(tr_ns::tr("window.params.placeholder"));
            m_layout->addWidget(input);
            m_inputs.append(input);
            if (previousInput != nullptr) {
                QWidget::setTabOrder(previousInput, input);
            }
            previousInput = input;
        }
    }

    m_layout->addStretch();
    if (!m_inputs.isEmpty()) {
        m_inputs.constFirst()->setFocus();
    }
}

void LineTemplateWidget::clearInputs() {
    for (QLineEdit* input : std::as_const(m_inputs)) {
        input->clear();
    }
    if (!m_inputs.isEmpty()) {
        m_inputs.constFirst()->setFocus();
    }
}

bool LineTemplateWidget::isValid() const {
    if (m_templateError) {
        return false;
    }
    if (!isVisible() || m_inputs.isEmpty()) {
        return true;
    }
    for (const QLineEdit* input : m_inputs) {
        if (input->text().trimmed().isEmpty()) {
            return false;
        }
    }
    return true;
}

bool LineTemplateWidget::hasTemplateError() const {
    return m_templateError;
}

QString LineTemplateWidget::fullName() const {
    if (m_templateError || !isVisible() || m_inputs.isEmpty()) {
        return m_template;
    }
    QString result = m_template;
    for (const QLineEdit* input : m_inputs) {
        const qsizetype position = result.indexOf(u"{}");
        if (position < 0) {
            break;
        }
        result.replace(position, 2, input->text().trimmed());
    }
    return result;
}

void LineTemplateWidget::focusFirstEmpty() {
    if (!isVisible()) {
        return;
    }
    for (QLineEdit* input : std::as_const(m_inputs)) {
        if (input->text().trimmed().isEmpty()) {
            input->setFocus();
            input->selectAll();
            return;
        }
    }
}

void LineTemplateWidget::clearLayout() {
    m_inputs.clear();
    while (m_layout->count() > 0) {
        QLayoutItem* item = m_layout->takeAt(0);
        if (item == nullptr) {
            continue;
        }
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

} // namespace sp::ui
