// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "examplemodel.h"

#include <KPackage/PackageLoader>
#include <KPluginMetaData>
#include <QCoreApplication>
#include <QDir>

using namespace Qt::StringLiterals;

ExampleModel::ExampleModel(QObject *parent)
    : QStandardItemModel(parent)
{
    const QString buildPluginPath = QCoreApplication::applicationDirPath() + u"/../kf6/packagestructure"_s;
    if (QDir(buildPluginPath).exists()) {
        QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() + u"/../kf6/packagestructure"_s);
    }

    reload();
}

void ExampleModel::reload()
{
    clear();

    const auto list = KPackage::PackageLoader::self()->listPackages(u"KirigamiWorkbench/Example"_s);

    for (const auto &plugin : list) {
        auto *item = new QStandardItem(plugin.name());
        item->setData(plugin.description(), DescriptionRole);
        item->setData(plugin.category(), CategoryRole);
        item->setData(plugin.value(u"Tags"_s, QStringList{}), TagsRole);
        item->setData(plugin.pluginId(), PluginIdRole);
        item->setData(plugin.iconName(), IconNameRole);
        appendRow(item);
    }

    if (rowCount() == 0) {
        const QStringList candidateDirs = {
            QCoreApplication::applicationDirPath() + u"/../../examples"_s,
            QCoreApplication::applicationDirPath() + u"/../../../examples"_s,
            QCoreApplication::applicationDirPath() + u"/../../../../examples"_s,
            QCoreApplication::applicationDirPath() + u"/../../../../../examples"_s,
        };
        for (const QString &dir : candidateDirs) {
            QDir examplesDir(dir);
            if (!examplesDir.exists())
                continue;
            for (const auto &entry : examplesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                const QString metaPath = entry.filePath() + u"/metadata.json"_s;
                if (!QFile::exists(metaPath))
                    continue;
                KPluginMetaData plugin = KPluginMetaData::fromJsonFile(metaPath);
                if (!plugin.isValid())
                    continue;
                auto *item = new QStandardItem(plugin.name());
                item->setData(plugin.description(), DescriptionRole);
                item->setData(plugin.category(), CategoryRole);
                item->setData(plugin.value(u"Tags"_s, QStringList{}), TagsRole);
                item->setData(plugin.pluginId(), PluginIdRole);
                item->setData(plugin.iconName(), IconNameRole);
                item->setData(entry.absoluteFilePath(), PackagePathRole);
                appendRow(item);
            }
            if (rowCount() > 0)
                break;
        }
    }
}

QHash<int, QByteArray> ExampleModel::roleNames() const
{
    QHash<int, QByteArray> roles = QStandardItemModel::roleNames();
    roles[DescriptionRole] = "description";
    roles[CategoryRole] = "category";
    roles[TagsRole] = "tags";
    roles[PluginIdRole] = "pluginId";
    roles[IconNameRole] = "iconName";
    return roles;
}

#include "moc_examplemodel.cpp"
