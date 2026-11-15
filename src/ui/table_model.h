/// @file table_model.h
/// @brief Table model for the calculation points, with violation highlighting.
#pragma once

#include "models.h"

#include <QAbstractTableModel>

namespace sp::ui {

/// @brief Shows one row per control frequency; the q cell is tinted on violations.
class MeasurementTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    static constexpr const char* const voltageColumns[] = {
        "№",   "Δf, Гц", "f, Гц", "U_сш",
        "U_ш", "U_с",    "q"}; ///< Column headers for voltage measurements.
    static constexpr const char* const currentColumns[] = {
        "№",   "Δf, Гц", "f, Гц", "I_сш",
        "I_ш", "I_с",    "q"};                 ///< Column headers for current measurements.
    static constexpr int columnCountValue = 7; ///< Fixed number of columns.

    explicit MeasurementTableModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;

    /// @brief Replaces the model contents.
    /// @param points Points of the latest calculation.
    /// @param measurementType Measured quantity (selects the column headers).
    void updateData(const QVector<sp::MeasurementPoint>& points,
                    sp::MeasurementKind measurementType);

    /// @brief Removes all rows and clears the columns.
    void clear();

private:
    QVector<sp::MeasurementPoint> m_points; ///< Current rows.
    bool m_columnsSet = false; ///< Whether the column set matches the measurements type.
    sp::MeasurementKind m_measurementType = sp::MeasurementKind::Voltage; ///< Active column set.
};

} // namespace sp::ui
