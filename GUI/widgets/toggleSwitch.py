from PySide6.QtWidgets import QCheckBox, QWidget, QLabel, QHBoxLayout
from PySide6.QtCore import Qt, Property, QPropertyAnimation
from PySide6.QtGui import QPainter, QColor


class ToggleSwitch(QCheckBox):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.setFixedSize(50, 25)

        self._offset = 2

        self._animation = QPropertyAnimation(self, b"offset", self)
        self._animation.setDuration(150)

        self.toggled.connect(self._start_animation)

    def _start_animation(self, checked):

        start = self._offset
        end = self.width() - self.height() + 2 if checked else 2

        self._animation.stop()
        self._animation.setStartValue(start)
        self._animation.setEndValue(end)
        self._animation.start()

    def get_offset(self):
        return self._offset

    def set_offset(self, value):
        self._offset = value
        self.update()

    offset = Property(float, get_offset, set_offset)

    def paintEvent(self, event):

        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        if self.isChecked():
            p.setBrush(QColor("#50C878"))
        else:
            p.setBrush(QColor("#777777"))

        p.setPen(Qt.NoPen)
        p.drawRoundedRect(0, 0, self.width(), self.height(), 12, 12)

        p.setBrush(QColor("white"))
        p.drawEllipse(int(self._offset), 2, self.height() - 4, self.height() - 4)

    def mousePressEvent(self, event):

        if event.button() == Qt.LeftButton:
            self.setChecked(not self.isChecked())
            event.accept()
            return

        super().mousePressEvent(event)


class LabeledToggle(QWidget):

    def __init__(self, text="", parent=None):
        super().__init__(parent)

        self.label = QLabel(text)
        self.toggle = ToggleSwitch()

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.label)
        layout.addStretch()
        layout.addWidget(self.toggle)

    # wygodne API

    def isChecked(self):
        return self.toggle.isChecked()

    def setChecked(self, state):
        
        if state:
            state = True
        else:
            state = False

        self.toggle.setChecked(state)

    @property
    def toggled(self):
        return self.toggle.toggled