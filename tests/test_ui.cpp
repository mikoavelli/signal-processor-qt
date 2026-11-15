/// @file test_ui.cpp
/// @brief Tests for UI helpers and widgets; port of the Python UI tests.
#include "models.h"
#include "ui/double_validator.h"
#include "ui/file_drop_line_edit.h"
#include "ui/line_template_widget.h"
#include "ui/table_model.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

class TestUi : public QObject {
    Q_OBJECT

private slots:
    void extractLocalFileSuccess();
    void extractLocalFileRejectsDirectory();
    void extractLocalFileRejectsMultipleUrls();
    void doubleValidatorLocaleCommaAndDot();
    void doubleValidatorRejectsOutOfRangeMaximum();
    void tableModelRowAndColumnCounts();
    void lineTemplateWidgetLifecycle();
    void lineTemplateWidgetRejectsUnbalancedBraces();
};

void TestUi::extractLocalFileSuccess() {
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("file.txt"));
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("data");
    }
    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(path)});
    const std::optional<QString> extracted = sp::ui::extractLocalFile(&mimeData);
    QVERIFY(extracted.has_value());
    QCOMPARE(extracted.value(), QFileInfo(path).absoluteFilePath());
}

void TestUi::extractLocalFileRejectsDirectory() {
    QTemporaryDir dir;
    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(dir.path())});
    QVERIFY(!sp::ui::extractLocalFile(&mimeData).has_value());
}

void TestUi::extractLocalFileRejectsMultipleUrls() {
    QTemporaryDir dir;
    const QString first = dir.filePath(QStringLiteral("1.txt"));
    const QString second = dir.filePath(QStringLiteral("2.txt"));
    for (const QString& path : {first, second}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("a");
    }
    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(first), QUrl::fromLocalFile(second)});
    QVERIFY(!sp::ui::extractLocalFile(&mimeData).has_value());
}

void TestUi::doubleValidatorLocaleCommaAndDot() {
    sp::ui::DoubleValidator validator(0.0, 1000.0, 4);
    int pos = 0;
    QString dot = QStringLiteral("50.25");
    QCOMPARE(validator.validate(dot, pos), QValidator::Acceptable);
    QString comma = QStringLiteral("50,25");
    QCOMPARE(validator.validate(comma, pos), QValidator::Acceptable);
}

void TestUi::doubleValidatorRejectsOutOfRangeMaximum() {
    sp::ui::DoubleValidator validator(0.0, 100.0, 2);
    int pos = 0;
    QString input = QStringLiteral("9999.99");
    QCOMPARE(validator.validate(input, pos), QValidator::Invalid);
}

void TestUi::tableModelRowAndColumnCounts() {
    sp::ui::MeasurementTableModel model;
    QCOMPARE(model.columnCount(), 0);
    QCOMPARE(model.rowCount(), 0);

    sp::MeasurementPoint point;
    point.index = 1;
    point.deltaF = 500.0;
    point.f = 500.0;
    point.uSn = 2.0;
    point.uN = 1.0;
    point.uS = 1.7321;
    point.q = 1.7321;
    point.isViolation = true;
    point.rI = std::nullopt;

    model.updateData({point}, sp::MeasurementKind::Voltage);
    QCOMPARE(model.columnCount(), 7);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.headerData(3, Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("U_сш"));

    const QColor color = model.data(model.index(0, 6), Qt::BackgroundRole).value<QColor>();
    QCOMPARE(color, QColor(QStringLiteral("#FEE2E2")));

    model.clear();
    QCOMPARE(model.columnCount(), 0);
    QCOMPARE(model.rowCount(), 0);
}

void TestUi::lineTemplateWidgetLifecycle() {
    sp::ui::LineTemplateWidget widget;
    widget.setTemplate(QStringLiteral("ЛВС {} - {}"));
    QVERIFY(!widget.isValid());

    Q_UNUSED(widget.fullName())
    widget.clearInputs();
}

void TestUi::lineTemplateWidgetRejectsUnbalancedBraces() {
    // Templates with unbalanced braces must be rejected.
    sp::ui::LineTemplateWidget widget;
    widget.setTemplate(QStringLiteral("Line {param"));
    QVERIFY(!widget.isValid());
    QVERIFY(widget.hasTemplateError());
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    TestUi test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_ui.moc"
