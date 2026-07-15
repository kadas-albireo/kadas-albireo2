import math
import os

from kadas.kadascore import KadasCoordinateUtils
from kadas.kadasgui import KadasBottomBar
from qgis.core import (
    Qgis,
    QgsAnnotationLayer,
    QgsCoordinateFormatter,
    QgsCoordinateReferenceSystem,
    QgsCoordinateTransform,
    QgsGeometry,
    QgsMapLayer,
    QgsMarkerSymbol,
    QgsProject,
    QgsSvgMarkerSymbolLayer,
    QgsWkbTypes,
)
from qgis.gui import QgsRubberBand
from qgis.PyQt.QtCore import QDateTime, QEventLoop, Qt, pyqtSignal
from qgis.PyQt.QtGui import QIcon, QPixmap
from qgis.PyQt.QtWidgets import QApplication, QLabel, QSizePolicy
from qgis.PyQt.uic import loadUiType

from .ephem_recompute_task import EphemComputeTask

Ui_EphemToolWidget, _ = loadUiType(os.path.join(os.path.dirname(__file__), "EphemToolWidget.ui"))


class EphemToolWidget(KadasBottomBar):

    close = pyqtSignal()

    def __init__(self, iface):
        KadasBottomBar.__init__(self, iface.mapCanvas())

        self.iface = iface

        self.ui = Ui_EphemToolWidget()
        self.ui.setupUi(self)

        self.ui.closeButton.setSizePolicy(
            QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred
        )
        self.ui.closeButton.setIcon(QIcon(":/kadas/icons/close"))
        self.ui.closeButton.setToolTip(self.tr("Close"))
        self.ui.closeButton.clicked.connect(self.close)

        self.ui.dateTimeEdit.setDateTime(QDateTime.currentDateTime())
        self.ui.dateTimeEdit.editingFinished.connect(self.recompute)
        self.ui.timezoneCombo.addItem(
            self.tr("System time"), EphemComputeTask.TimezoneType.TIMEZONE_SYSTEM
        )
        self.ui.timezoneCombo.addItem(self.tr("UTC"), EphemComputeTask.TimezoneType.TIMEZONE_UTC)
        self.ui.timezoneCombo.addItem(
            self.tr("Local time at position"), EphemComputeTask.TimezoneType.TIMEZONE_LOCAL
        )
        self.ui.timezoneCombo.currentIndexChanged.connect(self.recompute)
        self.ui.checkBoxRelief.toggled.connect(self.recompute)
        self.ui.tabWidgetOutput.setEnabled(False)
        self.ui.tabWidgetOutput.currentChanged.connect(self.recompute)

        self.busyOverlay = QLabel(self.tr("Calculating..."))
        self.busyOverlay.setStyleSheet("QLabel { background-color: white;}")
        self.busyOverlay.setAlignment(
            Qt.AlignmentFlag.AlignHCenter | Qt.AlignmentFlag.AlignVCenter
        )
        self.busyOverlay.setVisible(False)
        self.busyOverlay.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.ui.verticalLayout.addWidget(self.busyOverlay)

        self.wgsPos = None
        self.mrcPos = None

        self.azLayer = QgsAnnotationLayer(
            "azLayer", QgsAnnotationLayer.LayerOptions(QgsProject.instance().transformContext())
        )

        self.azLayer.setCrs(QgsCoordinateReferenceSystem("EPSG:4326"))
        QgsProject.instance().addMapLayer(self.azLayer)

        self.azLayer.setFlags(self.azLayer.flags() | QgsMapLayer.Private)

        self.sunAzIcon = QgsRubberBand(self.iface.mapCanvas(), QgsWkbTypes.PointGeometry)

        az_sun_svg_path = os.path.join(os.path.dirname(__file__), "icons/az_sun.svg")
        sunAzSymbolLayer = QgsSvgMarkerSymbolLayer(az_sun_svg_path)
        sunAzSymbolLayer.setSize(8)
        sunAzSymbolLayer.setAngle(0)
        sunAzSymbolLayer.setHorizontalAnchorPoint(Qgis.HorizontalAnchorPoint.Center)
        sunAzSymbolLayer.setVerticalAnchorPoint(Qgis.VerticalAnchorPoint.Bottom)

        self.sunAzIcon.setSymbol(QgsMarkerSymbol([sunAzSymbolLayer]))
        self.sunAzIcon.setVisible(False)

        self.moonAzIcon = QgsRubberBand(self.iface.mapCanvas(), QgsWkbTypes.PointGeometry)

        az_moon_svg_path = os.path.join(os.path.dirname(__file__), "icons/az_moon.svg")
        moonAzSymbolLayer = QgsSvgMarkerSymbolLayer(az_moon_svg_path)
        moonAzSymbolLayer.setSize(8)
        moonAzSymbolLayer.setAngle(0)
        moonAzSymbolLayer.setHorizontalAnchorPoint(Qgis.HorizontalAnchorPoint.Center)
        moonAzSymbolLayer.setVerticalAnchorPoint(Qgis.VerticalAnchorPoint.Bottom)

        self.moonAzIcon.setSymbol(QgsMarkerSymbol([moonAzSymbolLayer]))
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

        QgsProject.instance().removeMapLayer(self.azLayer)
        del self.azLayer

        self.sunAzIcon.reset()
        self.moonAzIcon.reset()

        self.iface.mapCanvas().refresh()

    def recompute(self):
        self.sunAzIcon.setVisible(False)
        self.moonAzIcon.setVisible(False)

        if not self.wgsPos:
            return

        QApplication.setOverrideCursor(Qt.CursorShape.BusyCursor)

        if self.ui.checkBoxRelief.isChecked():
            self.busyOverlay.setVisible(True)
            self.ui.tabWidgetOutput.setEnabled(False)
            QApplication.instance().processEvents(
                QEventLoop.ProcessEventsFlag.ExcludeUserInputEvents
            )

        self.ui.tabWidgetOutput.setEnabled(True)
        font = self.ui.labelPositionValue.font()
        font.setItalic(False)
        self.ui.labelPositionValue.setFont(font)
        x_str = QgsCoordinateFormatter.formatX(
            self.wgsPos.x(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1
        )
        y_str = QgsCoordinateFormatter.formatY(
            self.wgsPos.y(), QgsCoordinateFormatter.FormatDegreesMinutesSeconds, 1
        )
        self.ui.labelPositionValue.setText(y_str + " " + x_str)
        tz = KadasCoordinateUtils.getTimezoneAtPos(
            self.wgsPos, QgsCoordinateReferenceSystem("EPSG:4326")
        )
        self.ui.labelTimezone.setText(tz.data().decode("utf-8"))

        celestialBody = EphemComputeTask.CelestialBody.MOON
        if self.ui.tabWidgetOutput.currentIndex() == 0:
            celestialBody = EphemComputeTask.CelestialBody.SUN

        self.ephemRecomputeTask.compute(
            wgsPos=self.wgsPos,
            mrcPos=self.mrcPos,
            celestialBody=celestialBody,
            considerRelief=self.ui.checkBoxRelief.isChecked(),
            dateTime=self.ui.dateTimeEdit.dateTime(),
            timezoneType=self.ui.timezoneCombo.currentData(),
        )

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
            moon_image_suffix = math.floor(round(result.moonPhase / 12.5) * 12.5)
            self.ui.labelMoonPhaseIcon.setPixmap(
                QPixmap(
                    os.path.join(os.path.dirname(__file__), f"icons/moon_{moon_image_suffix}.svg")
                )
            )
            self.ui.labelMoonPhaseValue.setText("%.2f%%" % result.moonPhase)

        src_crs = QgsCoordinateReferenceSystem("EPSG:4326")
        dest_crs = self.iface.mapCanvas().mapSettings().destinationCrs()
        ct = QgsCoordinateTransform(src_crs, dest_crs, QgsProject.instance())
        point_geom = QgsGeometry.fromPointXY(result.position)
        point_geom.transform(ct)
        azIcon.setToGeometry(point_geom)

        s = azIcon.symbol().clone()
        s.setAngle(result.angle)
        azIcon.setSymbol(s)

        azIcon.setVisible(result.azimuthVisible)

        self.azLayer.triggerRepaint()
        self.iface.mapCanvas().refresh()

        self.busyOverlay.setVisible(False)
        self.ui.tabWidgetOutput.setEnabled(True)
        QApplication.restoreOverrideCursor()
