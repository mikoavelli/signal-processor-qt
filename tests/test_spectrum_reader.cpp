/// @file test_spectrum_reader.cpp
/// @brief Tests for the spectrum reader; port of the Python tests.
#include "errors.h"
#include "spectrum_reader.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class TestSpectrumReader : public QObject {
    Q_OBJECT

private slots:
    void validDataset();
    void commaSeparator();
    void commentsAndBlankLines();
    void malformedLines();
    void fileNotFound();
    void missingFrequencies();
    void emptyRequest();
    void negativeFrequencyRecord();
    void negativeVoltageRecord();
    void conflictingDuplicateRecords();

private:
    static QString writeFile(const QTemporaryDir &dir, const QString &name, const QString &content)
    {
        const QString path = dir.filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qFatal("cannot write %s", qPrintable(path));
        }
        file.write(content.toUtf8());
        return path;
    }
};

void TestSpectrumReader::validDataset()
{
    QTemporaryDir dir;
    QStringList lines;
    for (int frequency = 1; frequency <= 1000; ++frequency) {
        lines.append(QStringLiteral("%1.0\t%2").arg(frequency).arg(frequency * 0.5, 0, 'f', 2));
    }
    const QString path = writeFile(dir, QStringLiteral("spectrum.txt"), lines.join(u'\n'));
    const QVector<double> frequencies = {10.0, 50.0, 100.0};
    const QVector<double> results = readRequiredFrequencies(path, frequencies);
    QCOMPARE(results.size(), 3);
    QCOMPARE(results.at(0), 5.0);
    QCOMPARE(results.at(1), 25.0);
    QCOMPARE(results.at(2), 50.0);
}

void TestSpectrumReader::commaSeparator()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("spectrum_comma.txt"),
        QStringLiteral("100,0\t12,50\n200,0\t25,00\n"));
    const QVector<double> results = readRequiredFrequencies(path, {100.0, 200.0});
    QCOMPARE(results.at(0), 12.5);
    QCOMPARE(results.at(1), 25.0);
}

void TestSpectrumReader::commentsAndBlankLines()
{
    QTemporaryDir dir;
    const QString content = QStringLiteral(
        "# Header comment\n"
        "[Section header]\n"
        "(Auxiliary note)\n"
        "\n"
        "100.0\t1.5\n"
        "   \n"
        "200.0\t3.0\n");
    const QString path = writeFile(dir, QStringLiteral("spectrum_comments.txt"), content);
    const QVector<double> results = readRequiredFrequencies(path, {100.0, 200.0});
    QCOMPARE(results.at(0), 1.5);
    QCOMPARE(results.at(1), 3.0);
}

void TestSpectrumReader::malformedLines()
{
    QTemporaryDir dir;
    const QString content = QStringLiteral(
        "SINGLE_COLUMN\n100.0\tCORRUPT\n100.0\t4.5\n");
    const QString path = writeFile(dir, QStringLiteral("spectrum_corrupt_lines.txt"), content);
    const QVector<double> results = readRequiredFrequencies(path, {100.0});
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.at(0), 4.5);
}

void TestSpectrumReader::fileNotFound()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("missing.txt"));
    QVERIFY_EXCEPTION_THROWN(readRequiredFrequencies(path, {100.0}), SpectrumNotFoundError);
}

void TestSpectrumReader::missingFrequencies()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("incomplete.txt"),
        QStringLiteral("100.0\t1.0\n"));
    try {
        Q_UNUSED(readRequiredFrequencies(path, {100.0, 200.0}));
        QFAIL("expected SpectrumMissingFrequencyError");
    } catch (const SpectrumMissingFrequencyError &error) {
        QCOMPARE(error.filename(), QStringLiteral("incomplete.txt"));
        QVERIFY(error.missingText().contains(QStringLiteral("200")));
    }
}

void TestSpectrumReader::emptyRequest()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("valid.txt"), QStringLiteral("100.0\t1.0\n"));
    QVERIFY_EXCEPTION_THROWN(readRequiredFrequencies(path, {}), CalculationError);
}

void TestSpectrumReader::negativeFrequencyRecord()
{
    // Spectrum files with negative frequency records must be rejected.
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("negative_f.txt"),
        QStringLiteral("-500.0\t1.5\n"));
    QVERIFY_EXCEPTION_THROWN(readRequiredFrequencies(path, {-500.0}), SpectrumFormatError);
}

void TestSpectrumReader::negativeVoltageRecord()
{
    // Spectrum files with negative amplitudes must be rejected.
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("negative_v.txt"),
        QStringLiteral("500.0\t-2.5\n"));
    QVERIFY_EXCEPTION_THROWN(readRequiredFrequencies(path, {500.0}), SpectrumFormatError);
}

void TestSpectrumReader::conflictingDuplicateRecords()
{
    // Duplicate records for the same frequency with different values must be rejected.
    QTemporaryDir dir;
    const QString path = writeFile(dir, QStringLiteral("duplicates.txt"),
        QStringLiteral("500.0\t1.0\n500.0\t9.0\n"));
    QVERIFY_EXCEPTION_THROWN(readRequiredFrequencies(path, {500.0}), SpectrumFormatError);
}

QTEST_APPLESS_MAIN(TestSpectrumReader)
#include "test_spectrum_reader.moc"
