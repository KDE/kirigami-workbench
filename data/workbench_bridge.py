# SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import importlib.util
import os
import struct
import sys
import traceback

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

try:
    from PySide6.QtCore import (
        QByteArray, QCoreApplication, QObject, QSize, QTimer, Qt, QUrl,
    )
    from PySide6.QtGui import (
        QGuiApplication, QImage, QKeyEvent, QMouseEvent,
        QOffscreenSurface, QOpenGLContext, QSurfaceFormat,
    )
    from PySide6.QtNetwork import QLocalSocket
    from PySide6.QtOpenGL import QOpenGLFramebufferObject, QOpenGLFramebufferObjectFormat
    from PySide6.QtQml import QQmlComponent, QQmlEngine
    from PySide6.QtQuick import QQuickItem, QQuickRenderControl, QQuickRenderTarget, QQuickWindow
    _BINDING = "pyside6"
except ImportError:
    try:
        from PyQt6.QtCore import (
            QByteArray, QCoreApplication, QObject, QSize, QTimer, Qt, QUrl,
        )
        from PyQt6.QtGui import (
            QGuiApplication, QImage, QKeyEvent, QMouseEvent,
            QOffscreenSurface, QOpenGLContext, QSurfaceFormat,
        )
        from PyQt6.QtNetwork import QLocalSocket
        from PyQt6.QtOpenGL import QOpenGLFramebufferObject, QOpenGLFramebufferObjectFormat
        from PyQt6.QtQml import QQmlComponent, QQmlEngine
        from PyQt6.QtQuick import QQuickItem, QQuickRenderControl, QQuickRenderTarget, QQuickWindow
        _BINDING = "pyqt6"
    except ImportError as e:
        print(f"workbench_bridge: neither PySide6 nor PyQt6 found: {e}", file=sys.__stderr__)
        sys.exit(1)

# ---------------------------------------------------------------------------
# Protocol constants (mirror wbprotocol.h)
# ---------------------------------------------------------------------------
MSG_READY         = 0x01
MSG_FRAME         = 0x02
MSG_ERROR         = 0x03
MSG_STDOUT        = 0x04
MSG_STDERR        = 0x05

MSG_RESIZE        = 0x10
MSG_MOUSE_PRESS   = 0x11
MSG_MOUSE_RELEASE = 0x12
MSG_MOUSE_MOVE    = 0x13
MSG_KEY_PRESS     = 0x14
MSG_KEY_RELEASE   = 0x15
MSG_STOP          = 0x16

FMT_RGBA8888 = 0x01

HEADER_FMT = ">II"
HEADER_SIZE = struct.calcsize(HEADER_FMT)

# ---------------------------------------------------------------------------
# Public API — called from user's app.py
# ---------------------------------------------------------------------------

_bridge: "_Bridge | None" = None
_running_in_workbench = False  


def load(qml_file: str) -> None:
    """
    Load a QML file into the workbench preview.
    When running standalone (not under workbench), this is a no-op — the
    user's own QQmlApplicationEngine handles loading.
    """
    if _bridge is not None and _running_in_workbench:
        _bridge.load_qml_file(qml_file)


def load_string(qml_text: str, base_url: str = "") -> None:
    """Load QML from a string into the workbench preview."""
    if _bridge is not None and _running_in_workbench:
        _bridge.load_qml_string(qml_text, base_url)


def register(engine) -> None:  # type: ignore[no-untyped-def]
    """Legacy API: pass the engine; we extract the QML source URL from it."""
    if _bridge is None or not _running_in_workbench:
        return
    QTimer.singleShot(100, lambda: _bridge._try_load_from_engine(engine))


# ---------------------------------------------------------------------------
# Framing helpers
# ---------------------------------------------------------------------------

def _pack_message(msg_type: int, payload: bytes = b"") -> bytes:
    return struct.pack(HEADER_FMT, msg_type, len(payload)) + payload


def _pack_frame(width: int, height: int, data: bytes) -> bytes:
    payload = struct.pack(">iiBI", width, height, FMT_RGBA8888, len(data)) + data
    return _pack_message(MSG_FRAME, payload)


# ---------------------------------------------------------------------------
# Stdout/stderr capture
# ---------------------------------------------------------------------------

class _StreamCapture:
    def __init__(self, bridge: "_Bridge", msg_type: int, original):
        self._bridge = bridge
        self._msg_type = msg_type
        self._original = original

    def write(self, text: str) -> int:
        if text.strip():
            try:
                self._bridge.send_raw(
                    _pack_message(self._msg_type, text.encode("utf-8", errors="replace"))
                )
            except Exception:
                pass
        if self._original:
            self._original.write(text)
        return len(text)

    def flush(self):
        if self._original:
            self._original.flush()

    def isatty(self):
        return False


# ---------------------------------------------------------------------------
# Offscreen renderer — QQuickRenderControl + OpenGL FBO
# ---------------------------------------------------------------------------

class _Renderer(QObject):
    """
    Owns the full offscreen render stack:
      QOpenGLContext -> QOffscreenSurface -> QQuickRenderControl -> QQuickWindow
      -> QOpenGLFramebufferObject

    QML is loaded via QQmlComponent into the QQuickWindow's content item.
    """

    def __init__(self, width: int, height: int):
        super().__init__()
        self._width = width
        self._height = height
        self._gl: QOpenGLContext | None = None
        self._surface: QOffscreenSurface | None = None
        self._rc: QQuickRenderControl | None = None
        self._window: QQuickWindow | None = None
        self._fbo: QOpenGLFramebufferObject | None = None
        self._engine: QQmlEngine | None = None
        self._root_item: QQuickItem | None = None
        self._appwindow_ref = None
        self._ready = False

    def setup(self) -> bool:
        fmt = QSurfaceFormat()
        fmt.setDepthBufferSize(24)
        fmt.setStencilBufferSize(8)
        fmt.setVersion(3, 3)
        fmt.setProfile(QSurfaceFormat.OpenGLContextProfile.CoreProfile)
        QSurfaceFormat.setDefaultFormat(fmt)

        self._gl = QOpenGLContext()
        self._gl.setFormat(fmt)
        if not self._gl.create():
            # Retry with compatibility profile
            fmt.setProfile(QSurfaceFormat.OpenGLContextProfile.CompatibilityProfile)
            fmt.setVersion(2, 0)
            self._gl.setFormat(fmt)
            if not self._gl.create():
                print("workbench_bridge: OpenGL context creation failed", file=sys.__stderr__)
                return False

        self._surface = QOffscreenSurface()
        self._surface.setFormat(self._gl.format())
        self._surface.create()
        if not self._surface.isValid():
            print("workbench_bridge: offscreen surface creation failed", file=sys.__stderr__)
            return False

        if not self._gl.makeCurrent(self._surface):
            print("workbench_bridge: makeCurrent failed", file=sys.__stderr__)
            return False

        self._rc = QQuickRenderControl(self)
        self._window = QQuickWindow(self._rc)
        self._window.setGeometry(0, 0, self._width, self._height)

        if not self._rc.initialize():
            print("workbench_bridge: QQuickRenderControl.initialize() failed", file=sys.__stderr__)
            self._gl.doneCurrent()
            return False

        self._engine = QQmlEngine(self._window)

        qt_qml_path = os.environ.get("QT_INSTALL_QML", "")
        if qt_qml_path and os.path.isdir(qt_qml_path):
            self._engine.addImportPath(qt_qml_path)

        qml2_path = os.environ.get("QML2_IMPORT_PATH", "")
        if qml2_path:
            for p in qml2_path.split(os.pathsep):
                if p and os.path.isdir(p):
                    self._engine.addImportPath(p)

        self._create_fbo()
        self._gl.doneCurrent()
        self._ready = True
        return True

    def _create_fbo(self) -> None:
        if self._fbo:
            del self._fbo
        fmt = QOpenGLFramebufferObjectFormat()
        fmt.setAttachment(QOpenGLFramebufferObject.Attachment.CombinedDepthStencil)
        self._fbo = QOpenGLFramebufferObject(QSize(self._width, self._height), fmt)
        target = QQuickRenderTarget.fromOpenGLTexture(
            self._fbo.texture(), QSize(self._width, self._height)
        )
        self._window.setRenderTarget(target)

    def load_file(self, path: str) -> None:
        if not self._ready or self._engine is None or self._window is None:
            return
        self._load_component(QUrl.fromLocalFile(path))

    def load_string(self, text: str, base_url: str = "") -> None:
        if not self._ready or self._engine is None or self._window is None:
            return
        url = QUrl(base_url) if base_url else QUrl()
        comp = QQmlComponent(self._engine)
        comp.setData(text.encode("utf-8"), url)
        self._instantiate(comp)

    def _load_component(self, url: QUrl) -> None:
        comp = QQmlComponent(self._engine, url)
        if comp.isLoading():
            comp.statusChanged.connect(lambda _: self._instantiate(comp))
        else:
            self._instantiate(comp)

    def _instantiate(self, comp: QQmlComponent) -> None:
        if comp.isError():
            errs = "\n".join(e.toString() for e in comp.errors())
            print(f"workbench_bridge: QML errors:\n{errs}", file=sys.__stderr__)
            return
        obj = comp.create()
        if obj is None:
            print("workbench_bridge: component.create() returned None", file=sys.__stderr__)
            return
        if isinstance(obj, QQuickItem):
            self._root_item = obj
            obj.setParentItem(self._window.contentItem())
            obj.setWidth(self._width)
            obj.setHeight(self._height)
        elif isinstance(obj, QQuickWindow):
            # ApplicationWindow creates a QQuickWindow
            self._appwindow_ref = obj
            content = obj.contentItem()
            if content is not None:
                self._root_item = content
                content.setParentItem(self._window.contentItem())
                content.setWidth(self._width)
                content.setHeight(self._height)
        QTimer.singleShot(0, self._schedule_first_frame)

    def _schedule_first_frame(self) -> None:
        """Give Qt one more event-loop pass to finish QML bindings, then render."""
        if self._window:
            self._window.update()
        QTimer.singleShot(50, self._trigger_render)

    def _trigger_render(self) -> None:
        if self._ready and self._window:
            self._window.update()

    def resize(self, width: int, height: int) -> None:
        if not self._ready:
            return
        self._width = width
        self._height = height
        if self._gl and self._surface:
            self._gl.makeCurrent(self._surface)
            self._create_fbo()
            self._gl.doneCurrent()
        if self._window:
            self._window.setGeometry(0, 0, width, height)
        if self._root_item:
            self._root_item.setWidth(width)
            self._root_item.setHeight(height)

    def render(self) -> QImage | None:
        if not self._ready or not self._gl or not self._surface or not self._rc or not self._fbo:
            return None
        if not self._gl.makeCurrent(self._surface):
            return None
        try:
            self._rc.polishItems()
            self._rc.beginFrame()
            self._rc.render()
            self._rc.endFrame()
            img = self._fbo.toImage()
            return img.convertToFormat(QImage.Format.Format_RGBA8888) if not img.isNull() else None
        finally:
            self._gl.doneCurrent()

    def window(self) -> QQuickWindow | None:
        return self._window

    def cleanup(self) -> None:
        if not self._ready:
            return
        if self._gl and self._surface:
            self._gl.makeCurrent(self._surface)
        if self._fbo:
            del self._fbo
            self._fbo = None
        if self._rc:
            self._rc.invalidate()
        if self._gl and self._surface:
            self._gl.doneCurrent()
        self._root_item = None
        self._appwindow_ref = None
        self._ready = False


# ---------------------------------------------------------------------------
# The Bridge
# ---------------------------------------------------------------------------

class _Bridge(QObject):

    def __init__(self, socket_name: str):
        super().__init__()
        self._socket_name = socket_name
        self._socket = QLocalSocket(self)
        self._read_buf = bytearray()
        self._renderer: _Renderer | None = None
        self._width = 800
        self._height = 600

        self._socket.readyRead.connect(self._on_ready_read)
        self._socket.connected.connect(self._on_connected)
        self._socket.errorOccurred.connect(self._on_socket_error)

        self._connect_timer = QTimer(self)
        self._connect_timer.setInterval(100)
        self._connect_timer.timeout.connect(self._try_connect)
        self._connect_timer.start()

        self._render_timer = QTimer(self)
        self._render_timer.setInterval(16)
        self._render_timer.timeout.connect(self._maybe_render)

        # Create renderer immediately
        QTimer.singleShot(0, self._init_renderer)

    def _init_renderer(self) -> None:
        r = _Renderer(self._width, self._height)
        if r.setup():
            self._renderer = r
        else:
            QTimer.singleShot(500, lambda: self.send_raw(
                _pack_message(MSG_ERROR, b"OpenGL offscreen renderer failed to initialise. "
                              b"Ensure your system has OpenGL support.")
            ))

    def load_qml_file(self, path: str) -> None:
        if self._renderer:
            self._renderer.load_file(path)

    def load_qml_string(self, text: str, base_url: str = "") -> None:
        if self._renderer:
            self._renderer.load_string(text, base_url)

    def _try_load_from_engine(self, engine) -> None:
        """Fallback: extract QML URL from engine and reload in our renderer."""
        try:
            roots = engine.rootObjects()
            if not roots:
                return
            root = roots[0]
            url = getattr(root, "url", None) or getattr(engine, "baseUrl", lambda: None)()
            if url and str(url).endswith(".qml"):
                self.load_qml_file(str(url).replace("file://", "").replace("file:///", ""))
        except Exception:
            pass

    def send_raw(self, data: bytes) -> None:
        if self._socket.isValid() and self._socket.state() == QLocalSocket.LocalSocketState.ConnectedState:
            self._socket.write(QByteArray(data))

    def _try_connect(self) -> None:
        if self._socket.state() != QLocalSocket.LocalSocketState.UnconnectedState:
            return
        self._socket.connectToServer(self._socket_name)

    def _on_connected(self) -> None:
        self._connect_timer.stop()
        self.send_raw(_pack_message(MSG_READY))
        self._render_timer.start()
        QTimer.singleShot(100, self._maybe_render)

    def _on_socket_error(self, err) -> None:
        if self._socket.state() not in (
            QLocalSocket.LocalSocketState.ConnectingState,
            QLocalSocket.LocalSocketState.UnconnectedState,
        ):
            print(f"workbench_bridge: socket error: {self._socket.errorString()}", file=sys.__stderr__)

    def _maybe_render(self) -> None:
        if self._renderer is None:
            return
        try:
            img = self._renderer.render()
            if img is not None and not img.isNull():
                bits = img.constBits()
                data = bits.tobytes() if hasattr(bits, "tobytes") else bytes(bits)
                self.send_raw(_pack_frame(img.width(), img.height(), data))
        except Exception:
            err = traceback.format_exc()
            self.send_raw(_pack_message(MSG_ERROR, err.encode("utf-8")))

    def _on_ready_read(self) -> None:
        self._read_buf.extend(bytes(self._socket.readAll()))
        self._process_buffer()

    def _process_buffer(self) -> None:
        while len(self._read_buf) >= HEADER_SIZE:
            msg_type, payload_len = struct.unpack_from(HEADER_FMT, self._read_buf)
            total = HEADER_SIZE + payload_len
            if len(self._read_buf) < total:
                break
            payload = bytes(self._read_buf[HEADER_SIZE:total])
            del self._read_buf[:total]
            self._handle_message(msg_type, payload)

    def _handle_message(self, msg_type: int, payload: bytes) -> None:
        if msg_type == MSG_RESIZE:
            w, h = struct.unpack_from(">ii", payload)
            self._width, self._height = w, h
            if self._renderer:
                self._renderer.resize(w, h)

        elif msg_type in (MSG_MOUSE_PRESS, MSG_MOUSE_RELEASE, MSG_MOUSE_MOVE):
            self._inject_mouse(msg_type, payload)

        elif msg_type in (MSG_KEY_PRESS, MSG_KEY_RELEASE):
            self._inject_key(msg_type, payload)

        elif msg_type == MSG_STOP:
            QCoreApplication.quit()

    def _target_window(self) -> QQuickWindow | None:
        return self._renderer.window() if self._renderer else None

    def _inject_mouse(self, msg_type: int, payload: bytes) -> None:
        win = self._target_window()
        if win is None or len(payload) < 12:
            return
        x, y, button = struct.unpack_from(">iii", payload)
        if _BINDING == "pyside6":
            from PySide6.QtCore import QPointF
        else:
            from PyQt6.QtCore import QPointF
        pos = QPointF(x, y)
        qt_button = Qt.MouseButton(button) if button else Qt.MouseButton.NoButton
        type_map = {
            MSG_MOUSE_PRESS:   QMouseEvent.Type.MouseButtonPress,
            MSG_MOUSE_RELEASE: QMouseEvent.Type.MouseButtonRelease,
            MSG_MOUSE_MOVE:    QMouseEvent.Type.MouseMove,
        }
        try:
            ev = QMouseEvent(
                type_map[msg_type], pos, pos, qt_button,
                qt_button if msg_type != MSG_MOUSE_MOVE else Qt.MouseButton.NoButton,
                Qt.KeyboardModifier.NoModifier,
            )
            QCoreApplication.sendEvent(win, ev)
        except Exception:
            pass

    def _inject_key(self, msg_type: int, payload: bytes) -> None:
        win = self._target_window()
        if win is None or len(payload) < 12:
            return
        key, mods, text_len = struct.unpack_from(">iiI", payload)
        text = payload[12:12 + text_len].decode("utf-8", errors="replace")
        ev_type = QKeyEvent.Type.KeyPress if msg_type == MSG_KEY_PRESS else QKeyEvent.Type.KeyRelease
        try:
            ev = QKeyEvent(ev_type, key, Qt.KeyboardModifier(mods), text)
            QCoreApplication.sendEvent(win, ev)
        except Exception:
            pass


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    if len(sys.argv) < 3:
        print("Usage: workbench_bridge.py <socket_name> <user_script>", file=sys.stderr)
        sys.exit(1)

    socket_name = sys.argv[1]
    user_script = sys.argv[2]

    global _bridge, _running_in_workbench
    _running_in_workbench = True

    _fmt = QSurfaceFormat()
    _fmt.setDepthBufferSize(24)
    _fmt.setStencilBufferSize(8)
    QSurfaceFormat.setDefaultFormat(_fmt)

    app = QGuiApplication(sys.argv[:1])

    _bridge = _Bridge(socket_name)

    sys.stdout = _StreamCapture(_bridge, MSG_STDOUT, sys.__stdout__)
    sys.stderr = _StreamCapture(_bridge, MSG_STDERR, sys.__stderr__)

    try:
        spec = importlib.util.spec_from_file_location("__wb_user__", user_script)
        if spec is None or spec.loader is None:
            raise ImportError(f"Cannot load {user_script}")
        mod = importlib.util.module_from_spec(spec)
        sys.modules["workbench_bridge"] = sys.modules[__name__]
        spec.loader.exec_module(mod)
    except SystemExit:
        pass
    except Exception:
        _bridge.send_raw(_pack_message(MSG_ERROR, traceback.format_exc().encode("utf-8")))

    app.exec()

    if _bridge._renderer:
        _bridge._renderer.cleanup()


if __name__ == "__main__":
    main()
