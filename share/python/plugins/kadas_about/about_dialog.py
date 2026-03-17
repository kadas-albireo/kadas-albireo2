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

import os

from kadas.kadascore import *
from qgis.core import Qgis
from qgis.PyQt.QtCore import *
from qgis.PyQt.QtGui import *
from qgis.PyQt.QtWidgets import *


class AboutDialog(QDialog):
    def __init__(self, locale, parent):
        QDialog.__init__(self, parent)
        self.setModal(True)

        self.setWindowTitle(self.tr("About %s") % (Kadas.KADAS_FULL_RELEASE_NAME))
        layout = QGridLayout()
        layout.setVerticalSpacing(5)
        self.setLayout(layout)

        splashLabel = QLabel()
        splashLabel.setPixmap(QPixmap(":/kadas/splash"))
        layout.addWidget(splashLabel, layout.rowCount(), 0, 1, 2)

        versionLabel = QLabel(
            self.tr("<b>Version</b>: %s (%s) - Based on QGIS %s")
            % (
                f"{Kadas.KADAS_FULL_RELEASE_NAME} {Kadas.KADAS_VERSION}",
                Kadas.KADAS_BUILD_DATE,
                Qgis.version(),
            )
        )
        versionLabel.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        layout.addWidget(versionLabel, layout.rowCount(), 0, 1, 1)

        licenseLabel = QLabel(
            self.tr(
                'This software is released under the <a href="http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html">GNU Public License (GPL) Version 2</a>'
            )
        )
        licenseLabel.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        licenseLabel.setOpenExternalLinks(True)
        licenseLabel.setWordWrap(True)
        layout.addWidget(licenseLabel, layout.rowCount(), 0, 1, 1)
        mssLabel = QLabel(self.tr("The MSS/MilX components are property of gs-soft AG"))
        mssLabel.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        mssLabel.setWordWrap(True)
        layout.addWidget(mssLabel, layout.rowCount(), 0, 1, 1)

        adminLogo = QLabel()
        adminLogo.setPixmap(QPixmap(os.path.join(os.path.dirname(__file__), "adminch.png")))
        adminLogo.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop)
        layout.addWidget(adminLogo, 2, 1, 3, 1)

        hline = QFrame()
        hline.setFrameShape(QFrame.Shape.HLine)
        hline.setFrameShadow(QFrame.Shadow.Sunken)
        layout.addWidget(hline, layout.rowCount(), 0, 1, 1)

        hline2 = QFrame()
        hline2.setFrameShape(QFrame.Shape.HLine)
        hline2.setFrameShadow(QFrame.Shadow.Sunken)
        layout.addWidget(hline2, layout.rowCount() - 1, 1, 1, 1)

        pdfpath = os.path.join(
            os.path.dirname(__file__),
            "doc",
            "Nutzungsbestimmungen_Daten_swisstopo_KADAS_Albireo_%s.pdf",
        )
        if os.path.isfile(pdfpath % locale):
            lang = locale
        else:
            lang = "de"

        swisstopoDataTermsLink = QLabel(
            '<a href="file:///%s">%s</a>'
            % (pdfpath % lang, self.tr("Terms of use for swisstopo geodata"))
        )
        swisstopoDataTermsLink.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed
        )
        swisstopoDataTermsLink.setOpenExternalLinks(True)
        layout.addWidget(swisstopoDataTermsLink, layout.rowCount(), 0, 1, 1)

        gdiTermsLinkMap = {
            "en": "https://www.geo.admin.ch/en/about-swiss-geoportal/responsabilities-and-contacts.html",
            "de": "https://www.geo.admin.ch/de/ueber-geo-admin/impressum.html",
            "it": "https://www.geo.admin.ch/it/geo-admin-ch/colophon.html",
            "fr": "https://www.geo.admin.ch/fr/geo-admin-ch/impressum.html",
        }
        swissGDIdataTermsLink = QLabel(
            '<a href="%s">%s</a>'
            % (gdiTermsLinkMap[locale], self.tr("Terms of use for Swiss GDI geodata"))
        )
        swissGDIdataTermsLink.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        swissGDIdataTermsLink.setOpenExternalLinks(True)
        layout.addWidget(swissGDIdataTermsLink, layout.rowCount(), 0, 1, 1)

        dtmLinkMap = {
            "en": "https://www.swisstopo.admin.ch/en/height-model-swissaltiregio",
            "de": "https://www.swisstopo.admin.ch/de/hoehenmodell-swissaltiregio",
            "it": "https://www.swisstopo.admin.ch/it/modello-altimetrico-swissaltiregio",
            "fr": "https://www.swisstopo.admin.ch/fr/modele-altimetrique-swissaltiregio",
        }
        dtmLink = QLabel(
            '<a href="%s">%s<a>' % (dtmLinkMap[locale], self.tr("swissALTIRegio usage disclaimer"))
        )
        dtmLink.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        dtmLink.setOpenExternalLinks(True)
        layout.addWidget(dtmLink, layout.rowCount(), 0, 1, 1)

        dataContactLink = QLabel(
            '<a href="mailto:webgis@swisstopo.ch">%s<a>'
            % (self.tr("Data management and controller contact"))
        )
        dataContactLink.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        dataContactLink.setOpenExternalLinks(True)
        layout.addWidget(dataContactLink, layout.rowCount(), 0, 1, 1)

        disclaimerLinkMap = {
            "en": "https://www.admin.ch/gov/en/start/terms-and-conditions.html",
            "de": "https://www.admin.ch/gov/de/start/rechtliches.html",
            "it": "https://www.admin.ch/gov/it/pagina-iniziale/basi-legali.html",
            "fr": "https://www.admin.ch/gov/fr/accueil/conditions-utilisation.html",
        }
        disclaimerLink = QLabel(
            '<a href="%s">%s<a>'
            % (disclaimerLinkMap[locale], self.tr("Data usage liability disclaimer"))
        )
        disclaimerLink.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        disclaimerLink.setOpenExternalLinks(True)
        layout.addWidget(disclaimerLink, layout.rowCount(), 0, 1, 1)

        bbox = QDialogButtonBox(QDialogButtonBox.StandardButton.Close)
        bbox.accepted.connect(self.accept)
        bbox.rejected.connect(self.reject)
        layout.addWidget(bbox, layout.rowCount(), 0, 1, 2)

        self.setFixedSize(self.sizeHint())
