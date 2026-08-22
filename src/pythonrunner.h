// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QProcess>
#include <QString>

class PythonRunner : public QObject
{
    Q_OBJECT

public:
    explicit PythonRunner(QObject *parent = nullptr);
    ~PythonRunner() override;

    void start(const QString &userScript);
    void stop();
    bool isRunning() const;

    QLocalSocket *socket() const { return m_socket; }

    static QString findPython();

Q_SIGNALS:
    void started();
    void stopped(int exitCode);
    void bridgeConnected();
    void bridgeDisconnected();

    void outputLine(const QString &line);
    void errorLine(const QString &line);

    void error(const QString &message);

private Q_SLOTS:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError err);
    void onStdout();
    void onStderr();
    void onNewConnection();

private:
    QString bridgeScriptPath() const;
    QString uniqueSocketName() const;

    QProcess      *m_process = nullptr;
    QLocalServer  *m_server  = nullptr;
    QLocalSocket  *m_socket  = nullptr;
    QString        m_socketName;
};
