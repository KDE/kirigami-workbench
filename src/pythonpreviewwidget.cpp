// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pythonpreviewwidget.h"
#include "wbprotocol.h"

#include <KLocalizedString>

#include <QDataStream>
#include <QKeyEvent>
#include <QLocalSocket>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

using namespace Qt::StringLiterals;

PythonPreviewWidget::PythonPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_placeholderText = i18n("Press \u25B6 Run (Ctrl+F5) to start the Python app");
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void PythonPreviewWidget::attachRunner(PythonRunner *runner)
{
    detach();

    m_runner = runner;
    m_socket = runner->socket();
    m_state = State::Running;
    m_frame = QImage();

    if (m_socket) {
        connect(m_socket, &QLocalSocket::readyRead, this, &PythonPreviewWidget::onReadyRead);
        sendResize();
    }
    update();
}

void PythonPreviewWidget::detach()
{
    if (m_socket) {
        disconnect(m_socket, nullptr, this, nullptr);
        m_socket = nullptr;
    }
    m_runner = nullptr;
    m_frame = QImage();
    m_readBuffer.clear();
    m_state = State::Idle;
    update();
}

void PythonPreviewWidget::showPlaceholder(const QString &text)
{
    m_placeholderText = text;
    m_state = State::Idle;
    update();
}

void PythonPreviewWidget::showError(const QString &message)
{
    m_errorText = message;
    m_state = State::Error;
    update();
}

void PythonPreviewWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0xFF, 0xFF, 0xFF));

    if (m_state == State::Running && !m_frame.isNull()) {
        const QImage scaled = m_frame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint offset((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
        p.drawImage(offset, scaled);
        return;
    }

    const bool isError = (m_state == State::Error);

    p.fillRect(rect(), isError ? QColor(0x1a, 0x00, 0x00) : QColor(0xFF, 0xFF, 0xFF));

    if (isError) {
        const QString &text = m_errorText;
        const int margin = 20;
        const QRect textRect = rect().adjusted(margin, margin, -margin, -margin);

        QFont boldFont = font();
        boldFont.setBold(true);
        boldFont.setPointSizeF(boldFont.pointSizeF() * 1.1);
        p.setFont(boldFont);
        p.setPen(QColor(0xFF, 0x60, 0x60));
        const QRect titleRect(textRect.left(), textRect.top(), textRect.width(), QFontMetrics(boldFont).height() + 8);
        p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, i18nc("@label", "Python Error"));

        p.setFont(font());
        p.setPen(QColor(0xFF, 0xCC, 0xCC));
        p.drawText(textRect.adjusted(0, titleRect.height() + 4, 0, 0), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);

    } else if (!m_placeholderText.isEmpty()) {
        const int margin = 20;
        p.setPen(QColor(0x80, 0x80, 0x80));
        p.drawText(rect().adjusted(margin, margin, -margin, -margin), Qt::AlignCenter | Qt::TextWordWrap, m_placeholderText);
    }
}

void PythonPreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_state == State::Running)
        sendResize();
}

void PythonPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    sendMessage(WbProtocol::makeMouseEvent(WbProtocol::MSG_MOUSE_PRESS, event->position().x(), event->position().y(), static_cast<int>(event->button())));
}

void PythonPreviewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    sendMessage(WbProtocol::makeMouseEvent(WbProtocol::MSG_MOUSE_RELEASE, event->position().x(), event->position().y(), static_cast<int>(event->button())));
}

void PythonPreviewWidget::mouseMoveEvent(QMouseEvent *event)
{
    sendMessage(WbProtocol::makeMouseEvent(WbProtocol::MSG_MOUSE_MOVE, event->position().x(), event->position().y()));
}

void PythonPreviewWidget::keyPressEvent(QKeyEvent *event)
{
    sendMessage(WbProtocol::makeKeyEvent(WbProtocol::MSG_KEY_PRESS, event->key(), static_cast<int>(event->modifiers()), event->text()));
}

void PythonPreviewWidget::keyReleaseEvent(QKeyEvent *event)
{
    sendMessage(WbProtocol::makeKeyEvent(WbProtocol::MSG_KEY_RELEASE, event->key(), static_cast<int>(event->modifiers()), event->text()));
}

void PythonPreviewWidget::sendMessage(const QByteArray &msg)
{
    if (m_socket && m_socket->isValid())
        m_socket->write(msg);
}

void PythonPreviewWidget::sendResize()
{
    sendMessage(WbProtocol::makeResize(width(), height()));
}

void PythonPreviewWidget::onReadyRead()
{
    if (!m_socket)
        return;
    m_readBuffer.append(m_socket->readAll());
    processIncoming();
}

void PythonPreviewWidget::processIncoming()
{
    while (m_readBuffer.size() >= WbProtocol::HEADER_SIZE) {
        QDataStream hdr(m_readBuffer);
        hdr.setByteOrder(QDataStream::BigEndian);
        quint32 type, payloadLen;
        hdr >> type >> payloadLen;

        const int totalLen = WbProtocol::HEADER_SIZE + static_cast<int>(payloadLen);
        if (m_readBuffer.size() < totalLen)
            break;

        const QByteArray payload = m_readBuffer.mid(WbProtocol::HEADER_SIZE, static_cast<int>(payloadLen));
        m_readBuffer.remove(0, totalLen);

        handleMessage(type, payload);
    }
}

void PythonPreviewWidget::handleMessage(quint32 type, const QByteArray &payload)
{
    switch (type) {
    case WbProtocol::MSG_READY:
        m_state = State::Running;
        sendResize();
        update();
        break;

    case WbProtocol::MSG_FRAME: {
        const auto f = WbProtocol::parseFrame(payload);
        if (f.data.isEmpty())
            break;

        if (f.format == WbProtocol::FMT_RGBA8888) {
            m_frame = QImage(reinterpret_cast<const uchar *>(f.data.constData()), f.width, f.height, f.width * 4, QImage::Format_RGBA8888).copy();
        } else if (f.format == WbProtocol::FMT_JPEG) {
            m_frame.loadFromData(f.data, "JPEG");
        }

        m_state = State::Running;
        update();
        break;
    }

    case WbProtocol::MSG_ERROR:
        m_errorText = QString::fromUtf8(payload);
        m_state = State::Error;
        update();
        break;

    default:
        break;
    }
}

#include "moc_pythonpreviewwidget.cpp"
