/// @file errors.h
/// @brief Base application errors. Messages are already translated at throw sites.
#pragma once

#include <QString>
#include <stdexcept>
#include <utility>

#include "i18n.h"

/// @brief Base application error class.
///
/// Carries the translated message both as std::runtime_error text and as a
/// QString for direct display in the interface.
class SignalProcessorError : public std::runtime_error {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~SignalProcessorError() override;

public:
    /// @brief Creates an error with an already translated message.
    /// @param message Text shown to the user.
    explicit SignalProcessorError(const QString& message)
        : std::runtime_error(message.toStdString()), m_message(message) {}

    /// @brief Returns the translated message.
    [[nodiscard]] const QString& message() const noexcept { return m_message; }

private:
    QString m_message; ///< Translated message.
};

/// @brief Base configuration error class.
class ConfigError : public SignalProcessorError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~ConfigError() override;
    using SignalProcessorError::SignalProcessorError;
};

/// @brief The settings file was not found.
class ConfigNotFoundError final : public ConfigError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~ConfigNotFoundError() override;

public:
    /// @brief Creates the error for a missing file.
    /// @param path Path of the file that could not be found.
    explicit ConfigNotFoundError(const QString& path)
        : ConfigError(tr_ns::tr("err.configNotFound", path)), m_path(path) {}

    /// @brief Returns the path of the missing file.
    [[nodiscard]] const QString& path() const noexcept { return m_path; }

private:
    QString m_path; ///< Path of the missing file.
};

/// @brief The file is corrupted: incorrect syntax or unacceptable data.
class ConfigValidationError final : public ConfigError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~ConfigValidationError() override;
    using ConfigError::ConfigError;
};

/// @brief Base spectrum-file error class.
class SpectrumError : public SignalProcessorError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~SpectrumError() override;
    using SignalProcessorError::SignalProcessorError;
};

/// @brief The spectrum file was not found on the medium.
class SpectrumNotFoundError final : public SpectrumError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~SpectrumNotFoundError() override;

public:
    /// @brief Creates the error for a missing file.
    /// @param path Path of the file that could not be found.
    explicit SpectrumNotFoundError(const QString& path)
        : SpectrumError(tr_ns::tr("err.spectrumNotFound", path)), m_path(path) {}

    /// @brief Returns the path of the missing file.
    [[nodiscard]] const QString& path() const noexcept { return m_path; }

private:
    QString m_path; ///< Path of the missing file.
};

/// @brief Incorrect format or data structure of the spectrum file.
class SpectrumFormatError final : public SpectrumError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~SpectrumFormatError() override;
    using SpectrumError::SpectrumError;
};

/// @brief The spectrum file is missing required control frequencies.
class SpectrumMissingFrequencyError final : public SpectrumError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~SpectrumMissingFrequencyError() override;

public:
    /// @brief Creates the error with the file name and the missing frequencies, Hz.
    /// @param filename Name of the file that lacks the frequencies.
    /// @param missing Missing frequencies as preformatted text, Hz.
    SpectrumMissingFrequencyError(const QString& filename, const QString& missing)
        : SpectrumError(tr_ns::tr("err.spectrumMissingFrequencies", filename, missing)),
          m_filename(filename), m_missingText(missing) {}

    /// @brief Returns the name of the file that lacks the frequencies.
    [[nodiscard]] const QString& filename() const noexcept { return m_filename; }
    /// @brief Returns the missing frequencies as preformatted text, Hz.
    [[nodiscard]] const QString& missingText() const noexcept { return m_missingText; }

private:
    QString m_filename;    ///< Name of the file that lacks the frequencies.
    QString m_missingText; ///< Preformatted list of missing frequencies.
};

/// @brief Incorrect parameters or calculation failure.
class CalculationError : public SignalProcessorError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~CalculationError() override;
    using SignalProcessorError::SignalProcessorError;
};

/// @brief Database access or operation failure (SQLite).
class DatabaseError : public SignalProcessorError {
public:
    /// @brief Trivial destructor, defined out-of-line so the vtable lands in errors.cpp.
    ~DatabaseError() override;
    using SignalProcessorError::SignalProcessorError;
};
