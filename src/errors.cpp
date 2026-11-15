/// @file errors.cpp
/// @brief Out-of-line destructors for the error hierarchy (see errors.h).
#include "errors.h"

SignalProcessorError::~SignalProcessorError() = default;

ConfigError::~ConfigError() = default;
ConfigNotFoundError::~ConfigNotFoundError() = default;
ConfigValidationError::~ConfigValidationError() = default;

SpectrumError::~SpectrumError() = default;
SpectrumNotFoundError::~SpectrumNotFoundError() = default;
SpectrumFormatError::~SpectrumFormatError() = default;
SpectrumMissingFrequencyError::~SpectrumMissingFrequencyError() = default;

CalculationError::~CalculationError() = default;
DatabaseError::~DatabaseError() = default;
