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
#include <QFont>
#include <QMessageBox>
#include <QStyleFactory>


int main(int argc, char *argv[])
{
    // The starting folder is the one with settings.json: the configuration
    // file always sits next to the binary.
    const QString baseDir = QCoreApplication::applicationDirPath();
    QDir::setCurrent(baseDir);

    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setFont(QFont(QStringLiteral("Segoe UI"), 9));

    // The translation is compiled into the binary; initialise it before any
    // error message is produced.
    static_cast<void>(tr_ns::tr("window.title"));

    const QString settingsPath = QDir(baseDir).filePath(
        QString::fromUtf8(sp::settings_store::settingsName));

    try {
        const sp::AppConfig config = sp::settings_store::loadConfig(settingsPath);
        initDatabase(QDir(baseDir).filePath(QStringLiteral("measurements.db")));

        sp::ui::StartupDialog startup;
        if (startup.exec() != QDialog::Accepted) {
            return 0;
        }
        const auto [deviceName, categoryIndex] = startup.parameters();
        sp::ui::MainWindow window(config, deviceName, categoryIndex, baseDir);
        window.show();
        return app.exec();
    } catch (const ConfigNotFoundError &error) {
        QMessageBox::critical(nullptr, tr_ns::tr("startup.settingsNotFound.title"),
            tr_ns::tr("startup.settingsNotFound.text", error.path()));
        return 1;
    } catch (const ConfigError &error) {
        QMessageBox::critical(nullptr, tr_ns::tr("startup.settingsError.title"), error.message());
        return 1;
    } catch (const DatabaseError &error) {
        QMessageBox::critical(nullptr, tr_ns::tr("startup.databaseError.title"), error.message());
        return 1;
    }
}
