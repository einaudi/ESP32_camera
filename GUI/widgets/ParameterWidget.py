from PySide6.QtWidgets import (
    QWidget, QLabel, QSlider, QDoubleSpinBox,
    QHBoxLayout, QVBoxLayout
)
from PySide6.QtCore import Qt


class ParameterWidget(QWidget):

    def __init__(
        self,
        label,
        min_val,
        max_val,
        value,
        decimals=3,
        step=None,
        parent=None
    ):
        super().__init__(parent)

        self.min_val = min_val
        self.max_val = max_val
        self.decimals = decimals

        if step is None:
            self.step = 10 ** (-decimals)
        else:
            self.step = step

        # liczba pozycji slidera
        self.slider_steps = int(round((self.max_val - self.min_val) / self.step))

        self._build_ui(label)
        self._connect_signals()

        self.set_value(value)

    def _build_ui(self, label_text):

        self.label = QLabel(label_text)

        self.spinbox = QDoubleSpinBox()
        self.spinbox.setRange(self.min_val, self.max_val)
        self.spinbox.setDecimals(self.decimals)
        self.spinbox.setSingleStep(self.step)

        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, self.slider_steps)

        top_layout = QHBoxLayout()
        top_layout.addWidget(self.label)
        top_layout.addStretch()
        top_layout.addWidget(self.spinbox)

        layout = QVBoxLayout(self)
        layout.addLayout(top_layout)
        layout.addWidget(self.slider)

    def _connect_signals(self):

        self.slider.valueChanged.connect(self._slider_changed)
        self.spinbox.valueChanged.connect(self._spinbox_changed)

    def _slider_changed(self, slider_value):

        value = self.min_val + slider_value * self.step

        self.spinbox.blockSignals(True)
        self.spinbox.setValue(value)
        self.spinbox.blockSignals(False)

    def _spinbox_changed(self, value):

        slider_value = round((value - self.min_val) / self.step)

        self.slider.blockSignals(True)
        self.slider.setValue(slider_value)
        self.slider.blockSignals(False)

    def set_value(self, value):

        slider_value = round((value - self.min_val) / self.step)

        self.spinbox.blockSignals(True)
        self.spinbox.setValue(value)
        self.spinbox.blockSignals(False)

        self.slider.blockSignals(True)
        self.slider.setValue(slider_value)
        self.slider.blockSignals(False)

    def value(self):

        return self.spinbox.value()