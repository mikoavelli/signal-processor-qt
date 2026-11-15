/// @file startup_dialog.cpp
/// @brief First dialog of the program: device name and object category.
#include "startup_dialog.h"

#include "i18n.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace sp::ui {

StartupDialog::StartupDialog(QWidget* parent)
    : QDialog(parent), m_deviceName(new QLineEdit(tr_ns::tr("dialog.startup.defaultDevice"), this)),
      m_category(new QComboBox(this)) {
    setWindowTitle(tr_ns::tr("dialog.startup.title"));
    setFixedWidth(400);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    layout->addWidget(new QLabel(tr_ns::tr("dialog.startup.info"), this));

    auto* form = new QFormLayout();
    form->setSpacing(10);

    for (int category = 1; category <= 3; ++category) {
        m_category->addItem(tr_ns::tr("dialog.startup.categoryName", QString::number(category)));
    }

    form->addRow(new QLabel(tr_ns::tr("dialog.startup.device"), this), m_deviceName);
    form->addRow(new QLabel(tr_ns::tr("dialog.startup.category"), this), m_category);
    layout->addLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* cancel = new QPushButton(tr_ns::tr("dialog.startup.cancel"), this);
    auto* start = new QPushButton(tr_ns::tr("dialog.startup.start"), this);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(start);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(start, &QPushButton::clicked, this, &StartupDialog::onStart);
}

void StartupDialog::onStart() {
    if (m_deviceName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr_ns::tr("dialog.startup.warning.title"),
                             tr_ns::tr("dialog.startup.warning.emptyDevice"));
        return;
    }
    accept();
}

std::pair<QString, int> StartupDialog::parameters() const {
    return {m_deviceName->text().trimmed(), m_category->currentIndex()};
}

} // namespace sp::ui
