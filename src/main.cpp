#include <KLocalizedString>
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kirigami-workbench"));

    MainWindow w;
    w.show();

    return app.exec();
}