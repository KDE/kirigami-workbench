// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStandardItemModel>

class ExampleModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum AdditionalRoles {
        DescriptionRole = Qt::UserRole + 1,
        CategoryRole,
        TagsRole,
        PluginIdRole,
        IconNameRole,
        PackagePathRole = Qt::UserRole + 10,
    };
    Q_ENUM(AdditionalRoles)

    explicit ExampleModel(QObject *parent = nullptr);

    Q_INVOKABLE void reload();

    QHash<int, QByteArray> roleNames() const override;
};
