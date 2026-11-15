/// @file calculation.cpp
/// @brief Implementation of the spectrum calculations declared in calculation.h.
///
/// The Python reference implementation was used as the source of truth for the
/// formulas; the negative tests of the original test suite are implemented here
/// as well (negative constants are rejected, the intermediate value must be
/// positive, the resistance is range-checked).
#include "calculation.h"

#include "errors.h"
#include "i18n.h"
#include "spectrum_reader.h"

#include <QtMath>
#include <cmath>

namespace {

/// @brief Upper bound of R, ohm: above it the model does not apply.
inline constexpr double maxResistanceOhm = 1e9;

/// @brief Rounds to four decimals, like round(x, 4) in the reference implementation.
double round4(double value) {
    return std::round(value * 1e4) / 1e4;
}

/// @brief Computes the signal level from the mixture and the own noise.
double computeSignalLevel(double signalNoise, double noise) {
    const double signalPower = signalNoise * signalNoise - noise * noise;
    if (signalPower <= 0.0) {
        return 0.0;
    }
    return std::sqrt(signalPower);
}

/// @brief Computes the r_i coefficient of a single violating point.
double computePointRI(double q, double deltaA, double frequency) {
    const double logFactor = std::log(1.0 + frequency);
    const double scaledQ = q * deltaA;
    return round4(scaledQ * scaledQ * logFactor);
}

} // namespace

double calculateIntermediateU(double resistance, double deltaF, double normNoise,
                              sp::LineType lineType) {
    Q_UNUSED(resistance)
    Q_UNUSED(deltaF)
    switch (lineType) {
        case sp::LineType::Symmetrical:
        case sp::LineType::Asymmetrical:
        case sp::LineType::Power:
            return normNoise;
    }
    return normNoise; // unreachable: all enumerators are handled
}

double calculateQ(double signalLevel, double intermediateU, sp::MeasurementKind measurementType,
                  double resistance, int pointIndex) {
    if (intermediateU <= 0.0) {
        // Dividing by a non-positive norm is meaningless; the message names the point.
        throw CalculationError(
            tr_ns::tr("err.intermediateNotPositive", QString::number(pointIndex)));
    }
    if (measurementType == sp::MeasurementKind::Current) {
        return (signalLevel * resistance) / intermediateU;
    }
    return signalLevel / intermediateU;
}

double calculateW(double rSum) {
    return rSum;
}

sp::CalculationResult calculate(const QString& signalNoisePath, const QString& noisePath,
                                const sp::AppConfig& config, int categoryIndex,
                                sp::LineType lineType, sp::MeasurementKind measurementType,
                                std::optional<double> resistance) {
    // ------------------------------------------------------------------
    // Validate the inputs.
    // ------------------------------------------------------------------
    if (!resistance.has_value() || resistance.value() <= 0.0) {
        throw CalculationError(tr_ns::tr("err.resistanceNotPositive"));
    }
    const double validResistance = resistance.value();
    if (!qIsFinite(validResistance)) {
        throw CalculationError(tr_ns::tr("err.resistanceNotPositive"));
    }
    if (validResistance > maxResistanceOhm) {
        throw CalculationError(
            tr_ns::tr("err.resistanceTooLarge", QString::number(maxResistanceOhm, 'f', 0)));
    }

    const sp::FrequencyConstantsConfig& constants = config.frequencyConstants;
    const int numberOfConstants = constants.numberOfConstants;
    if (numberOfConstants <= 0) {
        throw CalculationError(tr_ns::tr("err.numberOfConstantsNotPositive"));
    }
    const QVector<double>& lineNoiseValues = sp::lineNoise(config.normNoiseByLine, lineType);
    if (lineNoiseValues.size() != numberOfConstants) {
        throw CalculationError(
            tr_ns::tr("err.lineNoiseCountMismatch", QString::number(numberOfConstants)));
    }
    if (constants.fI.size() != numberOfConstants || constants.deltaFI.size() != numberOfConstants ||
        constants.kI.size() != numberOfConstants || constants.deltaAI.size() != numberOfConstants) {
        throw CalculationError(
            tr_ns::tr("err.constantsCountMismatch", QString::number(numberOfConstants)));
    }
    for (int index = 0; index < numberOfConstants; ++index) {
        if (constants.fI.at(index) < 0.0) {
            throw CalculationError(
                tr_ns::tr("err.negativeFrequencyConstant", QString::number(index + 1)));
        }
    }

    const sp::NormParamsConfig& normParams = config.normParams;
    if (categoryIndex < 0 || categoryIndex >= normParams.deltaStn.size() ||
        categoryIndex >= normParams.wN.size()) {
        throw CalculationError(
            tr_ns::tr("err.categoryMissing", QString::number(categoryIndex + 1)));
    }
    const double deltaStn = normParams.deltaStn.at(categoryIndex);
    const double wN = normParams.wN.at(categoryIndex);

    // ------------------------------------------------------------------
    // Read the spectra.
    // ------------------------------------------------------------------
    const QVector<double> snValues = readRequiredFrequencies(signalNoisePath, constants.fI);
    const QVector<double> nValues = readRequiredFrequencies(noisePath, constants.fI);

    // ------------------------------------------------------------------
    // Calculate the points.
    // ------------------------------------------------------------------
    QVector<sp::MeasurementPoint> points;
    points.reserve(numberOfConstants);
    bool hasViolations = false;
    for (int index = 0; index < numberOfConstants; ++index) {
        const double signalLevel = computeSignalLevel(snValues.at(index), nValues.at(index));
        const double intermediateU = calculateIntermediateU(
            validResistance, constants.deltaFI.at(index), lineNoiseValues.at(index), lineType);
        const double q =
            calculateQ(signalLevel, intermediateU, measurementType, validResistance, index + 1);
        sp::MeasurementPoint point;
        point.index = index + 1;
        point.deltaF = round4(constants.deltaFI.at(index));
        point.f = round4(constants.fI.at(index));
        point.uSn = round4(snValues.at(index));
        point.uN = round4(nValues.at(index));
        point.uS = round4(signalLevel);
        point.q = round4(q);
        point.isViolation = q >= deltaStn;
        point.rI = std::nullopt;
        hasViolations = hasViolations || point.isViolation;
        points.append(point);
    }

    // ------------------------------------------------------------------
    // Finalise: compute W only when there are violations.
    // ------------------------------------------------------------------
    sp::CalculationResult result;
    result.hasViolations = hasViolations;
    result.measurementType = measurementType;
    if (!hasViolations) {
        result.points = std::move(points);
        result.w = std::nullopt;
        result.wN = std::nullopt;
        result.isWViolation = std::nullopt;
        return result;
    }

    double rWeightedSum = 0.0;
    for (int index = 0; index < points.size(); ++index) {
        sp::MeasurementPoint& point = points[index];
        const double rI = computePointRI(point.q, constants.deltaAI.at(index), point.f);
        rWeightedSum += rI * constants.kI.at(index);
        point.rI = rI;
    }
    const double wTotal = round4(calculateW(rWeightedSum));

    result.points = std::move(points);
    result.w = wTotal;
    result.wN = wN;
    result.isWViolation = wTotal >= wN;
    return result;
}
