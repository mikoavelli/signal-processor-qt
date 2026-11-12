/// @file settings_store.h
/// @brief Reading and validation of the settings.json file.
///
/// settings.json always lives next to the program; without it the application
/// does not start. The schema is validated strictly: unknown keys, wrong
/// types and unacceptable values are all reported with a precise message.
#pragma once

#include "models.h"

/// @brief Settings-file persistence.
namespace sp::settings_store {

/// @brief File name of the settings file, looked up next to the program.
inline constexpr char settingsName[] = "settings.json";

/// @brief Schema version supported by this program.
inline constexpr int schemaVersion = 1;

/// @brief Loads and validates settings.json.
/// @param settingsPath Full path of the settings file.
/// @return The validated configuration.
/// @throws ConfigNotFoundError If the file does not exist.
/// @throws ConfigValidationError If the file cannot be read, parsed, or fails
///     the schema and value checks.
AppConfig loadConfig(const QString &settingsPath);

} // namespace sp::settings_store
