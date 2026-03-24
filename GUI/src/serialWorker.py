import serial
import time
from collections import deque

from PySide6.QtCore import QObject, Signal, Slot, QTimer, QThread

from PIL import Image

from src.utils import timestamp, dump_rgb_stream_to_txt


class SerialWorker(QObject):

    console_output = Signal(str)
    connected = Signal()
    disconnected = Signal()

    # command_id, command, response
    response_received = Signal(int, str, object)

    def __init__(self, port):
        super().__init__()

        self.port = port
        self.serial = None
        self.rx_timer = None

        self.running = False

        # command queue
        self.command_queue = deque()

        # current command
        self.current_command = None
        self.current_command_id = None
        self.command_start_time = None

        self.command_timeout = 2.0

        self.command_counter = 0

    # ---------- lifecycle ----------
    @Slot()
    def start(self):

        try:
            self.serial = serial.Serial(self.port, 115200, timeout=0)
            if not serial:
                self._emit_log(f"Connection error")
                self.disconnected.emit()
                return

            self.running = True

            self.rx_timer = QTimer(self)
            self.rx_timer.setInterval(10)
            self.rx_timer.timeout.connect(self._process)
            self.rx_timer.start()

            self.connected.emit()
            self._emit_log(f"Connected to {self.port}")

        except Exception as e:
            self._emit_log(f"Connection error: {e}")
            self.disconnected.emit()

    @Slot()
    def stop(self):

        self.running = False

        if self.rx_timer:
            self.rx_timer.stop()
            self.rx_timer.deleteLater()
            self.rx_timer = None

        if self.serial and self.serial.is_open:
            self.serial.close()

        self._emit_log("Disconnected")
        self.disconnected.emit()

        QThread.currentThread().quit()    

    # ---------- API ----------
    @Slot(str)
    def send_command(self, cmd):

        # if not cmd.startswith(":"):
        #     cmd = ":" + cmd

        self.command_counter += 1

        command_id = self.command_counter

        self.command_queue.append((command_id, cmd))

    # ---------- main worker loop ----------
    def _process(self):

        if not self.running:
            return

        # start next command if none active
        if self.current_command is None and self.command_queue:

            command_id, cmd = self.command_queue.popleft()

            self.current_command = cmd
            self.current_command_id = command_id
            self.command_start_time = time.time()

            self.serial.reset_input_buffer()

            self.serial.write((cmd + "\n").encode())

            self._emit_tx(cmd)

        # check timeout
        if self.current_command:

            if time.time() - self.command_start_time > self.command_timeout:

                self._emit_log("command timeout")

                self.response_received.emit(
                    self.current_command_id,
                    self.current_command,
                    ':ERR:TIMEOUT'
                )

                self.current_command = None
                self.current_command_id = None

        # read all incoming lines
        while self.serial and self.serial.in_waiting:

            line = self.serial.readline()

            if not line:
                break

            text = line.decode(errors="ignore").strip()

            if text.startswith(":"):

                self._emit_rx(text)

                if text.startswith(':IMG'):
                    self._process_image(text)
                    break

                if self.current_command:

                    response = text[1:]

                    self.response_received.emit(
                        self.current_command_id,
                        self.current_command,
                        response
                    )

                    self.current_command = None
                    self.current_command_id = None

            else:

                self._emit_log(text)

    def _process_image(self, resp):

        self._emit_log('Acquiring image...')

        # Read image data
        parts = resp.split()
        size = int(parts[1])
        print(f"Receiving {size} bytes...")
        self.serial.timeout = 5
        data = self.serial.read(size)
        self.serial.timeout = 0
        print("Read", len(data), "bytes")

        # Read until :OK received
        while True:
            line = self.serial.readline().decode(errors="ignore").strip()

            if not line:
                continue
            
            if "OK" in line:
                self._emit_rx(line)
                break
            else:
                self._emit_log(line)

        # Convert data to PIL Image
        try:
            img = Image.frombytes(
                "RGB",
                (640, 480),
                data,
                "raw",
                "BGR"
            )
        except:
            img = None

        # Send response to caller
        if self.current_command:
            self.response_received.emit(
                self.current_command_id,
                self.current_command,
                img
            )

            self.current_command = None
            self.current_command_id = None

    def _process_image_sync(self, resp):

        self._emit_log('Acquiring image...')

        # Read image data
        parts = resp.split()
        size = int(parts[1])
        chunk_size = int(parts[3])
        print(f"Receiving {size} bytes in chunks...")

        self.serial.timeout = 5
        data = {}
        chunks_received = 0
        chunks_total = 0
        while True:
            line = self.serial.readline().decode(errors="ignore").strip()
            self._emit_log(line)
            if line.startswith(':CHUNK_ID'):
                chunk_id = int(line.split()[1])
                chunk_data = self.serial.read(chunk_size)
                data[chunk_id] = chunk_data
                chunks_received += 1
                print('New chunk! {}'.format(chunks_received))
            elif line.startswith(':END'):
                chunks_total = int(line.split()[2])
            elif line.startswith(':OK'):
                break
            else:
                self._emit_log('Image transfer error!')
                print(line)
                if self.current_command:
                    self.response_received.emit(
                        self.current_command_id,
                        self.current_command,
                        None
                    )

                    self.current_command = None
                    self.current_command_id = None
                return

        # print(data)

        # data = self.serial.read(size)

        self.serial.timeout = 0
        # print("Read", len(data), "bytes")


        # Send response to caller
        if self.current_command:
            self.response_received.emit(
                self.current_command_id,
                self.current_command,
                None
            )

            self.current_command = None
            self.current_command_id = None

    def _process_image_test(self, resp):

        self._emit_log('Acquiring image...')
        end_sequence = b'END OF IMAGE'

        # Read image data
        parts = resp.split()
        size = int(parts[1])
        print(f"Receiving {size} bytes...")
        self.serial.timeout = 20
        # Read until end sequence received
        data = self.serial.read_until(end_sequence)
        self.serial.timeout = 0
        data = data[:-len(end_sequence)]
        print("Read", len(data), "bytes")

        # Read until :OK received
        start_time = time.time()
        while True:
            if time.time() - start_time > 5:
                if self.current_command:
                    self.response_received.emit(
                        self.current_command_id,
                        self.current_command,
                        None
                    )

                    self.current_command = None
                    self.current_command_id = None
                self._emit_log('Image acquire timeout!')
                return

            line = self.serial.readline().decode(errors="ignore").strip()

            if not line:
                continue
            
            if "OK" in line:
                self._emit_rx(line)
                break
            else:
                self._emit_log(line)

        dump_rgb_stream_to_txt(data, 640, './debug.txt')

        dataCleaned = data.replace(b'\x0D\x0A', b'')
        print("Size after cleaning:", len(dataCleaned), "bytes")
        dump_rgb_stream_to_txt(dataCleaned, 640, './debug_cleaned.txt')

        # Convert data to PIL Image
        img = Image.frombytes(
            "RGB",
            (640, 480),
            data,
            "raw",
            "BGR"
        )

        # Send response to caller
        if self.current_command:
            self.response_received.emit(
                self.current_command_id,
                self.current_command,
                img
            )

            self.current_command = None
            self.current_command_id = None


    # ---------- console logging ----------
    def _emit_tx(self, text):

        html = f"""
        <span style="color:#888">{timestamp()}</span>
        <span style="color:#c0392b;font-weight:bold"> TX </span>
        <span style="color:#c0392b">{text}</span>
        """

        self.console_output.emit(html)

    def _emit_rx(self, text):

        html = f"""
        <span style="color:#888">{timestamp()}</span>
        <span style="color:#27ae60;font-weight:bold"> RX </span>
        {text}
        """

        self.console_output.emit(html)

    def _emit_log(self, text):

        html = f"""
        <span style="color:#888">{timestamp()}</span>
        <span style="color:#555"> LOG </span>
        {text}
        """

        self.console_output.emit(html)
