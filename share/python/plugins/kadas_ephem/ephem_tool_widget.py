
import math
import os

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.PyQt.uic import loadUiType
from qgis.core import *
from qgis.gui import *
from kadas.kadascore import *
from kadas.kadasgui import *
from kadas.kadasanalysis import *

from .ephem_recompute_task import EphemComputeTask


Ui_EphemToolWidget, _ = loadUiType(
    os.path.join(os.path.dirname(__file__), "EphemToolWidget.ui")
)

class EphemToolWidget(KadasBottomBar):

    close = pyqtSignal()

    def __init__(self, iface):
        KadasBottomBar.__init__(self, iface.mapCanvas())

        self.iface = iface

        self.ui = Ui_EphemToolWidget()
        self.ui.setupUi(self)

        self.ui.closeButton.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        self.ui.closeButton.setIcon(QIcon(":/kadas/icons/close"))
        self.ui.closeButton.setToolTip(self.tr("Close"))
        self.ui.closeButton.clicked.connect(self.close)

        self.ui.dateTimeEdit.setDateTime(QDateTime.currentDateTime())
        self.ui.dateTimeEdit.editingFinished.connect(self.recompute)
        self.ui.timezoneCombo.addItem(self.tr("System time"), EphemComputeTask.TimezoneType.TIMEZONE_SYSTEM)
        self.ui.timezoneCombo.addItem(self.tr("UTC"), EphemComputeTask.TimezoneType.TIMEZONE_UTC)
        self.ui.timezoneCombo.addItem(self.tr("Local time at position"), EphemComputeTask.TimezoneType.TIMEZONE_LOCAL)
        self.ui.timezoneCombo.currentIndexChanged.connect(self.recompute)
        self.ui.checkBoxRelief.toggled.connect(self.recompute)
        self.ui.tabWidgetOutput.setEnabled(False)
        self.ui.tabWidgetOutput.currentChanged.connect(self.recompute)

        self.busyOverlay = QLabel(self.tr("Calculating..."))
        self.busyOverlay.setStyleSheet("QLabel { background-color: white;}")
        self.busyOverlay.setAlignment(Qt.AlignHCenter|Qt.AlignVCenter)
        self.busyOverlay.setVisible(False)
        self.busyOverlay.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.ui.verticalLayout.addWidget(self.busyOverlay)

        self.wgsPos = None
        self.mrcPos = None

        self.sunAzIcon = KadasSymbolItem(QgsCoordinateReferenceSystem("EPSG:4326"))
        self.sunAzIcon.setup( os.path.join(os.path.dirname(__file__), "icons/az_sun.svg"), 0.5, 1.0 )
        self.sunAzIcon.setVisible(False)
        KadasMapCanvasItemManager.addItem(self.sunAzIcon)

        self.moonAzIcon = KadasSymbolItem(QgsCoordinateReferenceSystem("EPSG:4326"))
        self.moonAzIcon.setup( os.path.join(os.path.dirname(__file__), "icons/az_moon.svg"), 0.5, 1.0 )
        KadasMapCanvasItemManager.addItem(self.moonAzIcon)
        self.moonAzIcon.setVisible(False)

        self.ephemRecomputeTask = EphemComputeTask(self)
        self.ephemRecomputeTask.finished.connect(self.recomputeFinished)

    def setPos(self, wgsPos, mrcPos):
        self.wgsPos = wgsPos
        self.mrcPos = mrcPos

    def cleanup(self):

        if self.ephemRecomputeTask.isRunning():
            self.ephemRecomputeTask.cancel()
            self.ephemRecomputeTask.wait()
            QApplication.restoreOverrideCursor()

        KadasMapCanvasItemManager.removeItem(self.sunAzIcon)
        self.sunAzIcon = None
        KadasMapCanvasItemManager.removeItem(self.moonAzIcon)
        self.moonAzIcon = None

    def recompute(self):
        self.sunAzIcon.setVisible(False)
        self.moonAzIcon.setVisible(False)

        if not self.wgsPos:
            return

        QApplication.setOverrideCursor(Qt.BusyCursor)

        if self.ui.checkBoxRelief.isChecked():
            self.busyOverlay.setVisible(True)
            self.ui.tabWidgetOutput.setEnabled(False)
            QApplication.instance().processEvents(QEventLoop.ExcludeUserInputEvents)

        self.ui.tabWidgetOutput.setEnabled(True)
        font = self.ui.labelPositionValue.font()
        font.setItalic(False)
        self.ui.labelPositionValue.setFont(font)
        x_str = QgsCoordinateFormatter.formatX(self.wgsPos.x(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1)
        y_str = QgsCoordinateFormatter.formatY(self.wgsPos.y(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1)
        self.ui.labelPositionValue.setText(y_str + " " + x_str)
        tz = KadasCoordinateUtils.getTimezoneAtPos(self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326"))
        self.ui.labelTimezone.setText(tz.data().decode('utf-8'))

        celestialBody = EphemComputeTask.CelestialBody.MOON
        if self.ui.tabWidgetOutput.currentIndex() == 0:
            celestialBody = EphemComputeTask.CelestialBody.SUN

        self.ephemRecomputeTask.compute(wgsPos=self.wgsPos, 
                                        celestialBody=celestialBody,
                                        considerRelief=self.ui.checkBoxRelief.isChecked(), 
                                        dateTime=self.ui.dateTimeEdit.dateTime(),
                                        timezoneType=self.ui.timezoneCombo.currentData())




    def recomputeFinished(self):
        result = self.ephemRecomputeTask.getResult()

        if result.celestialBody == EphemComputeTask.CelestialBody.SUN:
            azIcon = self.sunAzIcon
            
            self.ui.labelAzimuthElevationValue.setText(result.azimuthElevationValueText)
            self.ui.labelSunRiseValue.setText(result.riseValueText)
            self.ui.labelSunSetValue.setText(result.setValueText)
            self.ui.labelZenithValue.setText(result.zenithValueText)

        else:
            azIcon = self.moonAzIcon

            self.ui.labelMoonAzimuthElevationValue.setText(result.azimuthElevationValueText)
            self.ui.labelMoonRiseValue.setText(result.riseValueText)
            self.ui.labelMoonSetValue

            # Moon phase
            moon_image_suffix = math.floor(round(result.moonPhase/12.5)*12.5)
            self.ui.labelMoonPhaseIcon.setPixmap(QPixmap(os.path.join(os.path.dirname(__file__), f"icons/moon_{moon_image_suffix}.svg")))
            self.ui.labelMoonPhaseValue.setText("%.2f%%" % result.moonPhase)

        azIcon.setVisible(result.azimuthVisible)
        azIcon.setPosition(KadasItemPos.fromPoint(result.position))
        azIcon.setAngle(result.angle)
        
        self.busyOverlay.setVisible(False)
        self.ui.tabWidgetOutput.setEnabled(True)
        QApplication.restoreOverrideCursor()
