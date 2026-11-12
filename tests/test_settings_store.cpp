/// @file test_settings_store.cpp
/// @brief Tests for the settings.json loader; port of the Python config-loader tests.
#include "errors.h"
#include "settings_store.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

#ifndef SIGNAL_PROCESSOR_SOURCE_DIR
#error "SIGNAL_PROCESSOR_SOURCE_DIR must be defined by the build system"
#endif

namespace {

/// @brief Builds a valid settings document; tests mutate it before serializing.
QJsonObject validDocument(int count = 1)
{
    QJsonArray frequencies;
    QJsonArray deltaFI;
    QJsonArray kI;
    QJsonArray deltaAI;
    QJsonArray noiseSym;
    QJsonArray noiseAsym;
    QJsonArray noisePower;
    for (int index = 0; index < count; ++index) {
        frequencies.append(500.0 + index * 100.0);
        deltaFI.append(100.0);
        kI.append(0.01);
        deltaAI.append(1.5);
        noiseSym.append(1.0);
        noiseAsym.append(1.0);
        noisePower.append(1.0);
    }
    QJsonObject frequencyConstants;
    frequencyConstants.insert(QStringLiteral("number_of_constants"), count);
    frequencyConstants.insert(QStringLiteral("f_i"), frequencies);
    frequencyConstants.insert(QStringLiteral("delta_f_i"), deltaFI);
    frequencyConstants.insert(QStringLiteral("k_i"), kI);
    frequencyConstants.insert(QStringLiteral("delta_a_i"), deltaAI);

    QJsonObject normParams;
    normParams.insert(QStringLiteral("delta_stn"), QJsonArray{2.0, 4.0, 6.0});
    normParams.insert(QStringLiteral("w_n"), QJsonArray{1000.0, 2000.0, 3000.0});

    QJsonObject normNoise;
    normNoise.insert(QStringLiteral("symmetrical"), noiseSym);
    normNoise.insert(QStringLiteral("asymmetrical"), noiseAsym);
    normNoise.insert(QStringLiteral("power"), noisePower);

    QJsonObject line;
    line.insert(QStringLiteral("symmetrical"), QJsonArray{QStringLiteral("Line {}")});
    line.insert(QStringLiteral("asymmetrical"), QJsonArray{QStringLiteral("Antenna")});
    line.insert(QStringLiteral("power"), QJsonArray{QStringLiteral("Power ({})")});

    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    root.insert(QStringLiteral("frequency_constants"), frequencyConstants);
    root.insert(QStringLiteral("norm_params_by_category"), normParams);
    root.insert(QStringLiteral("norm_noise_by_line"), normNoise);
    root.insert(QStringLiteral("line"), line);
    root.insert(QStringLiteral("operation_modes"),
        QJsonArray{QStringLiteral("XX"), QStringLiteral("DR"), QStringLiteral("RR")});
    return root;
}

/// @brief Serializes @p document into a fresh temporary settings.json.
QString writeConfig(const QTemporaryDir &dir, const QJsonObject &document)
{
    const QString path = dir.filePath(QStringLiteral("settings.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qFatal("cannot write %s", qPrintable(path));
    }
    file.write(QJsonDocument(document).toJson());
    return path;
}

/// @brief Writes raw bytes into a fresh temporary settings.json.
QString writeRawConfig(const QTemporaryDir &dir, const QByteArray &content)
{
    const QString path = dir.filePath(QStringLiteral("settings.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qFatal("cannot write %s", qPrintable(path));
    }
    file.write(content);
    return path;
}

} // namespace

class TestSettingsStore : public QObject {
    Q_OBJECT

private slots:
    void loadValidConfig();
    void missingFileThrows();
    void syntaxErrorThrows();
    void zeroNumberOfConstantsThrows();
    void constantsLengthMismatchThrows();
    void nonNumericFloatListThrows();
    void lineNoiseLengthMismatchThrows();
    void projectRootSettingsIsValid();
    void negativeFrequenciesThrow();
    void negativeNormNoiseThrows();
    void emptyOperationModesThrow();
};

void TestSettingsStore::loadValidConfig()
{
    QTemporaryDir dir;
    const QString path = writeConfig(dir, validDocument());
    const sp::AppConfig config = sp::settings_store::loadConfig(path);
    QCOMPARE(config.frequencyConstants.numberOfConstants, 1);
}

void TestSettingsStore::missingFileThrows()
{
    QTemporaryDir dir;
    QVERIFY_EXCEPTION_THROWN(
        sp::settings_store::loadConfig(dir.filePath(QStringLiteral("absent.json"))),
        ConfigNotFoundError);
}

void TestSettingsStore::syntaxErrorThrows()
{
    QTemporaryDir dir;
    const QString path = writeRawConfig(dir, QByteArray("broken = [unclosed"));
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::zeroNumberOfConstantsThrows()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument();
    QJsonObject frequency = document.value(QStringLiteral("frequency_constants")).toObject();
    frequency.insert(QStringLiteral("number_of_constants"), 0);
    document.insert(QStringLiteral("frequency_constants"), frequency);
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::constantsLengthMismatchThrows()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument(2);
    QJsonObject frequency = document.value(QStringLiteral("frequency_constants")).toObject();
    frequency.insert(QStringLiteral("f_i"), QJsonArray{500.0});
    document.insert(QStringLiteral("frequency_constants"), frequency);
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::nonNumericFloatListThrows()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument();
    QJsonObject frequency = document.value(QStringLiteral("frequency_constants")).toObject();
    frequency.insert(QStringLiteral("f_i"), QJsonArray{QStringLiteral("text")});
    document.insert(QStringLiteral("frequency_constants"), frequency);
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::lineNoiseLengthMismatchThrows()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument(1);
    QJsonObject noise = document.value(QStringLiteral("norm_noise_by_line")).toObject();
    noise.insert(QStringLiteral("symmetrical"), QJsonArray{1.0, 2.0});
    document.insert(QStringLiteral("norm_noise_by_line"), noise);
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::projectRootSettingsIsValid()
{
    const QString settingsPath =
        QString::fromUtf8(SIGNAL_PROCESSOR_SOURCE_DIR) + QStringLiteral("/settings.json");
    const sp::AppConfig config = sp::settings_store::loadConfig(settingsPath);
    const int count = config.frequencyConstants.numberOfConstants;
    QCOMPARE(count, 20);
    QCOMPARE(config.frequencyConstants.fI.size(), count);
}

void TestSettingsStore::negativeFrequenciesThrow()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument();
    QJsonObject frequency = document.value(QStringLiteral("frequency_constants")).toObject();
    frequency.insert(QStringLiteral("f_i"), QJsonArray{-500.0});
    document.insert(QStringLiteral("frequency_constants"), frequency);
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::negativeNormNoiseThrows()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument();
    QJsonObject noise = document.value(QStringLiteral("norm_noise_by_line")).toObject();
    noise.insert(QStringLiteral("symmetrical"), QJsonArray{-1.5});
    document.insert(QStringLiteral("norm_noise_by_line"), noise);
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

void TestSettingsStore::emptyOperationModesThrow()
{
    QTemporaryDir dir;
    QJsonObject document = validDocument();
    document.insert(QStringLiteral("operation_modes"), QJsonArray{});
    const QString path = writeConfig(dir, document);
    QVERIFY_EXCEPTION_THROWN(sp::settings_store::loadConfig(path), ConfigValidationError);
}

QTEST_APPLESS_MAIN(TestSettingsStore)
#include "test_settings_store.moc"
