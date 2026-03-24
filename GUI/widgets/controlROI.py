from PySide6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QPushButton
)

from widgets.ParameterWidget import ParameterWidget
from widgets.toggleSwitch import LabeledToggle
from config import config as cfg


class ROIControlWidget(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self._parent = parent

        self._init_widgets()
        self._init_layout()

        self._widgets['btnApply'].clicked.connect(self._apply_clicked)

        self.available_data = [
            'ROI',
            'ROI_enabled'
        ]

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
            'parameter_dx': ParameterWidget(
                'Width:',
                0,
                cfg.CAM_FRAMESIZE_WIDTH,
                50,
                0
            ),
            'parameter_dy': ParameterWidget(
                'Height:',
                0,
                cfg.CAM_FRAMESIZE_HEIGHT,
                50,
                0
            ),
            'toggleEnabled': LabeledToggle('ROI enable:'),
            'btnApply': QPushButton('Apply')
        }

    def _init_layout(self):

        btnBox = QHBoxLayout()
        btnBox.addWidget(self._widgets['btnApply'])

        mainLayout = QVBoxLayout(self)
        mainLayout.addWidget(self._widgets['parameter_x0'])
        mainLayout.addWidget(self._widgets['parameter_y0'])
        mainLayout.addWidget(self._widgets['parameter_dx'])
        mainLayout.addWidget(self._widgets['parameter_dy'])
        mainLayout.addWidget(self._widgets['toggleEnabled'])
        mainLayout.addLayout(btnBox)

        mainLayout.addStretch(1)

    def _validate_set_response(self, resp):

        if resp == 'OK':
            print('Settings applied successfully!')
        else:
            print('Failed! {}'.format(resp))

    def _validate_get_response(self, param: str, resp: str):

        try:
            value = int(resp)
            self.set_parameter(param, value)
        except ValueError:
            print('Parameter error! {}'.format(resp))
            return

    def _send_x0(self):

        value = int(self._widgets['parameter_x0'].value())
        cmd = ':roi:x0 {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_y0(self):

        value = int(self._widgets['parameter_y0'].value())
        cmd = ':roi:y0 {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_dx(self):

        value = int(self._widgets['parameter_dx'].value())
        cmd = ':roi:dx {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_dy(self):

        value = int(self._widgets['parameter_dy'].value())
        cmd = ':roi:dy {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_enable(self):

        value = int(self._widgets['toggleEnabled'].isChecked())
        cmd = ':roi:enable {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _apply_clicked(self):

        self._send_x0()
        self._send_y0()
        self._send_dx()
        self._send_dy()
        self._send_enable()

    def set_parameter(self, param: str, value: int):

        if param == 'x0':
            self._widgets['parameter_x0'].set_value(value)
        elif param == 'y0':
            self._widgets['parameter_y0'].set_value(value)
        elif param == 'dx':
            self._widgets['parameter_dx'].set_value(value)
        elif param == 'dy':
            self._widgets['parameter_dy'].set_value(value)
        elif param == 'enable':
            self._widgets['toggleEnabled'].setChecked(value)

    def get_parameters(self):

        cmd = ':roi:x0?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('x0', resp))

        cmd = ':roi:y0?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('y0', resp))

        cmd = ':roi:dx?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('dx', resp))

        cmd = ':roi:dy?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('dy', resp))

        cmd = ':roi:enable?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('enable', resp))
        
    def get_data(self, dataType=None):

        if not dataType:
            return
        
        if dataType == 'ROI':
            roi = (
                int(self._widgets['parameter_x0'].value()),
                int(self._widgets['parameter_y0'].value()),
                int(self._widgets['parameter_dx'].value()),
                int(self._widgets['parameter_dy'].value())
            )
            return roi
        elif dataType == 'ROI_enabled':
            return self._widgets['toggleEnabled'].isChecked()
        else:
            print('Unknown data type! {}'.format(dataType))
            return
