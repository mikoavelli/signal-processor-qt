/// @file database.cpp
/// @brief Creation and writes of the SQLite measurements database.
///
/// Implementation of the database API declared in database.h. The schema matches
/// the Python reference implementation: measurements + measurement_points with a
/// cascade delete and an index on measurement_points.measurement_id.
#include "database.h"

#include "errors.h"
#include "i18n.h"

#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>

namespace {

/// @brief RAII helper for a uniquely named SQLite connection.
class Connection {
public:
    explicit Connection(const QString& dbPath)
        : m_name(QStringLiteral("sp_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {
        m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
        m_database.setDatabaseName(dbPath);
        if (!m_database.open()) {
            const QString errorText = m_database.lastError().text();
            cleanup();
            m_valid = false;
            throw DatabaseError(tr_ns::tr("err.dbInit", QFileInfo(dbPath).fileName(), errorText));
        }
        m_valid = true;
    }
    ~Connection() { cleanup(); }
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;

    QSqlDatabase& db() { return m_database; }

private:
    void cleanup() {
        if (m_database.isValid()) {
            if (m_database.isOpen()) {
                m_database.close();
            }
            m_database = QSqlDatabase(); // release before removing by name
            QSqlDatabase::removeDatabase(m_name);
        }
    }

    QString m_name;          ///< Unique connection name.
    QSqlDatabase m_database; ///< The connection (invalid after cleanup).
    bool m_valid = false;    ///< Whether the connection opened successfully.
};

/// @brief Executes a statement or throws DatabaseError.
void execOrThrow(QSqlDatabase& database, const QString& statement, const QString& where) {
    QSqlQuery query(database);
    if (!query.exec(statement)) {
        throw DatabaseError(tr_ns::tr("err.dbInit", where, query.lastError().text()));
    }
}

} // namespace

void initDatabase(const QString& dbPath) {
    Connection connection(dbPath);
    QSqlDatabase& database = connection.db();
    const QString fileName = QFileInfo(dbPath).fileName();

    QSqlQuery pragma(database);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    execOrThrow(database,
                QStringLiteral("CREATE TABLE IF NOT EXISTS measurements ("
                               "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
                               "    device_name TEXT NOT NULL,"
                               "    category INTEGER NOT NULL,"
                               "    line_number INTEGER NOT NULL,"
                               "    line_name TEXT NOT NULL,"
                               "    line_type TEXT NOT NULL,"
                               "    operation_mode TEXT NOT NULL,"
                               "    measurement_type TEXT NOT NULL,"
                               "    parameter_r REAL NOT NULL,"
                               "    has_violations INTEGER NOT NULL,"
                               "    w REAL,"
                               "    is_w_violation INTEGER,"
                               "    created_at TEXT DEFAULT CURRENT_TIMESTAMP"
                               ")"),
                fileName);
    execOrThrow(database,
                QStringLiteral(
                    "CREATE TABLE IF NOT EXISTS measurement_points ("
                    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "    measurement_id INTEGER NOT NULL,"
                    "    point_index INTEGER NOT NULL,"
                    "    delta_f REAL NOT NULL,"
                    "    f REAL NOT NULL,"
                    "    u_sn REAL NOT NULL,"
                    "    u_n REAL NOT NULL,"
                    "    u_s REAL NOT NULL,"
                    "    q REAL NOT NULL,"
                    "    is_violation INTEGER NOT NULL,"
                    "    r_i REAL,"
                    "    FOREIGN KEY (measurement_id) REFERENCES measurements(id) ON DELETE CASCADE"
                    ")"),
                fileName);
    execOrThrow(database,
                QStringLiteral("CREATE INDEX IF NOT EXISTS idx_measurement_points_measurement_id"
                               " ON measurement_points(measurement_id)"),
                fileName);
}

int saveMeasurement(const QString& dbPath, const QString& deviceName, int category, int lineNumber,
                    const QString& lineName, sp::LineType lineType, const QString& operationMode,
                    const QString& measurementType, double resistance,
                    const sp::CalculationResult& result) {
    if (category < 1) {
        throw DatabaseError(tr_ns::tr("err.dbCategory", QString::number(category)));
    }
    if (lineNumber < 1) {
        throw DatabaseError(tr_ns::tr("err.dbLineNumber", QString::number(lineNumber)));
    }

    Connection connection(dbPath);
    QSqlDatabase& database = connection.db();

    QSqlQuery pragma(database);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    if (!database.transaction()) {
        throw DatabaseError(tr_ns::tr("err.dbSave", database.lastError().text()));
    }
    try {
        QSqlQuery insert(database);
        insert.prepare(
            QStringLiteral("INSERT INTO measurements ("
                           "    device_name, category, line_number, line_name, line_type,"
                           "    operation_mode, measurement_type, parameter_r, has_violations,"
                           "    w, is_w_violation"
                           ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        insert.addBindValue(deviceName);
        insert.addBindValue(category);
        insert.addBindValue(lineNumber);
        insert.addBindValue(lineName);
        insert.addBindValue(sp::lineTypeToString(lineType));
        insert.addBindValue(operationMode);
        insert.addBindValue(measurementType);
        insert.addBindValue(resistance);
        insert.addBindValue(result.hasViolations ? 1 : 0);
        insert.addBindValue(result.w.has_value() ? QVariant(result.w.value()) : QVariant());
        insert.addBindValue(result.isWViolation.has_value()
                                ? QVariant(result.isWViolation.value() ? 1 : 0)
                                : QVariant());
        if (!insert.exec()) {
            throw DatabaseError(tr_ns::tr("err.dbSave", insert.lastError().text()));
        }
        const QVariant lastId = insert.lastInsertId();
        if (!lastId.isValid()) {
            throw DatabaseError(tr_ns::tr("err.dbId"));
        }
        const int measurementId = lastId.toInt();

        for (const sp::MeasurementPoint& point : result.points) {
            QSqlQuery pointInsert(database);
            pointInsert.prepare(QStringLiteral(
                "INSERT INTO measurement_points ("
                "    measurement_id, point_index, delta_f, f, u_sn, u_n, u_s, q, is_violation, r_i"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
            pointInsert.addBindValue(measurementId);
            pointInsert.addBindValue(point.index);
            pointInsert.addBindValue(point.deltaF);
            pointInsert.addBindValue(point.f);
            pointInsert.addBindValue(point.uSn);
            pointInsert.addBindValue(point.uN);
            pointInsert.addBindValue(point.uS);
            pointInsert.addBindValue(point.q);
            pointInsert.addBindValue(point.isViolation ? 1 : 0);
            pointInsert.addBindValue(point.rI.has_value() ? QVariant(point.rI.value())
                                                          : QVariant());
            if (!pointInsert.exec()) {
                throw DatabaseError(tr_ns::tr("err.dbSave", pointInsert.lastError().text()));
            }
        }

        if (!database.commit()) {
            throw DatabaseError(tr_ns::tr("err.dbSave", database.lastError().text()));
        }
        return measurementId;
    } catch (const DatabaseError&) {
        database.rollback();
        throw;
    }
}
