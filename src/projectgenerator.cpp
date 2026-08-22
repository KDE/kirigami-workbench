// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "projectgenerator.h"

#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KConfig>
#include <KConfigGroup>
#include <KFileUtils>
#include <KLocalizedString>
#include <KMacroExpander>
#include <KSharedConfig>
#include <KTar>
#include <KZip>

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUrl>

Q_LOGGING_CATEGORY(WORKBENCH_GENERATOR, "kirigami.workbench.generator")

using namespace Qt::StringLiterals;

static QStringList templateSearchDirs()
{
    return QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                     u"kdevappwizard/templates/"_s,
                                     QStandardPaths::LocateDirectory);
}

ProjectGenerator::ProjectGenerator(QObject *parent)
    : QObject(parent)
{
    loadSettings();
}

void ProjectGenerator::loadSettings()
{
    KConfigGroup grp = KSharedConfig::openConfig()->group(u"ProjectGenerator"_s);
    m_name = grp.readEntry("appName", u"MyApp"_s);
    m_version = grp.readEntry("appVersion", u"0.1"_s);
    m_location = grp.readEntry("location", QDir::homePath() + u"/src"_s);
    m_authorName = grp.readEntry("authorName", QString());
    m_authorEmail = grp.readEntry("authorEmail", QString());
}

void ProjectGenerator::saveSettings() const
{
    KConfigGroup grp = KSharedConfig::openConfig()->group(u"ProjectGenerator"_s);
    grp.writeEntry("appName", m_name);
    grp.writeEntry("appVersion", m_version);
    grp.writeEntry("location", m_location);
    grp.writeEntry("authorName", m_authorName);
    grp.writeEntry("authorEmail", m_authorEmail);
    grp.sync();
}

QString ProjectGenerator::name() const { return m_name; }
QString ProjectGenerator::location() const { return m_location; }
QString ProjectGenerator::authorName() const { return m_authorName; }
QString ProjectGenerator::authorEmail() const { return m_authorEmail; }
QString ProjectGenerator::version() const { return m_version; }

void ProjectGenerator::setName(const QString &name)
{
    if (m_name == name)
        return;
    m_name = name;
    Q_EMIT nameChanged();
}

void ProjectGenerator::setLocation(const QString &location)
{
    if (m_location == location)
        return;
    m_location = location;
    Q_EMIT locationChanged();
}

void ProjectGenerator::setAuthorName(const QString &authorName)
{
    if (m_authorName == authorName)
        return;
    m_authorName = authorName;
    Q_EMIT authorNameChanged();
}

void ProjectGenerator::setAuthorEmail(const QString &authorEmail)
{
    if (m_authorEmail == authorEmail)
        return;
    m_authorEmail = authorEmail;
    Q_EMIT authorEmailChanged();
}

void ProjectGenerator::setVersion(const QString &version)
{
    if (m_version == version)
        return;
    m_version = version;
    Q_EMIT versionChanged();
}

QStringList ProjectGenerator::availableTemplates()
{
    QStringList result;
    const QStringList paths = templateSearchDirs();
    for (const QString &path : paths) {
        const auto entries = QDir(path).entryInfoList(QDir::Files);
        for (const auto &entry : entries)
            result << entry.baseName();
    }
    return result;
}

bool ProjectGenerator::unpackArchive(const KArchiveDirectory *dir,
                                     const QString &dest,
                                     const QStringList &skipList,
                                     bool firstCall)
{
    qCDebug(WORKBENCH_GENERATOR) << "unpacking dir:" << dir->name() << "to" << dest;

    if (firstCall)
        m_newFiles.clear();

    if (!QDir::root().mkpath(dest)) {
        Q_EMIT errorOccurred(i18n("%1 cannot be created.", dest));
        return false;
    }

    bool failed = false;
    QTemporaryDir tdir;
    QMimeDatabase mimeDb;

    const QStringList entries = dir->entries();
    int progress = 0;

    for (const QString &entry : entries) {
        ++progress;
        Q_EMIT progressUpdated(int(double(progress) / entries.size() * 100));

        if (skipList.contains(entry))
            continue;

        if (dir->entry(entry)->isDirectory()) {
            const auto *subDir = static_cast<const KArchiveDirectory *>(dir->entry(entry));
            const QString newDest = dest + u"/"_s + subDir->name();
            if (!QFileInfo(newDest).exists()) {
                if (!QDir::root().mkdir(newDest)) {
                    Q_EMIT errorOccurred(i18n("Path %1 could not be created.", newDest));
                    failed = true;
                    break;
                }
                m_newFiles.append(newDest);
            }
            if (!unpackArchive(subDir, newDest, skipList, false)) {
                failed = true;
                break;
            }
        } else if (dir->entry(entry)->isFile()) {
            const auto *file = static_cast<const KArchiveFile *>(dir->entry(entry));
            file->copyTo(tdir.path());

            const QString destName = KMacroExpander::expandMacros(dest + u"/"_s + file->name(), m_variables);
            if (QFile::exists(destName)) {
                Q_EMIT errorOccurred(i18n("Could not copy template file to %1, because the file already exists. "
                                          "The project has not been generated and all temporary files will be removed.",
                                          destName));
                failed = true;
                break;
            }

            if (QFile(QDir::cleanPath(tdir.path() + u"/"_s + file->name())).copy(destName)) {
                m_newFiles.append(destName);
                const auto mimeType = mimeDb.mimeTypeForFile(destName);
                if (mimeType.inherits(u"text/plain"_s) && !extractFileMacros(destName)) {
                    Q_EMIT errorOccurred(i18n("Failed to integrate your project information into the file %1. "
                                              "The project has not been generated and all temporary files will be removed.",
                                              destName));
                    failed = true;
                    break;
                }
            } else {
                Q_EMIT errorOccurred(i18n("Could not copy template file to %1.", destName));
                failed = true;
                break;
            }
        }
    }

    if (failed) {
        for (int i = m_newFiles.size() - 1; i >= 0; --i) {
            QFileInfo fi(m_newFiles[i]);
            if (fi.isDir())
                QDir().rmdir(m_newFiles[i]);
            else
                QFile::remove(m_newFiles[i]);
            m_newFiles.removeAt(i);
        }
        return false;
    }

    return true;
}

bool ProjectGenerator::extractFileMacros(const QString &entry)
{
    QFile file(entry);
    if (!file.exists() || !file.open(QFile::ReadOnly))
        return false;

    const QString input = QTextStream(&file).readAll();
    file.close();

    const QString text = KMacroExpander::expandMacros(input, m_variables);
    if (text == input)
        return true;

    if (!file.open(QFile::WriteOnly | QIODevice::Truncate))
        return false;

    QTextStream(&file) << text;
    return true;
}

void ProjectGenerator::startGeneration(const QString &templateName, const QString &displayName)
{
    saveSettings();

    const QStringList templatePaths = templateSearchDirs();
    const QStringList results = KFileUtils::findAllUniqueFiles(templatePaths, {templateName + u".*"_s});

    if (results.isEmpty()) {
        Q_EMIT errorOccurred(i18n("Could not load template archive for \"%1\". "
                                  "Make sure kapptemplate is installed.",
                                  displayName));
        return;
    }

    const QString archName = results.first();

    const QString appName = m_name;
    QUrl projectUrl = QUrl::fromLocalFile(m_location).adjusted(QUrl::StripTrailingSlash);
    projectUrl.setPath(projectUrl.path() + u"/"_s + appName.toLower());
    const QString dest = projectUrl.toLocalFile();
    const QString baseUrl = QUrl::fromLocalFile(m_location).toLocalFile();

    const QRegularExpression nonWord(u"[^\\w]"_s);
    auto makeIdentifier = [&nonWord](const QString &s) {
        return QString(s).replace(nonWord, u"_"_s);
    };

    m_variables.clear();
    m_variables[u"CURRENT_YEAR"_s] = QString::number(QDate::currentDate().year());
    m_variables[u"APPNAME"_s] = appName;
    m_variables[u"APPNAMEUC"_s] = makeIdentifier(appName.toUpper());
    m_variables[u"APPNAMELC"_s] = appName.toLower();
    m_variables[u"APPNAMEID"_s] = makeIdentifier(appName);
    m_variables[u"PROJECTDIR"_s] = dest;
    m_variables[u"PROJECTDIRNAME"_s] = projectUrl.fileName();
    m_variables[u"AUTHOR"_s] = m_authorName;
    m_variables[u"EMAIL"_s] = m_authorEmail;
    m_variables[u"VERSION"_s] = m_version;
    m_variables[u"dest"_s] = dest;
    m_variables[u"APPNAMEFU"_s] = QString(appName).replace(0, 1, appName.toUpper().at(0));
    m_variables[u"VERSIONCONTROLPLUGIN"_s] = QString();

    std::unique_ptr<KArchive> arch;
    if (archName.endsWith(u".zip"_s))
        arch = std::make_unique<KZip>(archName);
    else
        arch = std::make_unique<KTar>(archName, u"application/x-bzip"_s);

    if (!arch->open(QIODevice::ReadOnly)) {
        Q_EMIT errorOccurred(i18n("Could not open template archive \"%1\".", archName));
        return;
    }

    QStringList metaSkip;
    const KArchiveEntry *templateEntry = arch->directory()->entry(templateName + u".kdevtemplate"_s);
    if (!templateEntry || !templateEntry->isFile()) {
        for (const QString &entry : arch->directory()->entries()) {
            if (entry.endsWith(u".kdevtemplate"_s)) {
                templateEntry = arch->directory()->entry(entry);
                break;
            }
        }
    }
    if (templateEntry && templateEntry->isFile()) {
        metaSkip << templateEntry->name();
        const auto *templateFile = static_cast<const KArchiveFile *>(templateEntry);
        QTemporaryDir tmpDir;
        templateFile->copyTo(tmpDir.path());
        KConfig config(tmpDir.path() + u"/"_s + templateEntry->name());
        const QString iconEntry = KConfigGroup(&config, u"General"_s).readEntry("Icon");
        if (!iconEntry.isEmpty()) {
            if (const KArchiveEntry *ie = arch->directory()->entry(iconEntry); ie && ie->isFile())
                metaSkip << iconEntry;
        }
    }

    if (!QFileInfo(dest).exists())
        QDir::root().mkdir(dest);

    if (!unpackArchive(arch->directory(), dest, metaSkip)) {
        return;
    }

    const QString summary = i18n("Your project <b>%1</b> was created based on the <b>%2</b> template.<br />"
                                  "Version: %3<br /><br />"
                                  "Location: %4<br /><br />"
                                  "Open the project folder to get started.",
                                  appName, displayName, m_version, dest);
    Q_EMIT done(summary);
}
