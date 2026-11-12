/// @file database.h
/// @brief Creation and writes of the SQLite measurements database.
#pragma once

#include "models.h"

#include <QString>

/// @brief Creates the measurements database if it does not exist.
/// @param dbPath Path of the SQLite database file.
/// @throws DatabaseError If the database cannot be created or opened.
void initDatabase(const QString &dbPath);

/// @brief Saves one measurement and its points.
/// @param dbPath Path of the SQLite database file.
/// @param deviceName Name of the device under test.
/// @param category 1-based object category.
/// @param lineNumber 1-based number of the line within the series.
/// @param lineName Full line name (filled template).
/// @param lineType Type of the measured line.
/// @param operationMode Operation mode as stored in the configuration.
/// @param measurementType Wire code of the measured quantity ("voltage"/"current").
/// @param resistance Resistance R, ohm.
/// @param result Calculation result to persist.
/// @return The identifier of the created measurements row.
/// @throws DatabaseError If the parameters are rejected, or the write fails.
int saveMeasurement(const QString &dbPath, const QString &deviceName,
    int category, int lineNumber, const QString &lineName, sp::LineType lineType,
    const QString &operationMode, const QString &measurementType, double resistance,
    const sp::CalculationResult &result);
