
import os

from qgis.PyQt.QtCore import QSettings
from qgis.PyQt.QtGui import QPixmap
from qgis.PyQt.QtWidgets import QDialog, QFileDialog, QDialogButtonBox
from qgis.PyQt.uic import loadUiType

from qgis.core import Qgis, QgsRectangle

# from .kadas_gpkg_layer_list import KadasGpkgLayersList
from kadas.kadasgui import KadasMapToolSelectRect

WidgetUi, _ = loadUiType(
    os.path.join(os.path.dirname(__file__), "kadas_gpkg_export_dialog.ui")
)

class KadasGpkgExportDialog(QDialog, WidgetUi):

    def __init__(self, parent, iface):
        QDialog.__init__(self, parent)
        self.setupUi(self)

        self.iface = iface

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

        self.spinBoxExportScale.setValue(int(iface.mapCanvas().mapSettings().scale()))

        self.buttonSelectFile.clicked.connect(self.__selectOutputFile)
        self.checkBoxClear.toggled.connect(self.__updateLocalLayerList)
        self.checkBoxExportScale.toggled.connect(self.spinBoxExportScale.setEnabled)

        self.mGroupBoxExtent.toggled.connect(self.__extentToggled)

        self.mRectTool = KadasMapToolSelectRect(iface.mapCanvas())
        self.mRectTool.rectChanged.connect(self.__extentChanged)
        iface.mapCanvas().setMapTool(self.mRectTool)

        self.labelCheckIcon.setPixmap(QPixmap(":/images/themes/default/mIconSuccess.svg"))
        self.labelWarnIcon.setPixmap(QPixmap(":/images/themes/default/mIconWarning.svg"))

    def outputFile(self):
        return self.lineEditOutputFile.text()

    def clearOutputFile(self):
        return self.checkBoxClear.isChecked()

    def selectedLayers(self):
        return self.listWidgetLayers.getSelectedLayers()

    def buildPyramids(self):
        return self.checkBoxPyramids.isChecked()

    def rasterExportScale(self):
        return self.spinBoxExportScale.value() if self.checkBoxExportScale.isChecked() else None
    
    def filterExtent(self):
        if not self.mGroupBoxExtent.isChecked():
            return None
        
        return self.mRectTool.rect()
        
    def filterExtentCrs(self):
        if not self.mGroupBoxExtent.isChecked():
            return None
        
        return self.iface.mapCanvas().mapSettings().destinationCrs()
        
    def clear(self):
        self.iface.mapCanvas().unsetMapTool(self.mRectTool)
        self.mRectTool = None

    def __selectOutputFile(self):
        lastDir = QSettings().value("/UI/lastImportExportDir", ".")
        filename = QFileDialog.getSaveFileName(self, self.tr("Select GPKG File..."), lastDir, self.tr("GPKG Database (*.gpkg)"), "", QFileDialog.DontConfirmOverwrite)[0]

        if not filename:
            return

        if not filename.lower().endswith(".gpkg"):
            filename += ".gpkg"

        QSettings().setValue("/UI/lastImportExportDir", os.path.dirname(filename))
        self.lineEditOutputFile.setText(filename)

        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(filename is not None)
        self.__updateLocalLayerList()

    def __updateLocalLayerList(self):
        self.listWidgetLayers.updateLayerList(self.lineEditOutputFile.text() if not self.checkBoxClear.isChecked() else None)

    def __extentToggled(self, checked):
        if checked:
            self.mRectTool.setRect( self.iface.mapCanvas().extent() )
        else:
            self.mRectTool.clear()

    def __extentChanged(self, extent):
        if extent.isNull():
            self.mLineEditXMin.setText("")
            self.mLineEditYMin.setText("")
            self.mLineEditXMax.setText("")
            self.mLineEditYMax.setText("")
        else:
            decs = 0
            if self.iface.mapCanvas().mapSettings().mapUnits() == Qgis.DistanceUnit.Degrees:
                decs = 3
            self.mLineEditXMin.setText(f"{extent.xMinimum(): {decs}f}")
            self.mLineEditYMin.setText(f"{extent.yMinimum(): {decs}f}")
            self.mLineEditXMax.setText(f"{extent.xMaximum(): {decs}f}")
            self.mLineEditYMax.setText(f"{extent.yMaximum(): {decs}f}")

            
