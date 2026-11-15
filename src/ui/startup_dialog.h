/// @file startup_dialog.h
/// @brief First dialog of the program: device name and object category.
#pragma once

#include <QDialog>

#include <utility>

class QComboBox;
class QLineEdit;

namespace sp::ui {

/// @brief Collects the device under test and the category of the new series.
class StartupDialog : public QDialog {
    Q_OBJECT

public:
    explicit StartupDialog(QWidget* parent = nullptr);

    /// @brief Returns the validated inputs (device name, 0-based category index).
    [[nodiscard]] std::pair<QString, int> parameters() const;

private slots:
    void onStart();

private:
    QLineEdit* m_deviceName = nullptr; ///< Device name edit.
    QComboBox* m_category = nullptr;   ///< Category combo.
};

} // namespace sp::ui
