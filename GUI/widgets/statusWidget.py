from PySide6.QtWidgets import QWidget, QLabel, QGridLayout, QHBoxLayout, QPushButton
from PySide6.QtCore import Qt

from PySide6.QtGui import QFontDatabase

from widgets.statusLED import StatusLED

import numpy as np


class StatusWidget(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)
        self._parent = parent

        self._init_widgets()
        self._init_layout()

        self.available_data = []

    def _init_widgets(self):

        self.cx = QLabel('{:7.2f}'.format(0))
        self.cy = QLabel('{:7.2f}'.format(0))
        self.error_x = QLabel('{:7.2f}'.format(0))
        self.error_y = QLabel('{:7.2f}'.format(0))

        font = QFontDatabase.systemFont(QFontDatabase.FixedFont)
        font.setPointSize(12)

        self.cx.setFont(font)
        self.cx.setAlignment(Qt.AlignRight)
        self.cy.setFont(font)
        self.cy.setAlignment(Qt.AlignRight)
        self.error_x.setFont(font)
        self.error_x.setAlignment(Qt.AlignRight)
        self.error_y.setFont(font)
        self.error_y.setAlignment(Qt.AlignRight)

        self.hitStatusLED = StatusLED()

        self.btnGetData = QPushButton('Get data')
        self.btnGetData.clicked.connect(self.get_parameters)

        # Postprocessing
        self.imgMin = QLabel('{:7.2f}'.format(0))
        self.imgMax = QLabel('{:7.2f}'.format(0))
        self.imgAvg = QLabel('{:7.2f}'.format(0))

        self.imgMin.setFont(font)
        self.imgMin.setAlignment(Qt.AlignRight)
        self.imgMax.setFont(font)
        self.imgMax.setAlignment(Qt.AlignRight)
        self.imgAvg.setFont(font)
        self.imgAvg.setAlignment(Qt.AlignRight)

        self.btnPostProcess = QPushButton('Process data')
        self.btnPostProcess.clicked.connect(self._postprocess)

    def _init_layout(self):

        layout = QGridLayout()

        layout.addWidget(QLabel("Centroid X:"), 0, 0)
        layout.addWidget(self.cx, 0, 1)
        layout.addWidget(QLabel("Error X:"), 0, 2)
        layout.addWidget(self.error_x, 0, 3)
        layout.addWidget(QLabel("Hit status:"), 0, 4)
        layout.addWidget(self.hitStatusLED, 0, 5)

        layout.addWidget(QLabel("Centroid Y:"), 1, 0)
        layout.addWidget(self.cy, 1, 1)
        layout.addWidget(QLabel("Error Y:"), 1, 2)
        layout.addWidget(self.error_y, 1, 3)
        layout.addWidget(self.btnGetData, 1, 4, 1, 2)

        # Postprocessing
        layout.addWidget(QLabel("ROI min:"), 0, 6)
        layout.addWidget(self.imgMin, 0, 7)
        layout.addWidget(QLabel("ROI avg:"), 0, 8)
        layout.addWidget(self.imgAvg, 0, 9)

        layout.addWidget(QLabel("ROI max:"), 1, 6)
        layout.addWidget(self.imgMax, 1, 7)
        layout.addWidget(self.btnPostProcess, 1, 8, 1, 2)

        mainLayout = QHBoxLayout(self)
        mainLayout.addLayout(layout)
        mainLayout.addStretch(1)

    def set_parameter(self, param: str, value: float):

        if param == 'centroid_x':
            self.cx.setText('{:7.2f}'.format(value))
        elif param == 'centroid_y':
            self.cy.setText('{:7.2f}'.format(value))
        elif param == 'error_x':
            self.error_x.setText('{:7.2f}'.format(value))
        elif param == 'error_y':
            self.error_y.setText('{:7.2f}'.format(value))
        elif param == 'hit_status':
            self.hitStatusLED.set_state(value)
        else:
            print('Unknown status parameter!')

    def _validate_response(self, param: str, resp: str):

        try:
            value = float(resp)
            self.set_parameter(param, value)
        except ValueError:
            print('Parameter error! {}'.format(resp))
            return

    def get_parameters(self):

        cmd = ':centroid:x0?'
        self._parent.query(cmd, lambda resp: self._validate_response('centroid_x', resp))

        cmd = ':centroid:y0?'
        self._parent.query(cmd, lambda resp: self._validate_response('centroid_y', resp))

        cmd = ':target:error_x?'
        self._parent.query(cmd, lambda resp: self._validate_response('error_x', resp))

        cmd = ':target:error_y?'
        self._parent.query(cmd, lambda resp: self._validate_response('error_y', resp))

        cmd = ':target:hit?'
        self._parent.query(cmd, lambda resp: self._validate_response('hit_status', resp))

    def _postprocess(self):

        img = self._parent.get_data('img')
        roi = self._parent.get_data('ROI')
        roiEnable = self._parent.get_data('ROI_enabled')

        if roiEnable:
            x1 = roi[0] + 1
            y1 = roi[1] + 1
            x2 = roi[0] + roi[2] - 1
            y2 = roi[1] + roi[3] - 1
        else:
            x1 = 0
            y1 = 0
            x2 = -1
            y2 = -1

        imgArr = np.array(img)[y1:y2, x1:x2]

        valueMin = np.amin(imgArr)
        valueMax = np.amax(imgArr)
        valueAvg = np.average(imgArr)

        self.imgMin.setText('{:7.2f}'.format(valueMin))
        self.imgMax.setText('{:7.2f}'.format(valueMax))
        self.imgAvg.setText('{:7.2f}'.format(valueAvg))
