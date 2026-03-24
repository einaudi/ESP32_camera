import sys

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout, QGroupBox, QMessageBox

from widgets.imageWidget import ImageWidget
from widgets.controlCamera import CameraControlWidget
from widgets.controlROI import ROIControlWidget
from widgets.controlTarget import TargetControlWidget
from widgets.statusWidget import StatusWidget
from widgets.serialWidget import SerialWidget

from PIL import Image


class MainWindow(QMainWindow):
    
    def __init__(self):
        super().__init__()

        self.setWindowTitle("ESP32CAM")
        self.resize(800, 600)

        self._init_widgets()
        self._init_layout()
        self.init_ui()

    def _init_widgets(self):

        self._widgets = {
            'imageWidget': ImageWidget(self),
            'cameraControl': CameraControlWidget(self),
            'ROIControl': ROIControlWidget(self),
            'targetControl': TargetControlWidget(self),
            'statusWidget': StatusWidget(self),
            'serialWidget': SerialWidget(self)
        }

    def _init_layout(self):

        # Camera group
        groupCamera = QGroupBox('Camera control')
        groupCameraLayout = QVBoxLayout()
        groupCameraLayout.addWidget(self._widgets['cameraControl'])
        groupCamera.setLayout(groupCameraLayout)

        # ROI group
        groupROI = QGroupBox('ROI control')
        groupROILayout = QVBoxLayout()
        groupROILayout.addWidget(self._widgets['ROIControl'])
        groupROI.setLayout(groupROILayout)

        # Target group
        groupTarget = QGroupBox('Target control')
        groupTargetLayout = QVBoxLayout()
        groupTargetLayout.addWidget(self._widgets['targetControl'])
        groupTarget.setLayout(groupTargetLayout)

        # Control box
        controlBox = QHBoxLayout()
        controlBox.addWidget(groupCamera)
        controlBox.addWidget(groupROI)
        controlBox.addWidget(groupTarget)

        # Error group
        groupError = QGroupBox('Error data')
        groupErrorLayout = QVBoxLayout()
        groupErrorLayout.addWidget(self._widgets['statusWidget'])
        groupError.setLayout(groupErrorLayout)

        # Right box
        rightBox = QVBoxLayout()
        rightBox.addLayout(controlBox)
        rightBox.addWidget(groupError)
        rightBox.addStretch(1)

        # Preview (left) group
        groupPreview = QGroupBox('Preview')
        groupPreviewLayout = QVBoxLayout()
        groupPreviewLayout.addWidget(self._widgets['imageWidget'])
        groupPreview.setLayout(groupPreviewLayout)

        # Top layout
        topLayout = QHBoxLayout()
        topLayout.addWidget(groupPreview)
        topLayout.addLayout(rightBox)

        # Comm group
        groupComm = QGroupBox('Serial communication')
        groupCommLayout = QVBoxLayout()
        groupCommLayout.addWidget(self._widgets['serialWidget'])
        groupComm.setLayout(groupCommLayout)

        # Main layout
        central = QWidget()
        mainLayout = QVBoxLayout(central)
        mainLayout.addLayout(topLayout)
        mainLayout.addWidget(groupComm)
        mainLayout.addStretch(1)

        self.setCentralWidget(central)

    def init_ui(self):

        pil_image = Image.open('misc/preview_default.png')
        self._widgets['imageWidget'].set_image(pil_image)
        self._widgets['serialWidget'].connection_successful.connect(self._get_parameters)

    def query(self, command, callback):

        self._widgets['serialWidget'].query(command, callback)

    def closeEvent(self, event):

        if self._widgets['serialWidget'] and self._widgets['serialWidget'].is_connected:

            msg = QMessageBox(self)
            msg.setIcon(QMessageBox.Question)
            msg.setWindowTitle("Close application")
            msg.setText("Device is still connected.")
            msg.setInformativeText("Disconnect and exit?")
            msg.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
            msg.setDefaultButton(QMessageBox.No)

            reply = msg.exec()

            if reply == QMessageBox.No:
                event.ignore()
                return

            self._widgets['serialWidget'].disconnect()

        event.accept()

    def _get_parameters(self):

        self._widgets['cameraControl'].get_parameters()
        self._widgets['ROIControl'].get_parameters()
        self._widgets['targetControl'].get_parameters()

    def get_data(self, dataType=None):

        if not dataType:
            return
        
        for widget in self._widgets.values():
            if dataType in widget.available_data:
                return widget.get_data(dataType)
            
        print('Unknonw data type! {}'.format(dataType))
        return
    

def main():
    app = QApplication(sys.argv)

    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()