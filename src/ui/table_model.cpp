/// @file table_model.cpp
/// @brief Table model for the calculation points, with violation highlighting.
#include "table_model.h"

#include <QColor>
#include <QtMath>
#include <cmath>

namespace {

/// @brief Formats a frequency, dropping a trailing ".00".
QString formatFrequency(double value) {
    if (qFuzzyIsNull(value - std::floor(value))) {
        return QString::number(static_cast<qint64>(value));
    }
    return QString::number(value, 'f', 2);
}

} // namespace

namespace sp::ui {

MeasurementTableModel::MeasurementTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int MeasurementTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_points.size());
}

int MeasurementTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid() || !m_columnsSet) {
        return 0;
    }
    return columnCountValue;
}

QVariant MeasurementTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_points.size()) {
        return QVariant();
    }
    const sp::MeasurementPoint& point = m_points.at(index.row());
    const int column = index.column();

    switch (role) {
        case Qt::BackgroundRole:
            if (column == 6 && point.isViolation) {
                return QColor(QStringLiteral("#FEE2E2"));
            }
            return QVariant();
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (column) {
                case 0:
                    return point.index;
                case 1:
                    return formatFrequency(point.deltaF);
                case 2:
                    return formatFrequency(point.f);
                case 3:
                    return QString::number(point.uSn, 'f', 4);
                case 4:
                    return QString::number(point.uN, 'f', 4);
                case 5:
                    return QString::number(point.uS, 'f', 4);
                case 6:
                    return QString::number(point.q, 'f', 4);
                default:
                    return QVariant();
            }
        case Qt::TextAlignmentRole:
            if (column == 0) {
                return static_cast<int>(Qt::AlignCenter);
            }
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return QVariant();
    }
}

QVariant MeasurementTableModel::headerData(int section, Qt::Orientation orientation,
                                           int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal || !m_columnsSet) {
        return QVariant();
    }
    if (section < 0 || section >= columnCountValue) {
        return QVariant();
    }
    const char* const* columns =
        (m_measurementType == sp::MeasurementKind::Voltage) ? voltageColumns : currentColumns;
    return QString::fromUtf8(columns[section]);
}

void MeasurementTableModel::updateData(const QVector<sp::MeasurementPoint>& points,
                                       sp::MeasurementKind measurementType) {
    beginResetModel();
    m_points = points;
    m_measurementType = measurementType;
    m_columnsSet = !m_points.isEmpty();
    endResetModel();
}

void MeasurementTableModel::clear() {
    beginResetModel();
    m_points.clear();
    m_columnsSet = false;
    endResetModel();
}

} // namespace sp::ui
