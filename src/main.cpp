#if defined(__linux__)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWaylandIntegrationPlugin)
#endif

#include "mainwindow.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <SDL3/SDL.h>

int main(int argc, char *argv[])
{
    // Initialize basic SDL3 subsystem for file streams / error logging
    SDL_Init(0);

    // Request OpenGL 3.3 Core Profile before creating QApplication
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    int exitCode = a.exec();

    SDL_Quit();
    return exitCode;
}
