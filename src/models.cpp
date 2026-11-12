/// @file models.cpp
/// @brief Implementations of the wire-code conversions declared in models.h.
#include "models.h"

namespace sp {

QString lineTypeToString(LineType lineType)
{
    switch (lineType) {
    case LineType::Symmetrical:
        return QStringLiteral("symmetrical");
    case LineType::Asymmetrical:
        return QStringLiteral("asymmetrical");
    case LineType::Power:
        return QStringLiteral("power");
    }
    return QString(); // unreachable: all enumerators are handled
}

std::optional<LineType> lineTypeFromString(const QString &text)
{
    if (text == QLatin1StringView("symmetrical")) {
        return LineType::Symmetrical;
    }
    if (text == QLatin1StringView("asymmetrical")) {
        return LineType::Asymmetrical;
    }
    if (text == QLatin1StringView("power")) {
        return LineType::Power;
    }
    return std::nullopt;
}

QString measurementKindToString(MeasurementKind measurementType)
{
    switch (measurementType) {
    case MeasurementKind::Voltage:
        return QStringLiteral("voltage");
    case MeasurementKind::Current:
        return QStringLiteral("current");
    }
    return QString(); // unreachable: all enumerators are handled
}

std::optional<MeasurementKind> measurementKindFromString(const QString &text)
{
    if (text == QLatin1StringView("voltage")) {
        return MeasurementKind::Voltage;
    }
    if (text == QLatin1StringView("current")) {
        return MeasurementKind::Current;
    }
    return std::nullopt;
}

const QVector<double> &lineNoise(const NormNoiseByLineConfig &config, LineType lineType) noexcept
{
    switch (lineType) {
    case LineType::Symmetrical:
        return config.symmetrical;
    case LineType::Asymmetrical:
        return config.asymmetrical;
    case LineType::Power:
        return config.power;
    }
    return config.power; // unreachable: all enumerators are handled
}

} // namespace sp
