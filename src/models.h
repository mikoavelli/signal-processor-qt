/// @file models.h
/// @brief Settings models and calculation results shared by all parts of the program.
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>
#include <QMetaType>

/// @brief Core domain models shared by all parts of the program.
namespace sp {

/// @brief Which measured line the spectrum belongs to.
enum class LineType {
    Symmetrical,  ///< Symmetrical line (e.g. wire pair).
    Asymmetrical, ///< Asymmetrical line (e.g. antenna cable).
    Power,        ///< Power line.
};

/// @brief Which physical quantity is measured.
enum class MeasurementKind {
    Voltage, ///< Voltage measurement.
    Current, ///< Current measurement.
};

/// @brief Control-frequency constants. Order of values matches the report points.
struct FrequencyConstantsConfig {
    int numberOfConstants = 0; ///< Number of control constants.
    QVector<double> fI;        ///< Central frequencies, Hz.
    QVector<double> deltaFI;   ///< Band/control values for each point, Hz.
    QVector<double> kI;        ///< Weighting coefficients k for each control frequency.
    QVector<double> deltaAI;   ///< Coefficients A for each control frequency.
};

/// @brief Normalised parameters depending on the object category.
struct NormParamsConfig {
    QVector<double> deltaStn; ///< Violation thresholds per category.
    QVector<double> wN;       ///< Norm values of W per category.
};

/// @brief Normalised noise for each line type; one value per control frequency.
struct NormNoiseByLineConfig {
    QVector<double> symmetrical;  ///< Noise normals for symmetrical lines.
    QVector<double> asymmetrical; ///< Noise normals for asymmetrical lines.
    QVector<double> power;        ///< Noise normals for power lines.
};

/// @brief Line-name templates. "{}" marks a field filled in by the user.
struct LineConfig {
    QStringList symmetrical;  ///< Templates for symmetrical lines.
    QStringList asymmetrical; ///< Templates for asymmetrical lines.
    QStringList power;        ///< Templates for power lines.
};

/// @brief All program configuration; stored in settings.json next to the program.
struct AppConfig {
    FrequencyConstantsConfig frequencyConstants; ///< Control-frequency constants.
    NormParamsConfig normParams;                 ///< Normalised parameters by category.
    NormNoiseByLineConfig normNoiseByLine;       ///< Normalised noise by line type.
    LineConfig lines;                            ///< Line-name templates by line type.
    QStringList operationModes;                  ///< Operation modes shown in the interface.
};

/// @brief One row of the measurement table.
struct MeasurementPoint {
    int index = 0;            ///< 1-based point number.
    double deltaF = 0.0;      ///< Band/control value, Hz.
    double f = 0.0;           ///< Control frequency, Hz.
    double uSn = 0.0;         ///< Mixture level (signal + noise), V or A.
    double uN = 0.0;          ///< Own noise level, V or A.
    double uS = 0.0;          ///< Signal level, V or A.
    double q = 0.0;           ///< Signal-to-norm ratio q.
    bool isViolation = false; ///< @c true if q reaches the norm for the category.
    std::optional<double> rI; ///< Per-point r_i coefficient, set only when there are violations.
};

/// @brief Result of one calculation run.
struct CalculationResult {
    QVector<MeasurementPoint> points; ///< One point per control frequency.
    bool hasViolations = false;       ///< @c true if at least one point is a violation.
    MeasurementKind measurementType = MeasurementKind::Voltage; ///< Measured quantity.
    std::optional<double> w;          ///< Total W, set only when there are violations.
    std::optional<double> wN;         ///< Norm of W for the category, set only with violations.
    std::optional<bool> isWViolation; ///< @c true if W reaches the norm, unset without violations.
};

/// @brief Returns the stable wire code of a line type ("symmetrical"/"asymmetrical"/"power").
/// @param lineType Line type to convert.
[[nodiscard]] QString lineTypeToString(LineType lineType);

/// @brief Parses the stable wire code of a line type.
/// @param text Text to parse.
/// @return The line type, or empty if the text is not a known code.
[[nodiscard]] std::optional<LineType> lineTypeFromString(const QString& text);

/// @brief Returns the stable wire code of a measurement kind ("voltage"/"current").
/// @param measurementType Measurement kind to convert.
[[nodiscard]] QString measurementKindToString(MeasurementKind measurementType);

/// @brief Parses the stable wire code of a measurement kind.
/// @param text Text to parse.
/// @return The measurement kind, or empty if the text is not a known code.
[[nodiscard]] std::optional<MeasurementKind> measurementKindFromString(const QString& text);

/// @brief Returns the noise normal for the chosen line type.
/// @param config Normals for each line type.
/// @param lineType Line type to pick.
[[nodiscard]] const QVector<double>& lineNoise(const NormNoiseByLineConfig& config,
                                               LineType lineType) noexcept;

} // namespace sp

Q_DECLARE_METATYPE(sp::LineType)
Q_DECLARE_METATYPE(sp::MeasurementKind)
