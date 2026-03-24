from PySide6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QPushButton
)

from widgets.ParameterWidget import ParameterWidget

from config import config as cfg

class TargetControlWidget(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self._parent = parent

        self._init_widgets()
        self._init_layout()

        self._widgets['btnApply'].clicked.connect(self._apply_clicked)

        self.available_data = []

    def _init_widgets(self):

        self._widgets = {
            'parameter_x0': ParameterWidget(
                'x0:',
                0,
                cfg.CAM_FRAMESIZE_WIDTH,
                0,
                0
            ),
            'parameter_y0': ParameterWidget(
                'y0:',
                0,
                cfg.CAM_FRAMESIZE_HEIGHT,
                0,
                0
            ),
            'parameter_tol': ParameterWidget(
                'Tolerance:',
                0,
                100,
                5,
                0
            ),
            'parameter_threshold': ParameterWidget(
                'Threshold:',
                0,
                255,
                10,
                0
            ),
            'btnApply': QPushButton('Apply')
        }

    def _init_layout(self):

        btnBox = QHBoxLayout()
        btnBox.addWidget(self._widgets['btnApply'])

        mainLayout = QVBoxLayout(self)
        mainLayout.addWidget(self._widgets['parameter_x0'])
        mainLayout.addWidget(self._widgets['parameter_y0'])
        mainLayout.addWidget(self._widgets['parameter_tol'])
        mainLayout.addWidget(self._widgets['parameter_threshold'])
        mainLayout.addLayout(btnBox)

        mainLayout.addStretch(1)

    def _validate_set_response(self, resp):

        if resp == 'OK':
            print('Settings applied successfully!')
        else:
            print('Failed! {}'.format(resp))

    def _validate_get_response(self, param: str, resp: str):

        try:
            value = float(resp)
            self.set_parameter(param, value)
        except ValueError:
            print('Parameter error! {}'.format(resp))
            return

    def _send_x0(self):

        value = int(self._widgets['parameter_x0'].value())
        cmd = ':target:x0 {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_y0(self):

        value = int(self._widgets['parameter_y0'].value())
        cmd = ':target:y0 {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_tol(self):

        value = int(self._widgets['parameter_tol'].value())
        cmd = ':target:tolerance {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_threshold(self):

        value = int(self._widgets['parameter_threshold'].value())
        cmd = ':target:threshold {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _apply_clicked(self):

        self._send_x0()
        self._send_y0()
        self._send_tol()
        self._send_threshold()

    def set_parameter(self, param: str, value: float):

        if param == 'x0':
            self._widgets['parameter_x0'].set_value(value)
        elif param == 'y0':
            self._widgets['parameter_y0'].set_value(value)
        elif param == 'tol':
            self._widgets['parameter_tol'].set_value(value)
        elif param == 'threshold':
            self._widgets['parameter_threshold'].set_value(value)

    def get_parameters(self):

        cmd = ':target:x0?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('x0', resp))

        cmd = ':target:y0?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('y0', resp))

        cmd = ':target:tolerance?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('tol', resp))

        cmd = ':target:threshold?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('threshold', resp))
