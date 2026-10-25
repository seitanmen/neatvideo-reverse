"""Filter Settings page: accordion of the filter sections (the preview is shared by both pages)."""
from PySide6.QtWidgets import QScrollArea, QVBoxLayout, QWidget

from ..widgets.filter_panel import FilterPanel


class FilterPage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.panel = FilterPanel()
        sc = QScrollArea()
        sc.setWidgetResizable(True)
        sc.setWidget(self.panel)
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.addWidget(sc)
