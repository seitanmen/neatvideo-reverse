"""Prepare Profile page: side panel of the profile tools (the preview is shared by both pages)."""
from PySide6.QtWidgets import QScrollArea, QVBoxLayout, QWidget

from ..widgets.profile_panel import ProfilePanel


class ProfilePage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.panel = ProfilePanel()
        sc = QScrollArea()
        sc.setWidgetResizable(True)
        sc.setWidget(self.panel)
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.addWidget(sc)
