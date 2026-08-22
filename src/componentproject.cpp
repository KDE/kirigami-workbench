// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "componentproject.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace Qt::StringLiterals;

ComponentProject::ComponentProject(QObject *parent)
    : QObject(parent)
    , m_workingDir()
{
    initializeMainComponent();
}

ComponentProject::~ComponentProject() = default;

void ComponentProject::initializeMainComponent()
{
    if (!m_workingDir.isValid()) {
        return;
    }

    QString mainPath = m_workingDir.filePath(u"Main.qml"_s);
    QFile mainFile(mainPath);

    if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&mainFile);
        out << "import QtQuick\n"
            << "import QtQuick.Layouts\n"
            << "import QtQuick.Controls as Controls\n"
            << "import org.kde.kirigami as Kirigami\n"
            << "\n"
            << "Kirigami.ScrollablePage {\n"
            << "    title: \"My Page\"\n"
            << "\n"
            << "    Kirigami.PlaceholderMessage {\n"
            << "        anchors.centerIn: parent\n"
            << "        width: parent.width - Kirigami.Units.gridUnit * 4\n"
            << "        text: \"Start editing to see your component\"\n"
            << "        explanation: \"Edit Main.qml in the editor to build your Kirigami UI.\"\n"
            << "        icon.name: \"edit-symbolic\"\n"
            << "    }\n"
            << "}\n";
        mainFile.close();

        m_mainComponentPath = QFileInfo(mainPath).canonicalFilePath();
        if (m_mainComponentPath.isEmpty())
            m_mainComponentPath = mainPath;
    } else {
        qWarning() << "Failed to create main component file:" << mainPath;
    }
}

QString ComponentProject::mainComponentPath() const
{
    return m_mainComponentPath;
}

QStringList ComponentProject::subComponentPaths() const
{
    return m_subComponentPaths;
}

QString ComponentProject::addSubComponent(const QString &filename)
{
    if (!canAddFile()) {
        return QString();
    }

    QString qmlFilename = ensureQmlExtension(filename);
    QString filePath = m_workingDir.filePath(qmlFilename);

    if (QFile::exists(filePath)) {
        return QString();
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "import QtQuick\n\nItem {\n}\n";
        file.close();
        const QString canonical = QFileInfo(filePath).canonicalFilePath();
        m_subComponentPaths.append(canonical.isEmpty() ? filePath : canonical);
        Q_EMIT fileAdded(m_subComponentPaths.last());
        return m_subComponentPaths.last();
    }

    qWarning() << "Failed to create sub-component file:" << filePath;
    return QString();
}

bool ComponentProject::registerSubComponent(const QString &absolutePath)
{
    if (absolutePath.isEmpty() || !QFile::exists(absolutePath)) {
        return false;
    }
    const QString canonical = QFileInfo(absolutePath).canonicalFilePath();
    const QString key = canonical.isEmpty() ? absolutePath : canonical;
    if (key == m_mainComponentPath || m_subComponentPaths.contains(key))
        return false;
    m_subComponentPaths.append(key);
    Q_EMIT fileAdded(key);
    return true;
}

bool ComponentProject::removeSubComponent(const QString &path)
{
    if (!canRemove(path)) {
        qWarning() << "Cannot remove main component";
        return false;
    }
    if (!m_subComponentPaths.contains(path)) {
        qWarning() << "Sub-component not found:" << path;
        return false;
    }
    if (!QFile::remove(path)) {
        qWarning() << "Failed to delete file:" << path;
        return false;
    }
    m_subComponentPaths.removeAll(path);
    Q_EMIT fileRemoved(path);
    return true;
}

bool ComponentProject::canRemove(const QString &path) const
{
    return path != m_mainComponentPath;
}

QString ComponentProject::workingDirectory() const
{
    return m_workingDir.path();
}

bool ComponentProject::canAddFile() const
{
    return (1 + m_subComponentPaths.count()) < MAX_FILES;
}

QString ComponentProject::ensureQmlExtension(const QString &filename) const
{
    return filename.endsWith(u".qml"_s, Qt::CaseInsensitive) ? filename : filename + u".qml"_s;
}

void ComponentProject::reset()
{
    for (const QString &path : std::as_const(m_subComponentPaths)) {
        QFile::remove(path);
        Q_EMIT fileRemoved(path);
    }
    m_subComponentPaths.clear();
    initializeMainComponent();
}

#include "moc_componentproject.cpp"
