from PySide6.QtWidgets import QWidget
from PySide6.QtGui import QColor, QPainter, QRadialGradient, QPen


class StatusLED(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.setFixedSize(14, 14)
        self._connected = False

    def set_state(self, state):
        self._connected = state
        self.update()

    def paintEvent(self, event):

        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        rect = self.rect().adjusted(1, 1, -1, -1)

        if self._connected:
            base = QColor(80, 200, 120)   # miękka zieleń
        else:
            base = QColor(220, 90, 90)    # przyjemna czerwień

        gradient = QRadialGradient(
            rect.center(),
            rect.width() / 2
        )

        gradient.setColorAt(0.0, base.lighter(150))
        gradient.setColorAt(0.7, base)
        gradient.setColorAt(1.0, base.darker(160))

        painter.setBrush(gradient)

        pen = QPen(base.darker(200))
        pen.setWidth(1)
        painter.setPen(pen)

        painter.drawEllipse(rect)

