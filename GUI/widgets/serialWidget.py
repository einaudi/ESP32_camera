import serial
import serial.tools.list_ports


from PySide6.QtWidgets import (
    QWidget, QComboBox, QPushButton, QTextEdit,
    QLabel, QHBoxLayout, QVBoxLayout
)
from PySide6.QtCore import Signal, QThread

from widgets.statusLED import StatusLED
from src.serialWorker import SerialWorker
from src.utils import timestamp
from config import config as cfg



class SerialWidget(QWidget):

    connect_requested = Signal(str)
    disconnect_requested = Signal()
    send_command_signal = Signal(str)
    stop_worker_signal = Signal()
    connection_successful = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)

        self.is_connected = False
        self.worker = None
        self.thread = None

        self._pending_callbacks = {}

        self._build_ui()
        self._connect_signals()

        self.refresh_ports()

        self.available_data = []

    def _build_ui(self):

        # port selection
        self.port_combo = QComboBox()
        self.refresh_button = QPushButton("Refresh")

        # connection
        self.connect_button = QPushButton("Connect")
        self.status_led = StatusLED()

        text = "Disconnect"
        fm = self.connect_button.fontMetrics()

        width = fm.horizontalAdvance(text) + 50
        self.connect_button.setFixedWidth(width)

        # console
        self.console = QTextEdit()
        self.console.setReadOnly(True)
        self.console.document().setMaximumBlockCount(cfg.CONSOLE_MAX_LINES)

        # layouts
        top_layout = QHBoxLayout()
        top_layout.addWidget(QLabel("Port:"))
        top_layout.addWidget(self.port_combo)
        top_layout.addWidget(self.refresh_button)
        top_layout.addSpacing(10)
        top_layout.addWidget(self.connect_button)
        top_layout.addWidget(self.status_led)
        top_layout.addStretch(1)

        layout = QVBoxLayout(self)
        layout.addLayout(top_layout)
        layout.addWidget(self.console)

    def _connect_signals(self):

        self.refresh_button.clicked.connect(self.refresh_ports)
        self.connect_button.clicked.connect(self._connect_clicked)

    def refresh_ports(self):

        self.port_combo.clear()

        ports = serial.tools.list_ports.comports()

        for port in ports:
            self.port_combo.addItem(port.device)

    def _connect_clicked(self):

        if self.is_connected:
            self.disconnect()
        else:
            port = self.port_combo.currentText()
            self.connect(port)

    def _handle_response(self, id, command, value):

        if command in self._pending_callbacks:

            callback = self._pending_callbacks.pop(command)

            callback(value)

    def connect(self, port):

        if self.thread is not None:
            return

        self.thread = QThread()

        self.worker = SerialWorker(port)
        self.worker.moveToThread(self.thread)

        self.thread.started.connect(self.worker.start)

        self.worker.console_output.connect(self.console.append)
        self.worker.connected.connect(self._on_connected)
        self.worker.disconnected.connect(self._on_disconnected)
        self.worker.response_received.connect(self._handle_response)
        self.stop_worker_signal.connect(self.worker.stop)

        self.send_command_signal.connect(self.worker.send_command)

        self.thread.start()

    def disconnect(self):

        if self.worker:
            self.stop_worker_signal.emit()

        if self.thread:
            self.thread.wait()

        self.worker = None
        self.thread = None

    def _on_connected(self):

        print('Connected!')
        self.is_connected = True
        self.connect_button.setText("Disconnect")
        self.status_led.set_state(True)

        # Validate device
        self.query("*IDN?", self._validate_device)

    def _on_disconnected(self):

        print('Disconnected!')
        self.is_connected = False
        self.connect_button.setText("Connect")
        self.status_led.set_state(False)
        self.disconnect()

    def _validate_device(self, dev):

        if(not dev.startswith('ESP32-S3 CAMERA')):
            html = f"""
            <span style="color:#888">{timestamp()}</span>
            <span style="color:#555"> LOG </span>
            Incorrect device! Expected ESP32-S3 CAMERA
            """
            self.log(html)
            self.disconnect()
        else:
            self.connection_successful.emit()

    def log(self, text):

        self.console.append(text)

    def query(self, command, callback):

        if not self.is_connected:
            html = f"""
            <span style="color:#888">{timestamp()}</span>
            <span style="color:#555"> LOG </span>
            Device not connected!
            """
            self.log(html)
        else:
            self._pending_callbacks[command] = callback

            self.send_command_signal.emit(command)
