/// @file main.cpp
/// @brief Program entry point.
///
/// Loads settings.json (which must sit next to the binary), initialises the
/// database and shows the main window after the startup dialog.
#include "database.h"
#include "errors.h"
#include "i18n.h"
#include "settings_store.h"
#include "ui/main_window.h"
#include "ui/startup_dialog.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>

/// @brief Initialises measurements.db next to the executable, falling back to a
/// per-user writable location when the executable's directory is not writable
/// (e.g. C:\ root or Program Files). Returns the path actually used.
static QString initializeMeasurementsDatabase(const QString& baseDir) {
    const QString adjacent = QDir(baseDir).filePath(QStringLiteral("measurements.db"));
    try {
        initDatabase(adjacent);
        return adjacent;
    } catch (const DatabaseError&) {
        // Fallback: per-user data directory (%LOCALAPPDATA%/SignalProcessor on
        // Windows, ~/.local/share/SignalProcessor on Linux). Preserve an
        // existing adjacent database by copying it over if possible.
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                            QStringLiteral("/SignalProcessor");
        QDir().mkpath(dir);
        const QString fallback = QDir(dir).filePath(QStringLiteral("measurements.db"));
        if (!QFileInfo::exists(fallback) && QFileInfo::exists(adjacent)) {
            QFile::copy(adjacent, fallback); // best effort; ignore failures
        }
        initDatabase(fallback); // throws DatabaseError on failure
        return fallback;
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setFont(QFont(QStringLiteral("Segoe UI"), 9));

    // NOTE: applicationDirPath() must be called after the QApplication exists,
    // otherwise it returns an empty string and every path derived from it is
    // resolved against the (unpredictable) working directory.
    // The starting folder is the one with settings.json: the configuration
    // file always sits next to the binary.
    const QString baseDir = QCoreApplication::applicationDirPath();
    QDir::setCurrent(baseDir);

    // The translation is compiled into the binary; initialise it before any
    // error message is produced.
    static_cast<void>(tr_ns::tr("window.title"));

    const QString settingsPath =
        QDir(baseDir).filePath(QString::fromUtf8(sp::settings_store::settingsName));

    try {
        const sp::AppConfig config = sp::settings_store::loadConfig(settingsPath);
        const QString databasePath = initializeMeasurementsDatabase(baseDir);

        sp::ui::StartupDialog startup;
        if (startup.exec() != QDialog::Accepted) {
            return 0;
        }
        const auto [deviceName, categoryIndex] = startup.parameters();
        sp::ui::MainWindow window(config, deviceName, categoryIndex, databasePath);
        window.show();
        return app.exec();
    } catch (const ConfigNotFoundError& error) {
        QMessageBox::critical(nullptr, tr_ns::tr("startup.settingsNotFound.title"),
                              tr_ns::tr("startup.settingsNotFound.text", error.path()));
        return 1;
    } catch (const ConfigError& error) {
        QMessageBox::critical(nullptr, tr_ns::tr("startup.settingsError.title"), error.message());
        return 1;
    } catch (const DatabaseError& error) {
        QMessageBox::critical(nullptr, tr_ns::tr("startup.databaseError.title"), error.message());
        return 1;
    }
}
