import os
from qgis.core import (
    QgsVectorLayer,
    QgsCoordinateReferenceSystem,
    QgsWkbTypes,
    QgsProject,
    QgsFeature,
    QgsSingleSymbolRenderer,
    QgsGeometry,
)
from PyQt5.QtGui import QColor
from kadasrouting.utilities import transformToWGS


class CanvasLayerSaver():
    def __init__(
                self,
                name,
                features=None,
                crs=QgsCoordinateReferenceSystem(4326),
                color=QColor('lightGray'),
                style=None
            ):
        self.name = name
        self.color = color
        self.transformer = transformToWGS(crs)
        self.features = features
        self.style = style
        self.addPolygonLayer()

    def addPolygonLayer(self):
        try:
            # FIXME: we do not consider if there are several layers with the same name here
            existinglayer = QgsProject.instance().mapLayersByName(self.name)[0]
            QgsProject.instance().removeMapLayer(existinglayer.id())
        except IndexError:
            # if such layer does not exist we can pass over
            pass
        # arbitrary decision taken: WGS84 for these layers
        self.layer = QgsVectorLayer(
            "Polygon?crs=epsg:4326&field=centerx:double&field=centery:double&field=interval:double",
            self.name,
            "memory",
        )
        pr = self.layer.dataProvider()
        if self.features:
            for feature in self.features:
                print(feature)
                if isinstance(feature, QgsFeature):
                    geom = feature.geometry()
                    feature.setGeometry(self.reprojectToWGS84(geom))
                    pr.addFeatures([feature])
                elif isinstance(feature, QgsGeometry):
                    qgsFeature = QgsFeature()
                    qgsFeature.setGeometry(self.reprojectToWGS84(feature))
                    pr.addFeatures([qgsFeature])

        self.layer.updateExtents()
        QgsProject.instance().addMapLayer(self.layer)
        if self.style:
            self.layer.loadNamedStyle(self.style)
        else:
            renderer = QgsSingleSymbolRenderer.defaultRenderer(QgsWkbTypes.PolygonGeometry)
            symbol = renderer.symbol()
            symbol.setColor(self.color)
            symbol.symbolLayer(0).setStrokeColor(self.color)
            symbol.symbolLayer(0).setFillColor(QColor(0, 0, 0, 0))
            symbol.symbolLayer(0).setStrokeWidth(0.5)
            self.layer.setRenderer(renderer)

    def reprojectToWGS84(self, geom):
        geomType = geom.type()
        if geomType == QgsWkbTypes.LineGeometry:
            geomList = geom.asPolyline()
        elif geomType == QgsWkbTypes.PolygonGeometry:
            geomList = geom.asPolygon()
        newGeom = []
        for j in range(len(geomList)):
            if geomType == QgsWkbTypes.LineGeometry:
                newGeom.append(self.transformer.transform(geomList[j]))
            elif geomType == QgsWkbTypes.PolygonGeometry:
                line = geomList[j]
                for i in range(len(line)):
                    point = line[i]
                    newGeom.append(self.transformer.transform(point))
        if geomType == QgsWkbTypes.LineGeometry:
            return QgsGeometry.fromPolylineXY(newGeom)
        elif geomType == QgsWkbTypes.PolygonGeometry:
            return QgsGeometry.fromPolygonXY([newGeom])
