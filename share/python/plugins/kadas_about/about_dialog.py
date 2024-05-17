# -*- coding: utf-8 -*-
"""
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""

from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *
from qgis.core import Qgis
from kadas.kadascore import *
import os
from . import resources


class AboutDialog(QDialog):
    def __init__(self, locale, parent):
        QDialog.__init__(self, parent)
        self.setModal(True)

        self.setWindowTitle(self.tr("About %s") % (Kadas.KADAS_FULL_RELEASE_NAME))
        l = QGridLayout()
        l.setVerticalSpacing(5)
        self.setLayout(l)

        splashLabel = QLabel()
        splashLabel.setPixmap(QPixmap(":/plugins/KadasHelp/splash.jpg"))
        l.addWidget(splashLabel, l.rowCount(), 0, 1, 2)

        versionLabel = QLabel(self.tr("<b>Version</b>: %s (%s) - Based on QGIS %s") % (Kadas.KADAS_VERSION, Kadas.KADAS_BUILD_DATE, Qgis.version()))
        versionLabel.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        l.addWidget(versionLabel, l.rowCount(), 0, 1, 1)

        licenseLabel = QLabel(self.tr("This software is released under the <a href=\"http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html\">GNU Public License (GPL) Version 2</a>"))
        licenseLabel.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        licenseLabel.setOpenExternalLinks(True)
        licenseLabel.setWordWrap(True)
        l.addWidget(licenseLabel, l.rowCount(), 0, 1, 1)

        bbox = QDialogButtonBox(QDialogButtonBox.Close)
        bbox.accepted.connect(self.accept)
        bbox.rejected.connect(self.reject)
        l.addWidget(bbox, l.rowCount(), 0, 1, 2)

        self.setFixedSize(self.sizeHint())
