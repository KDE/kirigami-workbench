// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>

class KArchiveDirectory;

/**
 * Adapted from KAppTemplate's Generator. Scaffolds a KDE project from a
 * kdevappwizard template archive, substituting project macros throughout.
 *
 * Dependencies on KAppTemplate-internal Prefs/logging have been replaced
 * with KSharedConfig and a local logging category.
 */
class ProjectGenerator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString location READ location WRITE setLocation NOTIFY locationChanged)
    Q_PROPERTY(QString authorName READ authorName WRITE setAuthorName NOTIFY authorNameChanged)
    Q_PROPERTY(QString authorEmail READ authorEmail WRITE setAuthorEmail NOTIFY authorEmailChanged)
    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)

public:
    explicit ProjectGenerator(QObject *parent = nullptr);

    QString name() const;
    QString location() const;
    QString authorName() const;
    QString authorEmail() const;
    QString version() const;

    void setName(const QString &name);
    void setLocation(const QString &location);
    void setAuthorName(const QString &authorName);
    void setAuthorEmail(const QString &authorEmail);
    void setVersion(const QString &version);

    static QStringList availableTemplates();

    /**
     * Start scaffolding. Emits progressUpdated(), then either
     * errorOccurred() or done() when finished.
     *
     * @param templateName  basename of the archive (e.g. "kde-frameworks6-simple")
     * @param displayName   human-readable name for the summary message
     */
    void startGeneration(const QString &templateName, const QString &displayName);

Q_SIGNALS:
    void nameChanged();
    void locationChanged();
    void authorNameChanged();
    void authorEmailChanged();
    void versionChanged();

    void errorOccurred(const QString &errorMessage);
    void progressUpdated(int progress);
    void done(const QString &summary);

private:
    bool unpackArchive(const KArchiveDirectory *dir, const QString &dest, const QStringList &skipList = {}, bool firstCall = true);
    bool extractFileMacros(const QString &entry);

    void saveSettings() const;
    void loadSettings();

    QHash<QString, QString> m_variables;
    QString m_name;
    QString m_location;
    QString m_authorName;
    QString m_authorEmail;
    QString m_version;
    QStringList m_newFiles;
};
