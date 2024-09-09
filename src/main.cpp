// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kirigami-workbench"));

    KAboutData about(QStringLiteral("kirigami-workbench"),
                     i18n("Kirigami Workbench"),
                     QStringLiteral("0.1"), // TODO: replace with ECM version
                     i18n("Quickly prototype in QML"),
                     KAboutLicense::GPL_V3,
                     QStringLiteral("© 2024 Joshua Goins"));
    about.addAuthor(QStringLiteral("Joshua Goins"),
                    QStringLiteral("Maintainer"),
                    QStringLiteral("josh@redstrate.com"),
                    QStringLiteral("https://redstrate.com/"),
                    QUrl(QStringLiteral("https://redstrate.com/rss-image.png")));

    KAboutData::setApplicationData(about);

    MainWindow w;
    w.show();

    return app.exec();
}