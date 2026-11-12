/// @file test_database.cpp
/// @brief Tests for the SQLite measurements database; port of the Python tests.
#include "database.h"
#include "errors.h"
#include "models.h"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QVariant>
#include <QtTest>

namespace {

/// @brief Builds a mock result with 20 points.
sp::CalculationResult makeMockResult(bool hasViolations = false)
{
    sp::CalculationResult result;
    result.hasViolations = hasViolations;
    result.measurementType = sp::MeasurementKind::Voltage;
    if (hasViolations) {
        result.w = 25.0;
        result.wN = 1000.0;
        result.isWViolation = false;
    }
    for (int index = 1; index <= 20; ++index) {
        sp::MeasurementPoint point;
        point.index = index;
        point.deltaF = 100.0;
        point.f = static_cast<double>(index * 500);
        point.uSn = 3.0;
        point.uN = 1.0;
        point.uS = 2.8284;
        point.q = 2.8284;
        point.isViolation = hasViolations;
        point.rI = hasViolations ? std::optional<double>(12.5) : std::nullopt;
        result.points.append(point);
    }
    return result;
}

/// @brief Opens a raw connection for verification queries.
QSqlDatabase openRaw(const QString &dbPath)
{
    const QString name = QStringLiteral("raw_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    database.setDatabaseName(dbPath);
    if (!database.open()) {
        qFatal("cannot open %s for verification", qPrintable(dbPath));
    }
    return database;
}

void closeRaw(QSqlDatabase &database)
{
    const QString name = database.connectionName();
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

} // namespace

class TestDatabase : public QObject {
    Q_OBJECT

private slots:
    void initCreatesTablesAndIndex();
    void initInvalidPathThrows();
    void savePersistsData();
    void cascadingDelete();
    void rejectsNonPositiveLineNumber();
    void rejectsInvalidCategory();
};

void TestDatabase::initCreatesTablesAndIndex()
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("test.db"));
    initDatabase(dbPath);

    QSqlDatabase raw = openRaw(dbPath);
    QSqlQuery query(raw);
    QVERIFY(query.exec(QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='measurements'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);

    QVERIFY(query.exec(QStringLiteral(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='index'"
        " AND name='idx_measurement_points_measurement_id'")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    closeRaw(raw);
}

void TestDatabase::initInvalidPathThrows()
{
    QTemporaryDir dir;
    QVERIFY_EXCEPTION_THROWN(initDatabase(dir.path()), DatabaseError);
}

void TestDatabase::savePersistsData()
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("test.db"));
    initDatabase(dbPath);
    const sp::CalculationResult result = makeMockResult(/*hasViolations=*/true);

    const int measId = saveMeasurement(dbPath, QStringLiteral("TestDevice"), 1, 1,
        QStringLiteral("Line 1"), sp::LineType::Power, QStringLiteral("XX"),
        QStringLiteral("voltage"), 50.0, result);
    QVERIFY(measId > 0);

    QSqlDatabase raw = openRaw(dbPath);
    QSqlQuery query(raw);
    QVERIFY(query.exec(QStringLiteral(
        "SELECT COUNT(*) FROM measurement_points WHERE measurement_id = %1").arg(measId)));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 20);
    closeRaw(raw);
}

void TestDatabase::cascadingDelete()
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("test.db"));
    initDatabase(dbPath);
    const sp::CalculationResult result = makeMockResult();

    const int measId = saveMeasurement(dbPath, QStringLiteral("TestDevice"), 1, 1,
        QStringLiteral("Line 1"), sp::LineType::Power, QStringLiteral("XX"),
        QStringLiteral("voltage"), 50.0, result);

    QSqlDatabase raw = openRaw(dbPath);
    QSqlQuery pragma(raw);
    QVERIFY(pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON")));
    QSqlQuery deleteQuery(raw);
    QVERIFY(deleteQuery.exec(QStringLiteral("DELETE FROM measurements WHERE id = %1").arg(measId)));

    QSqlQuery countQuery(raw);
    QVERIFY(countQuery.exec(QStringLiteral(
        "SELECT COUNT(*) FROM measurement_points WHERE measurement_id = %1").arg(measId)));
    QVERIFY(countQuery.next());
    QCOMPARE(countQuery.value(0).toInt(), 0);
    closeRaw(raw);
}

void TestDatabase::rejectsNonPositiveLineNumber()
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("test.db"));
    initDatabase(dbPath);
    const sp::CalculationResult result = makeMockResult();

    QVERIFY_EXCEPTION_THROWN(
        saveMeasurement(dbPath, QStringLiteral("Device"), 1, 0, QStringLiteral("Line 1"),
            sp::LineType::Power, QStringLiteral("XX"), QStringLiteral("voltage"), 50.0, result),
        DatabaseError);
}

void TestDatabase::rejectsInvalidCategory()
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("test.db"));
    initDatabase(dbPath);
    const sp::CalculationResult result = makeMockResult();

    QVERIFY_EXCEPTION_THROWN(
        saveMeasurement(dbPath, QStringLiteral("Device"), -1, 1, QStringLiteral("Line 1"),
            sp::LineType::Power, QStringLiteral("XX"), QStringLiteral("voltage"), 50.0, result),
        DatabaseError);
}

QTEST_MAIN(TestDatabase)
#include "test_database.moc"
