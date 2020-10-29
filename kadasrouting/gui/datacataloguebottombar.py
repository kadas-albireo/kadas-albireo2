import os
import datetime
import logging

from kadasrouting.core.datacatalogueclient import (
    dataCatalogueClient,
    DataCatalogueClient
)

from kadasrouting.utilities import pushWarning, pushMessage

from kadas.kadasgui import (
    KadasBottomBar,
)

from PyQt5.QtGui import (
    QIcon
)

from PyQt5.QtWidgets import (
    QHBoxLayout,
    QLabel,
    QFrame,
    QPushButton,
    QListWidgetItem,
    QRadioButton,
    QButtonGroup
)

from PyQt5 import uic

LOG = logging.getLogger(__name__)


WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "datacataloguebottombar.ui")
)


class DataItem(QListWidgetItem):

    def __init__(self, data):
        super().__init__()
        self.data = data


class DataItemWidget(QFrame):

    def __init__(self, data):
        QFrame.__init__(self)
        self.data = data
        layout = QHBoxLayout()
        layout.setMargin(0)
        self.radioButton = QRadioButton()
        layout.addWidget(self.radioButton)
        self.label = QLabel()
        layout.addWidget(self.label)
        self.button = QPushButton()
        self.button.clicked.connect(self.buttonClicked)
        self.updateContent()
        layout.addStretch()
        layout.addWidget(self.button)
        self.setLayout(layout)
        self.setStyleSheet("QFrame { background-color: white; }")

    def updateContent(self):
        statuses = {
            DataCatalogueClient.NOT_INSTALLED: [self.tr("Install"), "black"],
            DataCatalogueClient.UPDATABLE: [self.tr("Update"), "orange"],
            DataCatalogueClient.UP_TO_DATE: [self.tr("Remove"), "green"]
        }
        status = self.data['status']
        date = datetime.datetime.fromtimestamp(self.data['modified'] / 1e3).strftime("%d-%m-%Y")
        self.label.setText(f"<b>{self.data['title']} [{date}]</b>")
        self.label.setStyleSheet(f"color: {statuses[status][1]}")
        self.button.setText(statuses[status][0])

    def buttonClicked(self):
        status = self.data['status']
        if status == DataCatalogueClient.UP_TO_DATE:
            ret = dataCatalogueClient.uninstall(self.data["id"])
            if not ret:
                pushWarning(self.tr("Cannot remove previous version of data"))
            else:
                pushMessage(self.tr("Data is successfully removed"))
                self.data['status'] = DataCatalogueClient.NOT_INSTALLED
        else:
            ret = dataCatalogueClient.install(self.data)
            if not ret:
                pushWarning(self.tr("Cannot install data"))
            else:
                pushMessage(self.tr("Data is successfully installed"))
                self.data['status'] = DataCatalogueClient.UP_TO_DATE

        if ret:
            self.updateContent()


class DataCatalogueBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.listWidget.setStyleSheet("QListWidget { background-color: white; }")
        self.action = action
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip(self.tr("Close data catalogue dialog"))
        self.btnClose.clicked.connect(self.action.toggle)
        self.radioButtonGroup = QButtonGroup(self)
        self.populateList()

    def populateList(self):
        LOG.debug('populating list')
        self.listWidget.clear()
        try:
            dataItems = dataCatalogueClient.getAvailableTiles()
        except Exception as e:
            pushWarning(str(e))
            return

        for data in dataItems:
            item = DataItem(data)
            widget = DataItemWidget(data)
            item.setSizeHint(widget.sizeHint())
            self.listWidget.addItem(item)
            self.listWidget.setItemWidget(item, widget)
            self.radioButtonGroup.addButton(widget.radioButton)
