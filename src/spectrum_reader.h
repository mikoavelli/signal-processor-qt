/// @file spectrum_reader.h
/// @brief Reading of control frequencies from text spectrum files.
///
/// The text format: one record per line, frequency and amplitude separated by
/// whitespace (or tabs). The decimal separator may be a comma or a dot; commas
/// are converted. Empty lines and lines starting with '#', '[' or '(' are
/// comments and are skipped. Unparseable lines are skipped; physically impossible
/// records (negative frequencies or amplitudes) and conflicting duplicates are
/// hard errors.
#pragma once

#include <QVector>
#include <QString>

/// @brief Reads the requested control frequencies from a spectrum file, in request order.
/// @param filePath Path of the spectrum file.
/// @param frequencies Control frequencies to look up, Hz.
/// @return The amplitudes found, in the same order as @p frequencies.
/// @throws CalculationError If @p frequencies is empty.
/// @throws SpectrumNotFoundError If the file does not exist.
/// @throws SpectrumFormatError If the file cannot be read, contains a negative
///     frequency or amplitude, or two conflicting records for the same frequency.
/// @throws SpectrumMissingFrequencyError If some control frequencies are absent.
QVector<double> readRequiredFrequencies(const QString& filePath,
                                        const QVector<double>& frequencies);
