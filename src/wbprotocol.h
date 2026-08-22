// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

/**
 * Binary framing protocol between Kirigami Workbench (C++) and the Python
 * bridge process (workbench_bridge.py).
 *
 * Each message:
 *   [quint32 type] [quint32 payloadLen] [payloadLen bytes payload]
 *
 * All integers are big-endian (QDataStream default).
 */
namespace WbProtocol
{

constexpr quint32 MSG_READY         = 0x01;
constexpr quint32 MSG_FRAME         = 0x02;
constexpr quint32 MSG_ERROR         = 0x03;
constexpr quint32 MSG_STDOUT        = 0x04;
constexpr quint32 MSG_STDERR        = 0x05;

constexpr quint32 MSG_RESIZE        = 0x10;
constexpr quint32 MSG_MOUSE_PRESS   = 0x11;
constexpr quint32 MSG_MOUSE_RELEASE = 0x12;
constexpr quint32 MSG_MOUSE_MOVE    = 0x13;
constexpr quint32 MSG_KEY_PRESS     = 0x14;
constexpr quint32 MSG_KEY_RELEASE   = 0x15;
constexpr quint32 MSG_STOP          = 0x16;

constexpr quint8 FMT_RGBA8888 = 0x01;
constexpr quint8 FMT_JPEG     = 0x02;

constexpr int HEADER_SIZE = 8;

inline QByteArray makeMessage(quint32 type, const QByteArray &payload = {})
{
    QByteArray msg;
    msg.resize(HEADER_SIZE + payload.size());
    QDataStream ds(&msg, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << type << static_cast<quint32>(payload.size());
    if (!payload.isEmpty())
        ds.writeRawData(payload.constData(), payload.size());
    return msg;
}

inline QByteArray makeResize(int w, int h)
{
    QByteArray pl;
    QDataStream ds(&pl, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<qint32>(w) << static_cast<qint32>(h);
    return makeMessage(MSG_RESIZE, pl);
}

inline QByteArray makeMouseEvent(quint32 type, int x, int y, int button = 0)
{
    QByteArray pl;
    QDataStream ds(&pl, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<qint32>(x) << static_cast<qint32>(y) << static_cast<qint32>(button);
    return makeMessage(type, pl);
}

inline QByteArray makeKeyEvent(quint32 type, int key, int modifiers, const QString &text)
{
    QByteArray pl;
    QDataStream ds(&pl, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    const QByteArray utf8 = text.toUtf8();
    ds << static_cast<qint32>(key) << static_cast<qint32>(modifiers)
       << static_cast<quint32>(utf8.size());
    pl.append(utf8);
    return makeMessage(type, pl);
}

struct FramePayload {
    int width = 0;
    int height = 0;
    quint8 format = FMT_RGBA8888;
    QByteArray data;
};

inline FramePayload parseFrame(const QByteArray &payload)
{
    FramePayload f;
    if (payload.size() < 9)
        return f;
    QDataStream ds(payload);
    ds.setByteOrder(QDataStream::BigEndian);
    qint32 w, h;
    quint8 fmt;
    quint32 dataLen;
    ds >> w >> h >> fmt >> dataLen;
    f.width  = w;
    f.height = h;
    f.format = fmt;
    if (static_cast<int>(dataLen) <= payload.size() - 13)
        f.data = payload.mid(13, static_cast<int>(dataLen));
    return f;
}

} // namespace WbProtocol
