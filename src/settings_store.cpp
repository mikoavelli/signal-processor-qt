/// @file settings_store.cpp
/// @brief Reading, validation and decoding of the settings.json file.
///
/// Implementation of the settings persistence declared in settings_store.h.
#include "settings_store.h"

#include "errors.h"
#include "i18n.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QStringDecoder>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>
#include <cmath>
#include <string>

namespace sp::settings_store {


namespace {

using Qt::Literals::StringLiterals::operator""_s;

/// @brief Keys allowed in the root object of settings.json.
const QStringList rootKeys = {u"schema_version"_s, u"frequency_constants"_s,
    u"norm_params_by_category"_s, u"norm_noise_by_line"_s, u"line"_s, u"operation_modes"_s};
/// @brief Keys allowed in the “frequency_constants” object.
const QStringList frequencyKeys = {u"number_of_constants"_s, u"f_i"_s, u"delta_f_i"_s,
    u"k_i"_s, u"delta_a_i"_s};
/// @brief Keys allowed in the “norm_params_by_category” object.
const QStringList normParamsKeys = {u"delta_stn"_s, u"w_n"_s};
/// @brief Keys allowed in the “norm_noise_by_line” object.
const QStringList normNoiseKeys = {u"symmetrical"_s, u"asymmetrical"_s, u"power"_s};
/// @brief Keys allowed in the “line” object.
const QStringList lineKeys = {u"symmetrical"_s, u"asymmetrical"_s, u"power"_s};

/// @brief Checks that an object contains exactly the expected keys.
/// @param object Object to inspect.
/// @param allowed Allowed keys.
/// @param where Path of the object for error messages.
void checkKeys(const QJsonObject &object, const QStringList &allowed, const QString &where)
{
    QStringList missing;
    for (const QString &key : allowed) {
        if (!object.contains(key)) {
            missing.append(key);
        }
    }
    if (!missing.isEmpty()) {
        throw ConfigValidationError(tr_ns::tr("err.keysMissing", where, missing.join(u", "_s)));
    }
    QStringList unknown;
    for (const QString &key : object.keys()) {
        if (!allowed.contains(key)) {
            unknown.append(key);
        }
    }
    if (!unknown.isEmpty()) {
        throw ConfigValidationError(tr_ns::tr("err.keysUnknown", where, unknown.join(u", "_s)));
    }
}

/// @brief Parses a finite number, rejecting bools and non-finite values.
/// @param value Value to parse.
/// @param where Path of the value for error messages.
/// @return The parsed number.
double asFiniteNumber(const QJsonValue &value, const QString &where)
{
    if (!value.isDouble()) {
        throw ConfigValidationError(tr_ns::tr("err.numberExpected", where));
    }
    const double number = value.toDouble();
    if (!std::isfinite(number)) {
        throw ConfigValidationError(tr_ns::tr("err.finiteNumber", where));
    }
    return number;
}

/// @brief Parses a non-negative finite number.
/// @param value Value to parse.
/// @param where Path of the value for error messages.
/// @return The parsed number.
double asNonNegativeNumber(const QJsonValue &value, const QString &where)
{
    const double number = asFiniteNumber(value, where);
    if (number < 0.0) {
        throw ConfigValidationError(tr_ns::tr("err.numberNegative", where));
    }
    return number;
}

/// @brief Parses an array of numbers with a per-element check.
/// @param value Value to parse.
/// @param where Path of the value for error messages.
/// @param check Per-element validator (must throw on unacceptable values).
/// @return The parsed numbers.
QVector<double> asNumberArray(const QJsonValue &value, const QString &where,
    double (*check)(const QJsonValue &, const QString &))
{
    if (!value.isArray()) {
        throw ConfigValidationError(tr_ns::tr("err.listExpected", where));
    }
    const QJsonArray array = value.toArray();
    if (array.isEmpty()) {
        throw ConfigValidationError(tr_ns::tr("err.listEmpty", where));
    }
    QVector<double> result;
    result.reserve(array.size());
    for (int index = 0; index < array.size(); ++index) {
        result.append(check(array.at(index), where + u"[%1]"_s.arg(index)));
    }
    return result;
}

/// @brief Parses a non-negative number per element.
QVector<double> asNonNegativeNumberArray(const QJsonValue &value, const QString &where)
{
    return asNumberArray(value, where, asNonNegativeNumber);
}

/// @brief Parses a list of non-empty strings.
/// @param value Value to parse.
/// @param where Path of the value for error messages.
/// @return The parsed strings.
QStringList asStringList(const QJsonValue &value, const QString &where)
{
    if (!value.isArray()) {
        throw ConfigValidationError(tr_ns::tr("err.listExpected", where));
    }
    const QJsonArray array = value.toArray();
    if (array.isEmpty()) {
        throw ConfigValidationError(tr_ns::tr("err.listEmpty", where));
    }
    QStringList result;
    result.reserve(array.size());
    for (int index = 0; index < array.size(); ++index) {
        const QJsonValue item = array.at(index);
        if (!item.isString()) {
            throw ConfigValidationError(tr_ns::tr("err.stringExpected", where + u"[%1]"_s.arg(index)));
        }
        const QString text = item.toString();
        if (text.trimmed().isEmpty()) {
            throw ConfigValidationError(tr_ns::tr("err.stringNotEmpty", where + u"[%1]"_s.arg(index)));
        }
        result.append(text);
    }
    return result;
}

/// @brief Parses an object of the expected shape.
/// @param value Value to parse.
/// @param where Path of the object for error messages.
/// @return The parsed object.
QJsonObject asObject(const QJsonValue &value, const QString &where)
{
    if (!value.isObject()) {
        throw ConfigValidationError(tr_ns::tr("err.objectExpected", where));
    }
    return value.toObject();
}

/// @brief Requires a vector of a given length.
void requireLength(const QVector<double> &vector, int expected, const QString &where)
{
    if (vector.size() != expected) {
        throw ConfigValidationError(tr_ns::tr("err.listLength", where, QString::number(expected),
            QString::number(vector.size())));
    }
}

} // namespace

AppConfig loadConfig(const QString &settingsPath)
{
    const QFileInfo info(settingsPath);
    if (!info.exists() || !info.isFile()) {
        throw ConfigNotFoundError(settingsPath);
    }

    QFile file(settingsPath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw ConfigValidationError(tr_ns::tr("err.configReadError", file.errorString()));
    }
    const QByteArray raw = file.readAll();
    file.close();

    // JSON must be UTF-8; reject undecodable bytes instead of guessing.
    QStringDecoder decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    const QString decoded = decoder(raw);
    if (decoder.hasError()) {
        throw ConfigValidationError(tr_ns::tr("err.configEncoding"));
    }
    static_cast<void>(decoded);

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        throw ConfigValidationError(tr_ns::tr("err.configJsonError", parseError.errorString()));
    }
    const QJsonObject root = document.object();
    checkKeys(root, rootKeys, QStringLiteral("settings.json"));

    // --- schema_version -----------------------------------------------------
    const QJsonValue versionValue = root.value(u"schema_version"_s);
    if (!versionValue.isDouble()) {
        throw ConfigValidationError(tr_ns::tr("err.numberExpected", u"schema_version"_s));
    }
    const double version = versionValue.toDouble();
    if (!qFuzzyIsNull(version - std::trunc(version))) {
        throw ConfigValidationError(tr_ns::tr("err.intExpected", u"schema_version"_s));
    }
    if (static_cast<int>(version) != schemaVersion) {
        throw ConfigValidationError(tr_ns::tr("err.configVersion", QString::number(static_cast<int>(version)),
            QString::number(schemaVersion)));
    }

    // --- frequency_constants -------------------------------------------------
    const QJsonObject frequencyObject =
        asObject(root.value(u"frequency_constants"_s), u"frequency_constants"_s);
    checkKeys(frequencyObject, frequencyKeys, u"frequency_constants"_s);

    const QJsonValue countValue = frequencyObject.value(u"number_of_constants"_s);
    if (!countValue.isDouble()) {
        throw ConfigValidationError(tr_ns::tr("err.intExpected", u"frequency_constants.number_of_constants"_s));
    }
    const double countDouble = countValue.toDouble();
    if (!qFuzzyIsNull(countDouble - std::trunc(countDouble))) {
        throw ConfigValidationError(tr_ns::tr("err.intExpected",
            u"frequency_constants.number_of_constants"_s));
    }
    const int numberOfConstants = static_cast<int>(countDouble);
    if (numberOfConstants <= 0) {
        throw ConfigValidationError(tr_ns::tr("err.numberNotPositive",
            u"frequency_constants.number_of_constants"_s));
    }

    FrequencyConstantsConfig frequencyConstants;
    frequencyConstants.numberOfConstants = numberOfConstants;
    frequencyConstants.fI = asNonNegativeNumberArray(frequencyObject.value(u"f_i"_s),
        u"frequency_constants.f_i"_s);
    frequencyConstants.deltaFI = asNonNegativeNumberArray(frequencyObject.value(u"delta_f_i"_s),
        u"frequency_constants.delta_f_i"_s);
    frequencyConstants.kI = asNonNegativeNumberArray(frequencyObject.value(u"k_i"_s),
        u"frequency_constants.k_i"_s);
    frequencyConstants.deltaAI = asNonNegativeNumberArray(frequencyObject.value(u"delta_a_i"_s),
        u"frequency_constants.delta_a_i"_s);
    requireLength(frequencyConstants.fI, numberOfConstants, u"frequency_constants.f_i"_s);
    requireLength(frequencyConstants.deltaFI, numberOfConstants, u"frequency_constants.delta_f_i"_s);
    requireLength(frequencyConstants.kI, numberOfConstants, u"frequency_constants.k_i"_s);
    requireLength(frequencyConstants.deltaAI, numberOfConstants, u"frequency_constants.delta_a_i"_s);

    // --- norm_params_by_category --------------------------------------------
    const QJsonObject normParamsObject =
        asObject(root.value(u"norm_params_by_category"_s), u"norm_params_by_category"_s);
    checkKeys(normParamsObject, normParamsKeys, u"norm_params_by_category"_s);
    NormParamsConfig normParams;
    normParams.deltaStn = asNonNegativeNumberArray(normParamsObject.value(u"delta_stn"_s),
        u"norm_params_by_category.delta_stn"_s);
    normParams.wN = asNonNegativeNumberArray(normParamsObject.value(u"w_n"_s),
        u"norm_params_by_category.w_n"_s);

    // --- norm_noise_by_line ---------------------------------------------------
    const QJsonObject noiseObject =
        asObject(root.value(u"norm_noise_by_line"_s), u"norm_noise_by_line"_s);
    checkKeys(noiseObject, normNoiseKeys, u"norm_noise_by_line"_s);
    NormNoiseByLineConfig normNoiseByLine;
    normNoiseByLine.symmetrical = asNonNegativeNumberArray(noiseObject.value(u"symmetrical"_s),
        u"norm_noise_by_line.symmetrical"_s);
    normNoiseByLine.asymmetrical = asNonNegativeNumberArray(noiseObject.value(u"asymmetrical"_s),
        u"norm_noise_by_line.asymmetrical"_s);
    normNoiseByLine.power = asNonNegativeNumberArray(noiseObject.value(u"power"_s),
        u"norm_noise_by_line.power"_s);
    requireLength(normNoiseByLine.symmetrical, numberOfConstants,
        u"norm_noise_by_line.symmetrical"_s);
    requireLength(normNoiseByLine.asymmetrical, numberOfConstants,
        u"norm_noise_by_line.asymmetrical"_s);
    requireLength(normNoiseByLine.power, numberOfConstants, u"norm_noise_by_line.power"_s);

    // --- line ------------------------------------------------------------------
    const QJsonObject lineObject = asObject(root.value(u"line"_s), u"line"_s);
    checkKeys(lineObject, lineKeys, u"line"_s);
    LineConfig lines;
    lines.symmetrical = asStringList(lineObject.value(u"symmetrical"_s), u"line.symmetrical"_s);
    lines.asymmetrical = asStringList(lineObject.value(u"asymmetrical"_s), u"line.asymmetrical"_s);
    lines.power = asStringList(lineObject.value(u"power"_s), u"line.power"_s);

    // --- operation_modes -------------------------------------------------------
    QStringList operationModes =
        asStringList(root.value(u"operation_modes"_s), u"operation_modes"_s);

    AppConfig config;
    config.frequencyConstants = std::move(frequencyConstants);
    config.normParams = std::move(normParams);
    config.normNoiseByLine = std::move(normNoiseByLine);
    config.lines = std::move(lines);
    config.operationModes = std::move(operationModes);
    return config;
}

} // namespace sp::settings_store
