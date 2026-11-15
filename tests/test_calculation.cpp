/// @file test_calculation.cpp
/// @brief Tests for the calculation engine; port of the Python tests.
#include "calculation.h"
#include "errors.h"
#include "models.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

namespace {

/// @brief Builds a mock configuration with 20 (or @p count) control constants.
sp::AppConfig makeMockConfig(int count = 20, double deltaStn = 2.0, double wN = 1000.0,
                             const QVector<double>& frequencies = {}) {
    QVector<double> fI;
    if (frequencies.isEmpty()) {
        for (int index = 1; index <= count; ++index) {
            fI.append(static_cast<double>(index));
        }
    } else {
        fI = frequencies;
    }
    sp::AppConfig config;
    config.frequencyConstants.numberOfConstants = count;
    config.frequencyConstants.fI = fI;
    config.frequencyConstants.deltaFI = QVector<double>(fI.size(), 100.0);
    config.frequencyConstants.kI = QVector<double>(fI.size(), 0.05);
    config.frequencyConstants.deltaAI = QVector<double>(fI.size(), 1.5);
    config.normParams.deltaStn = {deltaStn, deltaStn * 2.0, deltaStn * 3.0};
    config.normParams.wN = {wN, wN * 2.0, wN * 3.0};
    config.normNoiseByLine.symmetrical = QVector<double>(fI.size(), 1.0);
    config.normNoiseByLine.asymmetrical = QVector<double>(fI.size(), 1.0);
    config.normNoiseByLine.power = QVector<double>(fI.size(), 1.0);
    config.lines.symmetrical = {QStringLiteral("Line {}")};
    config.lines.asymmetrical = {QStringLiteral("Antenna")};
    config.lines.power = {QStringLiteral("Power ({})")};
    config.operationModes = {QStringLiteral("XX"), QStringLiteral("DR"), QStringLiteral("RR")};
    return config;
}

/// @brief Writes the mock spectrum files and returns their paths.
std::pair<QString, QString> writeSpectrumFiles(const QTemporaryDir& dir, int count = 20,
                                               double signalNoise = 5.0, double noise = 3.0) {
    const QString signalNoisePath = dir.filePath(QStringLiteral("signal_noise.txt"));
    const QString noisePath = dir.filePath(QStringLiteral("noise.txt"));
    QStringList signalNoiseLines;
    QStringList noiseLines;
    for (int index = 1; index <= count; ++index) {
        signalNoiseLines.append(QStringLiteral("%1.0\t%2").arg(index).arg(signalNoise, 0, 'f', 4));
        noiseLines.append(QStringLiteral("%1.0\t%2").arg(index).arg(noise, 0, 'f', 4));
    }
    for (const auto& [path, lines] :
         {std::pair{&signalNoisePath, &signalNoiseLines}, std::pair{&noisePath, &noiseLines}}) {
        QFile file(*path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qFatal("cannot write %s", qPrintable(*path));
        }
        file.write(lines->join(u'\n').toUtf8());
    }
    return {signalNoisePath, noisePath};
}

} // namespace

class TestCalculation : public QObject {
    Q_OBJECT

private slots:
    void voltageWithoutViolations();
    void voltageWithViolations();
    void currentMeasurementType();
    void clampsNegativeSignalPower();
    void validatesResistance();
    void validatesCategoryIndex();
    void validatesConstantsCountMismatch();
    void intermediateUAllLineTypes();
    void calculateWIdentity();
    void intermediateUZeroDivisionGuard();
    void rejectsNegativeFrequencyConstant();
    void rejectsExcessiveResistanceLimit();
};

void TestCalculation::voltageWithoutViolations() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir, 20, 2.0, 1.9);
    const sp::AppConfig config = makeMockConfig(20, 10.0);

    const sp::CalculationResult result =
        calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                  sp::MeasurementKind::Voltage, 50.0);

    QVERIFY(!result.hasViolations);
    QCOMPARE(result.measurementType, sp::MeasurementKind::Voltage);
    QVERIFY(!result.w.has_value());
    QVERIFY(!result.wN.has_value());
    QVERIFY(!result.isWViolation.has_value());
    QCOMPARE(result.points.size(), 20);
    QVERIFY(!result.points.at(0).rI.has_value());
}

void TestCalculation::voltageWithViolations() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir, 20, 5.0, 1.0);
    const sp::AppConfig config = makeMockConfig(20, 1.0, 10.0);

    const sp::CalculationResult result =
        calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                  sp::MeasurementKind::Voltage, 50.0);

    QVERIFY(result.hasViolations);
    QVERIFY(result.w.has_value());
    QVERIFY(result.wN.has_value());
    QVERIFY(result.isWViolation.has_value());
    QVERIFY(result.points.at(0).rI.has_value());
}

void TestCalculation::currentMeasurementType() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir, 20, 5.0, 1.0);
    const sp::AppConfig config = makeMockConfig(20, 1000.0);

    const sp::CalculationResult result =
        calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Symmetrical,
                  sp::MeasurementKind::Current, 50.0);

    QCOMPARE(result.measurementType, sp::MeasurementKind::Current);
    QCOMPARE(result.points.at(0).q, 244.949);
}

void TestCalculation::clampsNegativeSignalPower() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir, 20, 1.0, 2.0);
    const sp::AppConfig config = makeMockConfig();

    const sp::CalculationResult result =
        calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                  sp::MeasurementKind::Voltage, 50.0);

    QCOMPARE(result.points.at(0).uS, 0.0);
    QCOMPARE(result.points.at(0).q, 0.0);
    QVERIFY(!result.points.at(0).isViolation);
}

void TestCalculation::validatesResistance() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir);
    const sp::AppConfig config = makeMockConfig();

    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, std::nullopt),
                             CalculationError);
    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, 0.0),
                             CalculationError);
    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, -10.0),
                             CalculationError);
}

void TestCalculation::validatesCategoryIndex() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir);
    const sp::AppConfig config = makeMockConfig();

    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, -1, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, 50.0),
                             CalculationError);
    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 10, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, 50.0),
                             CalculationError);
}

void TestCalculation::validatesConstantsCountMismatch() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir);
    QVector<double> frequencies;
    for (int index = 1; index <= 19; ++index) {
        frequencies.append(static_cast<double>(index));
    }
    const sp::AppConfig config = makeMockConfig(20, 2.0, 1000.0, frequencies);

    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, 50.0),
                             CalculationError);
}

void TestCalculation::intermediateUAllLineTypes() {
    for (sp::LineType lineType :
         {sp::LineType::Symmetrical, sp::LineType::Asymmetrical, sp::LineType::Power}) {
        QCOMPARE(calculateIntermediateU(50.0, 100.0, 1.25, lineType), 1.25);
    }
}

void TestCalculation::calculateWIdentity() {
    QCOMPARE(calculateW(99.45), 99.45);
}

void TestCalculation::intermediateUZeroDivisionGuard() {
    // Deliberately passing zero pointIndex: the guard is about the error, not the index.
    QVERIFY_EXCEPTION_THROWN(calculateQ(5.0, 0.0, sp::MeasurementKind::Voltage, 50.0, 0),
                             CalculationError);
}

void TestCalculation::rejectsNegativeFrequencyConstant() {
    QTemporaryDir dir;
    const QString signalNoisePath = dir.filePath(QStringLiteral("signal_noise.txt"));
    const QString noisePath = dir.filePath(QStringLiteral("noise.txt"));
    QStringList signalNoiseLines{QStringLiteral("-100.0\t5.0")};
    QStringList noiseLines{QStringLiteral("-100.0\t1.0")};
    for (int index = 2; index <= 20; ++index) {
        signalNoiseLines.append(QStringLiteral("%1.0\t5.0").arg(index));
        noiseLines.append(QStringLiteral("%1.0\t1.0").arg(index));
    }
    for (const auto& [path, lines] :
         {std::pair{&signalNoisePath, &signalNoiseLines}, std::pair{&noisePath, &noiseLines}}) {
        QFile file(*path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qFatal("cannot write %s", qPrintable(*path));
        }
        file.write(lines->join(u'\n').toUtf8());
    }

    QVector<double> frequencies{-100.0};
    for (int index = 2; index <= 20; ++index) {
        frequencies.append(static_cast<double>(index));
    }
    const sp::AppConfig config = makeMockConfig(20, 100.0, 1000.0, frequencies);

    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, 50.0),
                             CalculationError);
}

void TestCalculation::rejectsExcessiveResistanceLimit() {
    QTemporaryDir dir;
    const auto [signalNoisePath, noisePath] = writeSpectrumFiles(dir);
    const sp::AppConfig config = makeMockConfig();

    QVERIFY_EXCEPTION_THROWN(calculate(signalNoisePath, noisePath, config, 0, sp::LineType::Power,
                                       sp::MeasurementKind::Voltage, 1e12),
                             CalculationError);
}

QTEST_APPLESS_MAIN(TestCalculation)
#include "test_calculation.moc"
