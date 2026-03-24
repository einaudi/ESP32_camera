from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel, QFileDialog
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtCore import Qt

from PIL import Image


class ImageWidget(QWidget):

    def __init__(self, parent=None, display_width=640):
        super().__init__(parent)
        self._parent = parent

        self.display_width = display_width
        self.original_pixmap = None

        self.image_label = QLabel()
        self.image_label.setAlignment(Qt.AlignCenter)

        self._btnCapture = QPushButton("Capture image")
        self._btnCapture.clicked.connect(self._capture_image)

        self._btnPreview = QPushButton("Capture preview")
        self._btnPreview.clicked.connect(self._capture_preview)

        self._btnSave = QPushButton("Save preview")
        self._btnSave.clicked.connect(self._save_preview)

        buttons_layout = QHBoxLayout()
        buttons_layout.addWidget(self._btnCapture)
        buttons_layout.addWidget(self._btnPreview)
        buttons_layout.addWidget(self._btnSave)

        layout = QVBoxLayout(self)
        layout.addWidget(self.image_label)
        layout.addLayout(buttons_layout)

        self._pil_image = None
        pil_image = Image.open("misc/preview_default.png")
        self.set_image(pil_image)

        self.available_data = [
            'img'
        ]

    def set_image(self, pil_image):

        if pil_image is None:
            return

        if pil_image.mode != "RGB":
            pil_image = pil_image.convert("RGB")
        self._pil_image = pil_image

        data = pil_image.tobytes()

        qimage = QImage(
            data,
            pil_image.width,
            pil_image.height,
            pil_image.width * 3,
            QImage.Format_RGB888
        )

        self.original_pixmap = QPixmap.fromImage(qimage)

        self._update_display()

    def _update_display(self):

        if self.original_pixmap is None:
            return

        scaled = self.original_pixmap.scaledToWidth(
            self.display_width,
            Qt.SmoothTransformation
        )

        self.image_label.setPixmap(scaled)

    def _validate_response(self, resp):

        if resp == 'OK':
            print('Image captured successfully!')
        else:
            print(f'Image captured failed! {resp}')

    def _capture_image(self):

        cmd = ':image:capture'
        self._parent.query(cmd, self._validate_response)

    def _capture_preview(self):

        self._capture_image()

        cmd = ':image:preview?'
        self._parent.query(cmd, self.set_image)

    def _save_preview(self):

        if self._pil_image is None:
            print("No image to save")
            return

        filename, _ = QFileDialog.getSaveFileName(
            self,
            "Save image",
            "image.png",
            "PNG files (*.png)"
        )

        if not filename:
            return

        # dopisz rozszerzenie jeśli użytkownik go nie podał
        if not filename.lower().endswith(".png"):
            filename += ".png"

        self._pil_image.save(filename, "PNG")

    def get_data(self, dataType=None):

        if not dataType:
            return
        
        if dataType == 'img':
            return self._pil_image.copy()
        else:
            print('Unknown data type! {}'.format(dataType))
            return
