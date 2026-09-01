#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

import os
import sys

os.environ.setdefault("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop")

try:
    from PySide6.QtGui import QGuiApplication
    from PySide6.QtQml import QQmlApplicationEngine
    from PySide6.QtCore import QUrl, QCoreApplication
except ImportError:
    print("PySide6 is required. Install it with: pip install PySide6", file=sys.stderr)
    sys.exit(1)

try:
    import workbench_bridge
except ImportError:
    workbench_bridge = None

QML_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "main.qml")


def main():
    # Tell workbench to render main.qml (no-op when running standalone)
    if workbench_bridge is not None:
        workbench_bridge.load(QML_FILE)
        return  # bridge owns the event loop when running in workbench

    # Standalone path
    app = QCoreApplication.instance() or QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()
    engine.load(QUrl.fromLocalFile(QML_FILE))

    if not engine.rootObjects():
        print("Failed to load QML", file=sys.stderr)
        sys.exit(1)

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
