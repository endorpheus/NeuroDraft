/*
 * NeuroDraft - Advanced Novel Writing Application
 * Concept and Development: Ryon Shane Hall
 * Built with Qt6 and C++20 for Linux
 */

#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("NeuroDraft");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Ryon Shane Hall");
    app.setApplicationDisplayName("NeuroDraft - Novel Writing Studio");
    
    // Ensure config directory exists
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    
    // Create main window
    MainWindow window;
    window.show();
    
    return app.exec();
}