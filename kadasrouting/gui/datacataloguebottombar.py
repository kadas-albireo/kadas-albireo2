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

from qgis.core import QgsSettings

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
        self.radioButton.toggled.connect(self.radioButtonToggled)
        layout.addWidget(self.radioButton)
        self.button = QPushButton()
        self.button.clicked.connect(self.buttonClicked)
        self.updateContent()
        layout.addStretch()
        layout.addWidget(self.button)
        self.setLayout(layout)
        self.setStyleSheet("QFrame { background-color: white; }")
        if QgsSettings().value("/kadas/activeValhallaTilesID") == self.data['id']:
            self.radioButton.setChecked(True)

    def updateContent(self):
        statuses = {
            DataCatalogueClient.NOT_INSTALLED: [self.tr("Install"), "black"],
            DataCatalogueClient.UPDATABLE: [self.tr("Update"), "orange"],
            DataCatalogueClient.UP_TO_DATE: [self.tr("Remove"), "green"]
        }
        status = self.data['status']
        date = datetime.datetime.fromtimestamp(self.data['modified'] / 1e3).strftime("%d-%m-%Y")
        self.radioButton.setText(f"{self.data['title']} [{date}]")
        self.radioButton.setStyleSheet(f"color: {statuses[status][1]}; font: bold")
        self.button.setText(statuses[status][0])
        if self.data['id'] == 'default':
            self.button.setEnabled(False)
            self.button.setToolTip(self.tr('Default data tiles can not be removed'))

    def buttonClicked(self):
        status = self.data['status']
        if status == DataCatalogueClient.UP_TO_DATE:
            ret = dataCatalogueClient.uninstall(self.data["id"])
            if not ret:
                pushWarning(
                    self.tr("Cannot remove previous version of tiles for {name}").format(name=self.data['title']))
            else:
                pushMessage(self.tr("Tiles for {name} is successfully deleted").format(name=self.data['title']))
                self.data['status'] = DataCatalogueClient.NOT_INSTALLED
        else:
            ret = dataCatalogueClient.install(self.data)
            if not ret:
                pushWarning(self.tr("Cannot install tiles for {name}").format(name=self.data['title']))
            else:
                pushMessage(self.tr("Tiles for {name} is successfully installed").format(name=self.data['title']))
                self.data['status'] = DataCatalogueClient.UP_TO_DATE

        if ret:
            self.updateContent()

    def radioButtonToggled(self):
        if self.radioButton.isChecked():
            # Update Kadas setting
            QgsSettings().setValue("/kadas/activeValhallaTilesID", self.data['id'])
            pushMessage('Active Valhalla tiles is set to %s' % self.data['title'])


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
        # Add default data tile first
        defaultData = {
                'status': DataCatalogueClient.UP_TO_DATE,
                'title': self.tr('Switzerland - Default'),
                'id': 'default',
                'modified': 1
            }
        dataItems = [defaultData]
        try:
            dataItems.extend(dataCatalogueClient.getAvailableTiles())
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
