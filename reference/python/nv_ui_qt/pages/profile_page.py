"""Prepare Noise Profile page: the Device Noise Profile side panel (the viewer is shared by both pages)."""
from PySide6.QtWidgets import QVBoxLayout, QWidget

from ..widgets.profile_panel import ProfilePanel


class ProfilePage(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.panel = ProfilePanel()
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.addWidget(self.panel)
