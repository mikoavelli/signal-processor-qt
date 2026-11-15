/// @file main_window.cpp
/// @brief Implementation of the main window.
#include "main_window.h"

#include "calculation.h"
#include "database.h"
#include "errors.h"
#include "i18n.h"
#include "ui/double_validator.h"
#include "ui/file_drop_line_edit.h"
#include "ui/line_template_widget.h"
#include "ui/table_model.h"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QFileInfo>
#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTableView>
#include <QVBoxLayout>


namespace sp::ui {

MainWindow::MainWindow(const sp::AppConfig &config, const QString &deviceName,
    int categoryIndex, const QString &databasePath, QWidget *parent)
    : QMainWindow(parent)
    , m_config(config)
    , m_deviceName(deviceName)
    , m_categoryIndex(categoryIndex)
    , m_databasePath(databasePath)
    , m_signalNoiseEdit(new FileDropLineEdit(tr_ns::tr("window.placeholder.file"),
          [this](const QString &path) { setSignalNoiseFile(path); },
          this))
    , m_noiseEdit(new FileDropLineEdit(tr_ns::tr("window.placeholder.file"),
          [this](const QString &path) { setNoiseFile(path); },
          this))
    , m_currentRadio(new QRadioButton(tr_ns::tr("window.radio.current"), this))
    , m_voltageRadio(new QRadioButton(tr_ns::tr("window.radio.voltage"), this))
    , m_resistanceEdit(new QLineEdit(this))
    , m_lineCombo(new QComboBox(this))
    , m_lineTemplate(new LineTemplateWidget(this))
    , m_operationMode(new QComboBox(this))
    , m_calculateButton(new QPushButton(tr_ns::tr("window.button.calculate"), this))
    , m_tableView(new QTableView(this))
    , m_tableModel(new MeasurementTableModel(this))
    , m_statusCard(new QFrame(this))
    , m_statusLabel(new QLabel(tr_ns::tr("status.idle"), m_statusCard))
    , m_saveButton(new QPushButton(tr_ns::tr("window.button.save"), this))
{
    loadLineTemplates();
    // ------------------------------------------------------------------
    // Central widget layout
    // ------------------------------------------------------------------
    resize(1080, 780);
    setMinimumSize(980, 720);

    auto *mainWidget = new QWidget(this);
    auto *root = new QVBoxLayout(mainWidget);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto *cards = new QHBoxLayout();
    cards->setSpacing(10);
    cards->addWidget(createInputFilesCard(), 1);
    cards->addWidget(createLineControlCard(), 1);
    root->addLayout(cards);

    m_tableView->setModel(m_tableModel);
    m_tableView->setAlternatingRowColors(true);
    if (QHeaderView *header = m_tableView->horizontalHeader()) {
        header->setSectionResizeMode(QHeaderView::Stretch);
    }
    root->addWidget(m_tableView, 1);

    root->addLayout(createBottomBar());
    setCentralWidget(mainWidget);

    loadOperationModes();
    updateWindowTitle();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_calculationResult.has_value()) {
        QMessageBox dialog(this);
        dialog.setIcon(QMessageBox::Question);
        dialog.setWindowTitle(tr_ns::tr("dialog.close.title"));
        dialog.setText(tr_ns::tr("dialog.close.text"));
        QPushButton *yesButton = dialog.addButton(tr_ns::tr("dialog.close.yes"), QMessageBox::YesRole);
        QPushButton *noButton = dialog.addButton(tr_ns::tr("dialog.close.no"), QMessageBox::NoRole);
        dialog.setDefaultButton(noButton);
        dialog.exec();
        if (dialog.clickedButton() != yesButton) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr_ns::tr("window.title", m_deviceName, QString::number(m_lineNumber)));
}

QGroupBox *MainWindow::createInputFilesCard()
{
    auto *box = new QGroupBox(tr_ns::tr("window.group.inputs"), this);
    auto *layout = new QVBoxLayout(box);
    layout->addWidget(new QLabel(tr_ns::tr("window.label.signalNoise"), box));

    auto *signalRow = new QHBoxLayout();
    auto *signalButton = new QPushButton(tr_ns::tr("window.button.browse"), box);
    signalRow->addWidget(m_signalNoiseEdit);
    signalRow->addWidget(signalButton);
    layout->addLayout(signalRow);

    layout->addWidget(new QLabel(tr_ns::tr("window.label.noise"), box));
    auto *noiseRow = new QHBoxLayout();
    auto *noiseButton = new QPushButton(tr_ns::tr("window.button.browse"), box);
    noiseRow->addWidget(m_noiseEdit);
    noiseRow->addWidget(noiseButton);
    layout->addLayout(noiseRow);

    auto *paramsRow = new QHBoxLayout();
    paramsRow->addWidget(new QLabel(tr_ns::tr("window.label.amount"), box));
    m_voltageRadio->setChecked(true);
    auto *measurementGroup = new QButtonGroup(this);
    measurementGroup->addButton(m_currentRadio, 1);
    measurementGroup->addButton(m_voltageRadio, 2);
    paramsRow->addWidget(m_voltageRadio);
    paramsRow->addWidget(m_currentRadio);
    paramsRow->addSpacing(18);
    paramsRow->addWidget(new QLabel(tr_ns::tr("window.label.resistance"), box));
    m_resistanceEdit->setFixedWidth(80);
    m_resistanceEdit->setValidator(new DoubleValidator(0.0, 1e9, 4, m_resistanceEdit));
    paramsRow->addWidget(m_resistanceEdit);
    paramsRow->addWidget(new QLabel(tr_ns::tr("window.unit.ohm"), box));
    paramsRow->addStretch();
    layout->addLayout(paramsRow);

    connect(signalButton, &QPushButton::clicked, this, &MainWindow::selectSignalNoiseFile);
    connect(noiseButton, &QPushButton::clicked, this, &MainWindow::selectNoiseFile);
    return box;
}

QGroupBox *MainWindow::createLineControlCard()
{
    auto *box = new QGroupBox(tr_ns::tr("window.group.line"), this);
    auto *layout = new QVBoxLayout(box);
    layout->addWidget(new QLabel(tr_ns::tr("window.label.line"), box));

    auto *lineRow = new QHBoxLayout();
    for (const auto &item : m_lineItems) {
        m_lineCombo->addItem(item.second);
    }
    connect(m_lineCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onLineChanged);
    lineRow->addWidget(m_lineCombo);
    lineRow->addWidget(m_lineTemplate, 1);
    layout->addLayout(lineRow);

    auto *modeRow = new QHBoxLayout();
    modeRow->addWidget(new QLabel(tr_ns::tr("window.label.mode"), box));
    modeRow->addWidget(m_operationMode);
    modeRow->addStretch();
    layout->addLayout(modeRow);
    layout->addStretch();
    connect(m_calculateButton, &QPushButton::clicked, this, &MainWindow::runCalculation);
    layout->addWidget(m_calculateButton);

    if (!m_lineItems.isEmpty()) {
        m_lineTemplate->setTemplate(m_lineItems.constFirst().second);
    }
    return box;
}

QHBoxLayout *MainWindow::createBottomBar()
{
    auto *bottom = new QHBoxLayout();
    bottom->addWidget(m_statusCard, 1);
    auto *statusLayout = new QHBoxLayout(m_statusCard);
    statusLayout->addWidget(m_statusLabel);
    m_saveButton->setEnabled(false);
    bottom->addWidget(m_saveButton);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveToDatabase);
    return bottom;
}

void MainWindow::loadLineTemplates()
{
    for (const QString &name : m_config.lines.symmetrical) {
        m_lineItems.append({sp::LineType::Symmetrical, name});
    }
    for (const QString &name : m_config.lines.asymmetrical) {
        m_lineItems.append({sp::LineType::Asymmetrical, name});
    }
    for (const QString &name : m_config.lines.power) {
        m_lineItems.append({sp::LineType::Power, name});
    }
}

void MainWindow::loadOperationModes()
{
    for (const QString &mode : m_config.operationModes) {
        m_operationMode->addItem(mode);
    }
}

void MainWindow::selectSignalNoiseFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr_ns::tr("fileDialog.signal.title"),
        QString(), tr_ns::tr("fileDialog.filter"));
    if (!path.isEmpty()) {
        setSignalNoiseFile(path);
    }
}

void MainWindow::selectNoiseFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr_ns::tr("fileDialog.noise.title"),
        QString(), tr_ns::tr("fileDialog.filter"));
    if (!path.isEmpty()) {
        setNoiseFile(path);
    }
}

void MainWindow::setSignalNoiseFile(const QString &path)
{
    m_signalNoisePath = QFileInfo(path).absoluteFilePath();
    m_signalNoiseEdit->setFilePath(m_signalNoisePath.value());
}

void MainWindow::setNoiseFile(const QString &path)
{
    m_noisePath = QFileInfo(path).absoluteFilePath();
    m_noiseEdit->setFilePath(m_noisePath.value());
}

void MainWindow::onLineChanged(int index)
{
    if (index >= 0 && index < m_lineItems.size()) {
        m_lineTemplate->setTemplate(m_lineItems.at(index).second);
    }
}

sp::LineType MainWindow::currentLineType() const
{
    const int index = m_lineCombo->currentIndex();
    if (index >= 0 && index < m_lineItems.size()) {
        return m_lineItems.at(index).first;
    }
    return sp::LineType::Power;
}

sp::MeasurementKind MainWindow::currentMeasurementType() const
{
    if (m_currentRadio->isChecked()) {
        return sp::MeasurementKind::Current;
    }
    return sp::MeasurementKind::Voltage;
}

std::optional<double> MainWindow::readResistance() const
{
    const QString text = m_resistanceEdit->text().trimmed();
    if (text.isEmpty()) {
        return std::nullopt;
    }
    bool ok = false;
    const double value = QString(text).replace(u',', u'.').toDouble(&ok);
    if (!ok) {
        return std::nullopt;
    }
    return value;
}

void MainWindow::runCalculation()
{
    if (!m_signalNoisePath.has_value() || !m_noisePath.has_value()) {
        QMessageBox::warning(this, tr_ns::tr("message.warning.title"), tr_ns::tr("message.selectBothFiles"));
        return;
    }
    if (!m_lineTemplate->isValid()) {
        QMessageBox::warning(this, tr_ns::tr("message.warning.title"), tr_ns::tr("message.fillLineParams"));
        m_lineTemplate->focusFirstEmpty();
        return;
    }
    const std::optional<double> resistance = readResistance();
    if (!resistance.has_value() || resistance.value() <= 0.0) {
        QMessageBox::warning(this, tr_ns::tr("message.warning.title"), tr_ns::tr("message.resistancePositive"));
        m_resistanceEdit->setFocus();
        m_resistanceEdit->selectAll();
        return;
    }

    const sp::MeasurementKind measurementType = currentMeasurementType();
    try {
        const sp::CalculationResult result = calculate(m_signalNoisePath.value(),
            m_noisePath.value(), m_config, m_categoryIndex, currentLineType(),
            measurementType, resistance);
        m_calculationResult = result;
        m_tableModel->updateData(result.points, result.measurementType);
        if (QHeaderView *header = m_tableView->horizontalHeader()) {
            header->setSectionResizeMode(QHeaderView::Stretch);
        }
        updateStatus(result);
        m_saveButton->setEnabled(true);
    } catch (const SpectrumError &error) {
        QMessageBox::critical(this, tr_ns::tr("message.spectrumError.title"), error.message());
    } catch (const CalculationError &error) {
        QMessageBox::critical(this, tr_ns::tr("message.calcError.title"), error.message());
    } catch (const SignalProcessorError &error) {
        QMessageBox::critical(this, tr_ns::tr("message.dataError.title"), error.message());
    }
}

void MainWindow::updateStatus(const sp::CalculationResult &result)
{
    if (result.hasViolations) {
        if (result.w.has_value() && result.wN.has_value()) {
            m_statusLabel->setText(tr_ns::tr("status.violations",
                QString::number(result.w.value(), 'f', 4),
                QString::number(result.wN.value(), 'f', 4)));
        } else {
            m_statusLabel->setText(tr_ns::tr("status.violationsNoW"));
        }
    } else {
        m_statusLabel->setText(tr_ns::tr("status.ok"));
    }
}

void MainWindow::saveToDatabase()
{
    if (!m_calculationResult.has_value()) {
        return;
    }
    const std::optional<double> resistance = readResistance();
    if (!resistance.has_value() || resistance.value() <= 0.0) {
        QMessageBox::warning(this, tr_ns::tr("message.warning.title"),
            tr_ns::tr("message.resistancePositiveSave"));
        return;
    }

    const int index = m_lineCombo->currentIndex();
    const QString lineName = m_lineTemplate->fullName();
    const QString mode = m_operationMode->currentText();
    const sp::MeasurementKind measurementType = currentMeasurementType();

    try {
        saveMeasurement(m_databasePath, m_deviceName,
            m_categoryIndex + 1, m_lineNumber, lineName,
            (index >= 0) ? currentLineType() : sp::LineType::Power, mode,
            sp::measurementKindToString(measurementType), resistance.value(),
            m_calculationResult.value());
    } catch (const DatabaseError &error) {
        QMessageBox::critical(this, tr_ns::tr("message.dbError.title"), error.message());
        return;
    } catch (const SignalProcessorError &error) {
        QMessageBox::critical(this, tr_ns::tr("message.saveError.title"), error.message());
        return;
    }

    QMessageBox::information(this, tr_ns::tr("message.saved.title"), tr_ns::tr("message.saved.text"));
    ++m_lineNumber;
    updateWindowTitle();
    m_calculationResult.reset();
    m_tableModel->clear();
    m_saveButton->setEnabled(false);

    m_resistanceEdit->clear();
    m_lineTemplate->clearInputs();
}

} // namespace sp::ui
