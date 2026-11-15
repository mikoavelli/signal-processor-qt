/// @file main_window.h
/// @brief Main window: inputs, calculation and saving.
#pragma once

#include "models.h"

#include <QMainWindow>
#include <optional>

class QComboBox;
class QFrame;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QTableView;
class QCloseEvent;

namespace sp::ui {

class FileDropLineEdit;
class LineTemplateWidget;
class MeasurementTableModel;

/// @brief Main window of the application.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /// @brief Creates the main window.
    /// @param config Validated application configuration.
    /// @param deviceName Name of the device under test.
    /// @param categoryIndex 0-based index of the object category.
    /// @param baseDirectory Directory for the measurements database.
    /// @param parent Parent widget.
    explicit MainWindow(const sp::AppConfig& config, const QString& deviceName, int categoryIndex,
                        const QString& baseDirectory, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void selectSignalNoiseFile();
    void selectNoiseFile();
    void onLineChanged(int index);
    void runCalculation();
    void saveToDatabase();

private:
    void updateWindowTitle();
    QGroupBox* createInputFilesCard();
    QGroupBox* createLineControlCard();
    QHBoxLayout* createBottomBar();
    void loadLineTemplates();
    void loadOperationModes();
    void setSignalNoiseFile(const QString& path);
    void setNoiseFile(const QString& path);
    [[nodiscard]] sp::LineType currentLineType() const;
    [[nodiscard]] sp::MeasurementKind currentMeasurementType() const;
    [[nodiscard]] std::optional<double> readResistance() const;
    void updateStatus(const sp::CalculationResult& result);

    sp::AppConfig m_config;  ///< Validated configuration.
    QString m_deviceName;    ///< Device under test.
    int m_categoryIndex = 0; ///< 0-based object category index.
    int m_lineNumber = 1;    ///< 1-based line number within the series.
    QString m_databasePath;  ///< Path of the measurements database file.

    std::optional<QString> m_signalNoisePath;                 ///< Chosen mixture spectrum file.
    std::optional<QString> m_noisePath;                       ///< Chosen own-noise spectrum file.
    std::optional<sp::CalculationResult> m_calculationResult; ///< Result awaiting save.
    QVector<std::pair<sp::LineType, QString>> m_lineItems;    ///< Flattened line templates.

    FileDropLineEdit* m_signalNoiseEdit = nullptr; ///< Drop target for the mixture file.
    FileDropLineEdit* m_noiseEdit = nullptr;       ///< Drop target for the noise file.
    QRadioButton* m_currentRadio = nullptr;        ///< “Current” radio.
    QRadioButton* m_voltageRadio = nullptr;        ///< “Voltage” radio.
    QLineEdit* m_resistanceEdit = nullptr;         ///< Resistance R edit.
    QComboBox* m_lineCombo = nullptr;              ///< Line-template selector.
    LineTemplateWidget* m_lineTemplate = nullptr;  ///< Parameters of the line template.
    QComboBox* m_operationMode = nullptr;          ///< Operation mode selector.
    QPushButton* m_calculateButton = nullptr;      ///< Calculate button.
    QTableView* m_tableView = nullptr;             ///< Result table.
    MeasurementTableModel* m_tableModel = nullptr; ///< Model of the result table.
    QFrame* m_statusCard = nullptr;                ///< Card carrying the status label.
    QLabel* m_statusLabel = nullptr;               ///< Status text.
    QPushButton* m_saveButton = nullptr;           ///< Save button.
};

} // namespace sp::ui
