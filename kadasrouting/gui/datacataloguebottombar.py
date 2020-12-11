import os
import datetime
import logging

from PyQt5 import uic
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import (
    QHBoxLayout,
    QFrame,
    QPushButton,
    QListWidgetItem,
    QRadioButton,
    QButtonGroup
)

from qgis.core import QgsSettings

from kadas.kadasgui import KadasBottomBar

from kadasrouting.utilities import pushWarning, pushMessage, icon
from kadasrouting.core.datacatalogueclient import (
    DataCatalogueClient,
    DEFAULT_REPOSITORY_URLS,
    DEFAULT_ACTIVE_REPOSITORY_URL
)

LOG = logging.getLogger(__name__)

WIDGET, BASE = uic.loadUiType(os.path.join(os.path.dirname(__file__), "datacataloguebottombar.ui"))


class DataItem(QListWidgetItem):

    def __init__(self, data):
        super().__init__()
        self.data = data


class DataItemWidget(QFrame):

    def __init__(self, data, data_catalogue_client):
        QFrame.__init__(self)
        self.data = data
        self.dataCatalogueClient = data_catalogue_client
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
        if QgsSettings().value("/kadasrouting/activeValhallaTilesID", 'default') == self.data['id']:
            self.radioButton.setChecked(True)

    def updateContent(self):
        statuses = {
            DataCatalogueClient.NOT_INSTALLED: [self.tr("Install"), "black", "bold"],
            DataCatalogueClient.UPDATABLE: [self.tr("Update"), "orange", "bold"],
            DataCatalogueClient.UP_TO_DATE: [self.tr("Remove"), "green", "bold"],
            DataCatalogueClient.LOCAL_ONLY: [self.tr("Remove"), "green", "bold italic"],
            DataCatalogueClient.LOCAL_DELETED: [self.tr("N/A"), "black", "italic"],
        }
        status = self.data['status']
        date = datetime.datetime.fromtimestamp(self.data['modified'] / 1e3).strftime("%d-%m-%Y")
        self.radioButton.setText(f"{self.data['title']} [{date}]")
        self.radioButton.setStyleSheet(f"color: {statuses[status][1]}; font: {statuses[status][2]}")
        self.button.setText(statuses[status][0])
        if status == DataCatalogueClient.LOCAL_ONLY:
            self.button.setToolTip(self.tr(
                'This map package is local only, if you delete it you can not download it from the selected URL'))
        if status == DataCatalogueClient.LOCAL_DELETED:
            self.button.setEnabled(False)
            self.button.hide()
            self.radioButton.setStyleSheet(
                f"color: {statuses[status][1]}; font: {statuses[status][2]}; text-decoration: line-through")
        # Add addditional behaviour for radio button according to installation status
        if status == DataCatalogueClient.NOT_INSTALLED or status == DataCatalogueClient.LOCAL_DELETED:
            self.radioButton.setDisabled(True)
            self.radioButton.setToolTip(self.tr('Map package has to be installed first'))
        else:
            self.radioButton.setDisabled(False)
            self.radioButton.setToolTip('')

    def buttonClicked(self):
        status = self.data['status']
        if status == DataCatalogueClient.UP_TO_DATE:
            ret = self.dataCatalogueClient.uninstall(self.data["id"])
            if not ret:
                pushWarning(
                    self.tr("Cannot remove previous version of the {name} map package").format(
                        name=self.data['title']))
            else:
                pushMessage(self.tr("Map package {name} has been successfully deleted ").format(
                    name=self.data['title']))
                self.data['status'] = DataCatalogueClient.NOT_INSTALLED
        elif status == DataCatalogueClient.LOCAL_ONLY:
            ret = self.dataCatalogueClient.uninstall(self.data["id"])
            if not ret:
                pushWarning(
                    self.tr("Cannot remove previous version of the {name} map package").format(
                        name=self.data['title']))
            else:
                pushMessage(self.tr("Map package {name} has been successfully deleted ").format(
                    name=self.data['title']))
                self.data['status'] = DataCatalogueClient.LOCAL_DELETED
        else:
            ret = self.dataCatalogueClient.install(self.data)
            if not ret:
                pushWarning(self.tr("Cannot install map package {name}").format(name=self.data['title']))
            else:
                pushMessage(self.tr("Map package {name} has been successfully installed ").format(
                    name=self.data['title']))
                self.data['status'] = DataCatalogueClient.UP_TO_DATE

        if ret:
            self.updateContent()

    def radioButtonToggled(self):
        if self.radioButton.isChecked():
            # Update Kadas setting
            QgsSettings().setValue("/kadasrouting/activeValhallaTilesID", self.data['id'])
            pushMessage(self.tr('Active map package is set to {tile}').format(tile=self.data['title']))


class DataCatalogueBottomBar(KadasBottomBar, WIDGET):

    def __init__(self, canvas, action):
        KadasBottomBar.__init__(self, canvas, "orange")
        self.setupUi(self)
        self.setStyleSheet("QFrame { background-color: orange; }")
        self.listWidget.setStyleSheet("QListWidget { background-color: white; }")
        self.radioButtonGroup = QButtonGroup(self)
        self.action = action
        # Close button
        self.btnClose.setIcon(QIcon(":/kadas/icons/close"))
        self.btnClose.setToolTip(self.tr("Close data catalogue dialog"))
        self.btnClose.clicked.connect(self.action.toggle)
        # Reload button
        self.reloadRepositoryButton.setIcon(icon("reload.png"))
        self.reloadRepositoryButton.setToolTip(self.tr("Reload data catalogue with the selected repository"))
        self.reloadRepositoryButton.clicked.connect(self.reloadRepository)

        # data catalogue client
        self.dataCatalogueClient = None

        # Repository URLs combo box
        self.repoUrlComboBox.setEditable(True)
        self.populateListRepositoryURLs()
        self.reloadRepository()

    def populateList(self):
        dataItems = self.dataCatalogueClient.getTiles()

        # Clear first before populating (in case failed request, the list is still there)
        self.listWidget.clear()
        for data in dataItems:
            item = DataItem(data)
            widget = DataItemWidget(data, self.dataCatalogueClient)
            item.setSizeHint(widget.sizeHint())
            self.listWidget.addItem(item)
            self.listWidget.setItemWidget(item, widget)
            self.radioButtonGroup.addButton(widget.radioButton)
        return True

    def populateListRepositoryURLs(self):
        self.repoUrlComboBox.clear()
        active_repository_url = QgsSettings().value(
            "/kadasrouting/active_repository_url", DEFAULT_ACTIVE_REPOSITORY_URL)
        repository_urls = list(DEFAULT_REPOSITORY_URLS)
        if active_repository_url not in repository_urls:
            repository_urls.append(active_repository_url)

        self.repoUrlComboBox.addItems(repository_urls)
        self.repoUrlComboBox.setCurrentText(active_repository_url)

    def reloadRepository(self):
        # Update the list
        active_repository_url = self.repoUrlComboBox.currentText()
        self.dataCatalogueClient = DataCatalogueClient(active_repository_url)
        success = self.populateList()
        # Store the active repository URL
        if success:
            QgsSettings().setValue("/kadasrouting/active_repository_url", active_repository_url)

    def show(self):
        KadasBottomBar.show(self)
        self.populateListRepositoryURLs()
