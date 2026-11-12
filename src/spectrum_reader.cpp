/// @file spectrum_reader.cpp
/// @brief Reading of control frequencies from text spectrum files.
///
/// Implementation of the spectrum reader declared in spectrum_reader.h.
#include "spectrum_reader.h"

#include "errors.h"
#include "i18n.h"

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>
#include <cmath>

using Qt::Literals::StringLiterals::operator""_s;

namespace {

/// @brief Formats a frequency without trailing zeros for messages (Hz).
QString formatFrequency(double frequency)
{
    QString text = QString::number(frequency, 'f', 4);
    while (text.endsWith(u'0')) {
        text.chop(1);
    }
    if (text.endsWith(u'.')) {
        text.chop(1);
    }
    return text;
}

} // namespace

QVector<double> readRequiredFrequencies(const QString &filePath,
    const QVector<double> &frequencies)
{
    if (frequencies.isEmpty()) {
        throw CalculationError(tr_ns::tr("err.spectrumEmptyFrequencies"));
    }
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        throw SpectrumNotFoundError(filePath);
    }

    // Rounded requested frequencies, Hz (used as hash keys).
    QHash<int, QVector<int>> requested;
    requested.reserve(frequencies.size());
    for (int index = 0; index < frequencies.size(); ++index) {
        requested[static_cast<int>(std::lround(frequencies[index]))].append(index);
    }

    QHash<int, double> found;
    found.reserve(requested.size());

    const QString fileName = info.fileName();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw SpectrumFormatError(
            tr_ns::tr("err.spectrumReadError", fileName, file.errorString()));
    }
    // The spectrum files are ASCII-compatible; fromUtf8 replaces any invalid
    // bytes lossily, like errors="replace" in the reference implementation.
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream.setAutoDetectUnicode(false);

    const QRegularExpression whitespace(QStringLiteral("\\s+"));
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        QString stripped = line.trimmed();
        if (stripped.isEmpty()) {
            continue;
        }
        const QChar first = stripped.at(0);
        if (first == u'#' || first == u'[' || first == u'(') {
            continue;
        }
        stripped.replace(u',', u'.');
        const QStringList columns = stripped.split(whitespace, Qt::SkipEmptyParts);
        if (columns.size() < 2) {
            continue;
        }
        bool frequencyOk = false;
        bool voltageOk = false;
        const double frequencyValue = columns.at(0).toDouble(&frequencyOk);
        const double voltageValue = columns.at(1).toDouble(&voltageOk);
        if (!frequencyOk || !voltageOk) {
            continue;
        }
        if (!qIsFinite(frequencyValue) || !qIsFinite(voltageValue)) {
            continue;
        }
        if (frequencyValue < 0.0 || voltageValue < 0.0) {
            throw SpectrumFormatError(tr_ns::tr("err.spectrumNegativeValue", fileName));
        }
        const int frequency = static_cast<int>(std::lround(frequencyValue));
        if (!requested.contains(frequency)) {
            continue; // Not a control frequency: nothing to record.
        }
        const auto foundIt = found.constFind(frequency);
        if (foundIt != found.constEnd() && !qFuzzyIsNull(foundIt.value() - voltageValue)) {
            throw SpectrumFormatError(tr_ns::tr("err.spectrumDuplicate", fileName,
                formatFrequency(frequency)));
        }
        found.insert(frequency, voltageValue);
    }

    QStringList missing;
    for (double frequency : frequencies) {
        const int rounded = static_cast<int>(std::lround(frequency));
        if (!found.contains(rounded)) {
            missing.append(formatFrequency(frequency));
        }
    }
    if (!missing.isEmpty()) {
        throw SpectrumMissingFrequencyError(fileName, missing.join(u", "_s));
    }

    QVector<double> result;
    result.reserve(frequencies.size());
    for (double frequency : frequencies) {
        result.append(found.value(static_cast<int>(std::lround(frequency))));
    }
    return result;
}
