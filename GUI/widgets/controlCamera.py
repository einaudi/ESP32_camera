from PySide6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QPushButton
)

from widgets.ParameterWidget import ParameterWidget


class CameraControlWidget(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self._parent = parent

        self._init_widgets()
        self._init_layout()

        self._widgets['btnApply'].clicked.connect(self._apply_clicked)

        self.available_data = []

    def _init_widgets(self):

        self._widgets = {
            'parameterExposure': ParameterWidget(
                'Exposure:',
                0,
                1200,
                10,
                0
            ),
            'parameterGain': ParameterWidget(
                'Gain:',
                0,
                30,
                0,
                0
            ),
            'parameterBrightness': ParameterWidget(
                'Brightness:',
                -2,
                2,
                0,
                0
            ),
            'parameterContrast': ParameterWidget(
                'Contrast:',
                -2,
                2,
                0,
                0
            ),
            'btnApply': QPushButton('Apply')
        }

    def _init_layout(self):

        btnBox = QHBoxLayout()
        btnBox.addWidget(self._widgets['btnApply'])

        mainLayout = QVBoxLayout(self)
        mainLayout.addWidget(self._widgets['parameterExposure'])
        mainLayout.addWidget(self._widgets['parameterGain'])
        mainLayout.addWidget(self._widgets['parameterBrightness'])
        mainLayout.addWidget(self._widgets['parameterContrast'])
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

    def _send_exposure(self):

        value = int(self._widgets['parameterExposure'].value())
        cmd = ':camera:exposure {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_gain(self):

        value = int(self._widgets['parameterGain'].value())
        cmd = ':camera:gain {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_brightness(self):

        value = int(self._widgets['parameterBrightness'].value())
        cmd = ':camera:brightness {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _send_contrast(self):

        value = int(self._widgets['parameterContrast'].value())
        cmd = ':camera:contrast {:d}'.format(value)

        self._parent.query(cmd, self._validate_set_response)

    def _apply_clicked(self):

        self._send_exposure()
        self._send_gain()
        self._send_brightness()
        self._send_contrast()
        
    def set_parameter(self, param: str, value: int):

        if param == 'exposure':
            self._widgets['parameterExposure'].set_value(value)
        elif param == 'gain':
            self._widgets['parameterGain'].set_value(value)
        elif param == 'brightness':
            self._widgets['parameterBrightness'].set_value(value)
        elif param == 'contrast':
            self._widgets['parameterContrast'].set_value(value)

    def get_parameters(self):

        cmd = ':camera:exposure?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('exposure', resp))

        cmd = ':camera:gain?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('gain', resp))

        cmd = ':camera:brightness?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('brightness', resp))

        cmd = ':camera:contrast?'
        self._parent.query(cmd, lambda resp: self._validate_get_response('contrast', resp))
        