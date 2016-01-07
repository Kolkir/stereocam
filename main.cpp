#include "mainwindow.h"

#include "camerautils.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char** argv)
{
    auto devs = camera::utils::getDeviceList();
    for (auto& dev : devs)
    {
        auto formats = camera::utils::getDeviceFormats(dev.id);
    }

    QApplication app(argc, argv);
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
