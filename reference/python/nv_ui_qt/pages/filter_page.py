"""Adjust and Preview page: the Filter Settings side panel (the viewer is shared by both pages)."""
from PySide6.QtWidgets import QVBoxLayout, QWidget

from ..widgets.filter_panel import FilterPanel


class FilterPage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.panel = FilterPanel()
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.addWidget(self.panel)
