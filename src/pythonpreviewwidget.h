// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pythonrunner.h"

#include <QByteArray>
#include <QImage>
#include <QWidget>

class QLocalSocket;
class QLabel;
class QVBoxLayout;

/**
 * Preview widget for Python+Kirigami apps.
 *
 * Connects to a PythonRunner's QLocalSocket and:
 *  - Receives rendered frames and paints them
 *  - Forwards mouse and keyboard events to the Python process
 *  - Forwards resize events so the offscreen surface matches
 *  - Shows a placeholder when idle and an error overlay on failure
 */
class PythonPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PythonPreviewWidget(QWidget *parent = nullptr);

    void attachRunner(PythonRunner *runner);
    void detach();

    void showPlaceholder(const QString &text);
    void showError(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private Q_SLOTS:
    void onReadyRead();

private:
    void sendMessage(const QByteArray &msg);
    void processIncoming();
    void handleMessage(quint32 type, const QByteArray &payload);
    void sendResize();

    PythonRunner *m_runner = nullptr;
    QLocalSocket *m_socket = nullptr;

    QImage m_frame;
    QByteArray m_readBuffer;

    QString m_placeholderText;
    QString m_errorText;

    enum class State {
        Idle,
        Running,
        Error
    };
    State m_state = State::Idle;
};
