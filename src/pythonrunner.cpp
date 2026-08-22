// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pythonrunner.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QUuid>

using namespace Qt::StringLiterals;

PythonRunner::PythonRunner(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &PythonRunner::onNewConnection);
}

PythonRunner::~PythonRunner()
{
    stop();
}

void PythonRunner::start(const QString &userScript)
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->setReadChannel(QProcess::StandardOutput);
        while (m_process->canReadLine())
            Q_EMIT outputLine(QString::fromUtf8(m_process->readLine()).trimmed());
        m_process->setReadChannel(QProcess::StandardError);
        while (m_process->canReadLine())
            Q_EMIT errorLine(QString::fromUtf8(m_process->readLine()).trimmed());

        disconnect(m_process, nullptr, this, nullptr);
        m_process->kill();
        m_process->waitForFinished(1000);
    }

    if (m_socket) {
        disconnect(m_socket, nullptr, this, nullptr);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    if (m_server->isListening()) {
        m_server->close();
        QLocalServer::removeServer(m_socketName);
    }

    const QString python = findPython();
    if (python.isEmpty()) {
        Q_EMIT error(i18n("Python interpreter not found. "
                          "Install Python 3 and make sure it is on PATH."));
        return;
    }

    const QString bridge = bridgeScriptPath();
    if (!QFileInfo::exists(bridge)) {
        Q_EMIT error(i18n("workbench_bridge.py not found at: %1", bridge));
        return;
    }

    m_socketName = uniqueSocketName();
    QLocalServer::removeServer(m_socketName);
    if (!m_server->listen(m_socketName)) {
        Q_EMIT error(i18n("Cannot open local server: %1", m_server->errorString()));
        return;
    }

    {
        QProcess check;
        check.start(python, { u"-c"_s,
            u"import importlib.util\n"
            "found = importlib.util.find_spec('PySide6') or importlib.util.find_spec('PyQt6')\n"
            "raise SystemExit(0 if found else 1)"_s });
        check.waitForFinished(5000);
        if (check.exitCode() != 0) {
            Q_EMIT error(
                i18n("PySide6 or PyQt6 is required for Python preview.\n\n"
                     "Install it with:\n"
                     "    pip install PySide6\n\n"
                     "On Linux you can also use your package manager:\n"
                     "    sudo apt install python3-pyside6   # Debian/Ubuntu\n"
                     "    sudo dnf install python3-pyside6   # Fedora\n"
                     "    sudo pacman -S pyside6             # Arch"));
            m_server->close();
            QLocalServer::removeServer(m_socketName);
            return;
        }
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_WIN
    {
        QProcess probe;
        probe.start(python, { u"-c"_s,
            u"import PySide6, os; print(os.path.dirname(PySide6.__file__))"_s });
        if (probe.waitForFinished(5000)) {
            const QString pyside6Dir = QString::fromUtf8(probe.readAllStandardOutput()).trimmed();

            const QString exeDir = QCoreApplication::applicationDirPath();
            const QString exeDirNative = QDir::toNativeSeparators(exeDir);
            QStringList pathDirs = env.value(u"PATH"_s).split(u";"_s, Qt::SkipEmptyParts);
            pathDirs.erase(std::remove_if(pathDirs.begin(), pathDirs.end(),
                [&exeDirNative](const QString &d) {
                    return QDir::toNativeSeparators(d).compare(exeDirNative, Qt::CaseInsensitive) == 0;
                }), pathDirs.end());
            if (!pyside6Dir.isEmpty())
                pathDirs.prepend(pyside6Dir);
            env.insert(u"PATH"_s, pathDirs.join(u";"_s));
        }
    }
#else
    {
        QStringList qmlPaths;
        const QString qtQmlDir = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
        if (!qtQmlDir.isEmpty() && QDir(qtQmlDir).exists())
            qmlPaths << qtQmlDir;

        const QString existing = env.value(u"QML2_IMPORT_PATH"_s);
        if (!existing.isEmpty())
            qmlPaths << existing;

        if (!qmlPaths.isEmpty())
            env.insert(u"QML2_IMPORT_PATH"_s, qmlPaths.join(u":"_s));

        if (!qtQmlDir.isEmpty())
            env.insert(u"QT_INSTALL_QML"_s, qtQmlDir);
    }
#endif
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::finished,                this, &PythonRunner::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,           this, &PythonRunner::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &PythonRunner::onStdout);
    connect(m_process, &QProcess::readyReadStandardError,  this, &PythonRunner::onStderr);

    const QStringList args = { u"-u"_s, bridge, m_socketName, userScript };
    m_process->start(python, args);

    if (!m_process->waitForStarted(5000)) {
        Q_EMIT error(i18n("Failed to start Python process: %1", m_process->errorString()));
        m_process->deleteLater();
        m_process = nullptr;
        m_server->close();
        return;
    }

    Q_EMIT started();
}

void PythonRunner::stop()
{
    if (m_socket) {
        disconnect(m_socket, nullptr, this, nullptr);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_process) {
        m_process->setReadChannel(QProcess::StandardOutput);
        while (m_process->canReadLine())
            Q_EMIT outputLine(QString::fromUtf8(m_process->readLine()).trimmed());
        m_process->setReadChannel(QProcess::StandardError);
        while (m_process->canReadLine())
            Q_EMIT errorLine(QString::fromUtf8(m_process->readLine()).trimmed());

        disconnect(m_process, nullptr, this, nullptr);
        m_process->kill();
        m_process->waitForFinished(1000);
        m_process->deleteLater();
        m_process = nullptr;
    }
    if (m_server->isListening()) {
        m_server->close();
        QLocalServer::removeServer(m_socketName);
    }
}

bool PythonRunner::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

QString PythonRunner::findPython()
{
#ifdef Q_OS_WIN
    const QStringList names = { u"python3.exe"_s, u"python.exe"_s };
#else
    const QStringList names = { u"python3"_s, u"python"_s };
#endif

    QDir dir(QCoreApplication::applicationDirPath());
    for (int level = 0; level < 8; ++level) {
        for (const QString &name : names) {
            const QString candidate = dir.filePath(name);
            if (QFileInfo::exists(candidate))
                return candidate;
        }
        if (level > 0) {
            for (const QString &name : names) {
                const QString candidate = dir.filePath(u"bin/"_s + name);
                if (QFileInfo::exists(candidate))
                    return candidate;
            }
        }
        if (!dir.cdUp())
            break;
    }

    for (const QString &name : names) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty())
            return path;
    }

#ifndef Q_OS_WIN
    const QStringList systemDirs = { u"/usr/bin"_s, u"/usr/local/bin"_s, u"/opt/homebrew/bin"_s };
    for (const QString &dir2 : systemDirs) {
        for (const QString &name : names) {
            const QString candidate = dir2 + u"/"_s + name;
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }
#endif

    return {};
}

QString PythonRunner::bridgeScriptPath() const
{
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString candidate1 = exeDir + u"/data/kirigami-workbench/workbench_bridge.py"_s;
    if (QFileInfo::exists(candidate1))
        return candidate1;

    const QString located = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        u"kirigami-workbench/workbench_bridge.py"_s);
    if (!located.isEmpty())
        return located;

    const QString candidate3 = exeDir + u"/workbench_bridge.py"_s;
    if (QFileInfo::exists(candidate3))
        return candidate3;

    return {};
}

QString PythonRunner::uniqueSocketName() const
{
    return u"wb-bridge-"_s + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

void PythonRunner::onProcessFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    if (m_process) {
        m_process->setReadChannel(QProcess::StandardOutput);
        while (m_process->canReadLine())
            Q_EMIT outputLine(QString::fromUtf8(m_process->readLine()).trimmed());
        m_process->setReadChannel(QProcess::StandardError);
        while (m_process->canReadLine()) {
            const QString line = QString::fromUtf8(m_process->readLine()).trimmed();
            if (!line.isEmpty())
                Q_EMIT errorLine(line);
        }
    }
    Q_EMIT stopped(exitCode);
}

void PythonRunner::onProcessError(QProcess::ProcessError err)
{
    Q_UNUSED(err)
    Q_EMIT error(i18n("Python process error: %1", m_process ? m_process->errorString() : QString()));
}

void PythonRunner::onStdout()
{
    if (!m_process)
        return;
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        const QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        if (!line.isEmpty())
            Q_EMIT outputLine(line);
    }
}

void PythonRunner::onStderr()
{
    if (!m_process)
        return;
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        const QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        if (!line.isEmpty())
            Q_EMIT errorLine(line);
    }
}

void PythonRunner::onNewConnection()
{
    QLocalSocket *sock = m_server->nextPendingConnection();
    if (!sock)
        return;

    if (m_socket) {
        sock->disconnectFromServer();
        sock->deleteLater();
        return;
    }

    m_socket = sock;
    m_socket->setParent(this);
    Q_EMIT bridgeConnected();

    connect(m_socket, &QLocalSocket::disconnected, this, [this]() {
        m_socket = nullptr;
        Q_EMIT bridgeDisconnected();
    });
}

#include "moc_pythonrunner.cpp"
