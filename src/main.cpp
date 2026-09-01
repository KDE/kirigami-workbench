// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <KAboutData>
#include <KIconTheme>
#include <KLocalizedString>
#include <QApplication>
#include <QByteArray>
#include <QStyleHints>

#include "mainwindow.h"
using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();

    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kirigami-workbench"));

    KAboutData about(u"kirigami-workbench"_s,
                     i18n("Kirigami Workbench"),
                     u"0.1"_s,
                     i18n("Quickly prototype in QML"),
                     KAboutLicense::GPL_V3,
                     u"© 2024 Joshua Goins"_s);
    about.addAuthor(u"Joshua Goins"_s, u"Maintainer"_s, u"josh@redstrate.com"_s, u"https://redstrate.com/"_s, QUrl(u"https://redstrate.com/rss-image.png"_s));

    KAboutData::setApplicationData(about);

    MainWindow w;
    w.show();

    return app.exec();
}
