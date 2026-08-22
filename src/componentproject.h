// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QTemporaryDir>

class ComponentProject : public QObject
{
    Q_OBJECT

public:
    static constexpr int MAX_FILES = 100;

    explicit ComponentProject(QObject *parent = nullptr);
    ~ComponentProject() override;

    QString mainComponentPath() const;
    QStringList subComponentPaths() const;

    QString addSubComponent(const QString &filename);
    bool registerSubComponent(const QString &absolutePath);
    bool removeSubComponent(const QString &path);
    bool canRemove(const QString &path) const;

    QString workingDirectory() const;
    bool canAddFile() const;
    void reset();

Q_SIGNALS:
    void fileAdded(const QString &path);
    void fileRemoved(const QString &path);

private:
    void initializeMainComponent();
    QString ensureQmlExtension(const QString &filename) const;

    QTemporaryDir m_workingDir;
    QString m_mainComponentPath;
    QStringList m_subComponentPaths;
};
