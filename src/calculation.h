/// @file calculation.h
/// @brief Public API for the spectrum calculations.
#pragma once

#include "models.h"

/// @brief Calculates the measurement result for one line.
/// @param signalNoisePath Path of the mixture (signal + noise) spectrum file.
/// @param noisePath Path of the own-noise spectrum file.
/// @param config Application configuration.
/// @param categoryIndex 0-based object category index.
/// @param lineType Type of the measured line.
/// @param measurementType Which quantity is measured.
/// @param resistance Value of the resistance R, ohm.
/// @return The calculated points and, when there are violations, the W totals.
/// @throws CalculationError If the parameters are unacceptable.
/// @throws SpectrumError If a spectrum file cannot be read.
sp::CalculationResult calculate(const QString& signalNoisePath, const QString& noisePath,
                                const sp::AppConfig& config, int categoryIndex,
                                sp::LineType lineType, sp::MeasurementKind measurementType,
                                std::optional<double> resistance);

/// @brief Intermediate U value for one point (currently equals the noise norm).
/// @param resistance Resistance R, ohm (reserved for future line-dependent formulas).
/// @param deltaF Band/control value of the point, Hz (reserved).
/// @param normNoise Normalised noise for the point.
/// @param lineType Type of the measured line.
/// @return The intermediate value.
double calculateIntermediateU(double resistance, double deltaF, double normNoise,
                              sp::LineType lineType);

/// @brief Computes q for one point.
/// @param signalLevel Signal level of the point.
/// @param intermediateU Intermediate value of the point; must be positive.
/// @param measurementType Which quantity is measured.
/// @param resistance Resistance R, ohm.
/// @return The q ratio.
/// @throws CalculationError If @p intermediateU is not positive.
double calculateQ(double signalLevel, double intermediateU, sp::MeasurementKind measurementType,
                  double resistance);

/// @brief Computes the total W from the weighted r_i sum (identity).
/// @param rSum Weighted sum of the point r_i values.
/// @return The same value.
double calculateW(double rSum);
