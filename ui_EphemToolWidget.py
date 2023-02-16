# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'EphemToolWidget.ui'
#
# Created by: PyQt5 UI code generator 5.15.9
#
# WARNING: Any manual changes made to this file will be lost when pyuic5 is
# run again.  Do not edit this file unless you know what you are doing.


from PyQt5 import QtCore, QtGui, QtWidgets


class Ui_EphemToolWidget(object):
    def setupUi(self, EphemToolWidget):
        EphemToolWidget.setObjectName("EphemToolWidget")
        EphemToolWidget.resize(788, 330)
        self.verticalLayout = QtWidgets.QVBoxLayout(EphemToolWidget)
        self.verticalLayout.setContentsMargins(0, 0, 0, 0)
        self.verticalLayout.setObjectName("verticalLayout")
        self.widgetHeader = QtWidgets.QWidget(EphemToolWidget)
        self.widgetHeader.setObjectName("widgetHeader")
        self.horizontalLayout = QtWidgets.QHBoxLayout(self.widgetHeader)
        self.horizontalLayout.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.label_2 = QtWidgets.QLabel(self.widgetHeader)
        self.label_2.setObjectName("label_2")
        self.horizontalLayout.addWidget(self.label_2)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.label = QtWidgets.QLabel(self.widgetHeader)
        self.label.setObjectName("label")
        self.horizontalLayout.addWidget(self.label)
        self.dateTimeEdit = QtWidgets.QDateTimeEdit(self.widgetHeader)
        self.dateTimeEdit.setObjectName("dateTimeEdit")
        self.horizontalLayout.addWidget(self.dateTimeEdit)
        self.timezoneCombo = QtWidgets.QComboBox(self.widgetHeader)
        self.timezoneCombo.setObjectName("timezoneCombo")
        self.horizontalLayout.addWidget(self.timezoneCombo)
        self.checkBoxRelief = QtWidgets.QCheckBox(self.widgetHeader)
        self.checkBoxRelief.setChecked(True)
        self.checkBoxRelief.setObjectName("checkBoxRelief")
        self.horizontalLayout.addWidget(self.checkBoxRelief)
        self.verticalLayout.addWidget(self.widgetHeader)
        self.line = QtWidgets.QFrame(EphemToolWidget)
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setFrameShadow(QtWidgets.QFrame.Sunken)
        self.line.setObjectName("line")
        self.verticalLayout.addWidget(self.line)
        self.widgetInfo = QtWidgets.QWidget(EphemToolWidget)
        self.widgetInfo.setObjectName("widgetInfo")
        self.horizontalLayout_2 = QtWidgets.QHBoxLayout(self.widgetInfo)
        self.horizontalLayout_2.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.labelPosition = QtWidgets.QLabel(self.widgetInfo)
        font = QtGui.QFont()
        font.setBold(True)
        font.setWeight(75)
        self.labelPosition.setFont(font)
        self.labelPosition.setObjectName("labelPosition")
        self.horizontalLayout_2.addWidget(self.labelPosition)
        self.labelPositionValue = QtWidgets.QLabel(self.widgetInfo)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.labelPositionValue.sizePolicy().hasHeightForWidth())
        self.labelPositionValue.setSizePolicy(sizePolicy)
        font = QtGui.QFont()
        font.setItalic(True)
        self.labelPositionValue.setFont(font)
        self.labelPositionValue.setObjectName("labelPositionValue")
        self.horizontalLayout_2.addWidget(self.labelPositionValue)
        self.label_4 = QtWidgets.QLabel(self.widgetInfo)
        self.label_4.setObjectName("label_4")
        self.horizontalLayout_2.addWidget(self.label_4)
        self.labelTimezone = QtWidgets.QLabel(self.widgetInfo)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.labelTimezone.sizePolicy().hasHeightForWidth())
        self.labelTimezone.setSizePolicy(sizePolicy)
        self.labelTimezone.setObjectName("labelTimezone")
        self.horizontalLayout_2.addWidget(self.labelTimezone)
        self.verticalLayout.addWidget(self.widgetInfo)
        self.tabWidgetOutput = QtWidgets.QTabWidget(EphemToolWidget)
        self.tabWidgetOutput.setStyleSheet("QWidget { background-color: white; }")
        self.tabWidgetOutput.setObjectName("tabWidgetOutput")
        self.tabSun = QtWidgets.QWidget()
        self.tabSun.setStyleSheet("QWidget { background-color: white; }")
        self.tabSun.setObjectName("tabSun")
        self.gridLayout_2 = QtWidgets.QGridLayout(self.tabSun)
        self.gridLayout_2.setVerticalSpacing(2)
        self.gridLayout_2.setObjectName("gridLayout_2")
        self.labelSunRiseValue = QtWidgets.QLabel(self.tabSun)
        self.labelSunRiseValue.setText("")
        self.labelSunRiseValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelSunRiseValue.setObjectName("labelSunRiseValue")
        self.gridLayout_2.addWidget(self.labelSunRiseValue, 0, 1, 1, 1)
        self.labelZenithIcon = QtWidgets.QLabel(self.tabSun)
        self.labelZenithIcon.setPixmap(QtGui.QPixmap(":/plugins/Ephem/icons/midday.svg"))
        self.labelZenithIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.labelZenithIcon.setObjectName("labelZenithIcon")
        self.gridLayout_2.addWidget(self.labelZenithIcon, 1, 2, 1, 1)
        self.labelSunAzimuthElevation = QtWidgets.QLabel(self.tabSun)
        self.labelSunAzimuthElevation.setObjectName("labelSunAzimuthElevation")
        self.gridLayout_2.addWidget(self.labelSunAzimuthElevation, 7, 0, 1, 1)
        self.labelZenithValue = QtWidgets.QLabel(self.tabSun)
        self.labelZenithValue.setText("")
        self.labelZenithValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelZenithValue.setObjectName("labelZenithValue")
        self.gridLayout_2.addWidget(self.labelZenithValue, 1, 1, 1, 1)
        self.labelZeith = QtWidgets.QLabel(self.tabSun)
        self.labelZeith.setObjectName("labelZeith")
        self.gridLayout_2.addWidget(self.labelZeith, 1, 0, 1, 1)
        self.labelSunRise = QtWidgets.QLabel(self.tabSun)
        self.labelSunRise.setObjectName("labelSunRise")
        self.gridLayout_2.addWidget(self.labelSunRise, 0, 0, 1, 1)
        self.labelSunRiseIcon = QtWidgets.QLabel(self.tabSun)
        self.labelSunRiseIcon.setText("")
        self.labelSunRiseIcon.setPixmap(QtGui.QPixmap(":/plugins/Ephem/icons/sunrise.svg"))
        self.labelSunRiseIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.labelSunRiseIcon.setObjectName("labelSunRiseIcon")
        self.gridLayout_2.addWidget(self.labelSunRiseIcon, 0, 2, 1, 1)
        self.labelSunSet = QtWidgets.QLabel(self.tabSun)
        self.labelSunSet.setObjectName("labelSunSet")
        self.gridLayout_2.addWidget(self.labelSunSet, 2, 0, 1, 1)
        self.labelSunSetValue = QtWidgets.QLabel(self.tabSun)
        self.labelSunSetValue.setText("")
        self.labelSunSetValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelSunSetValue.setObjectName("labelSunSetValue")
        self.gridLayout_2.addWidget(self.labelSunSetValue, 2, 1, 1, 1)
        self.labelSunSetIcon = QtWidgets.QLabel(self.tabSun)
        self.labelSunSetIcon.setPixmap(QtGui.QPixmap(":/plugins/Ephem/icons/sunset.svg"))
        self.labelSunSetIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.labelSunSetIcon.setObjectName("labelSunSetIcon")
        self.gridLayout_2.addWidget(self.labelSunSetIcon, 2, 2, 1, 1)
        self.labelAzimuthElevationValue = QtWidgets.QLabel(self.tabSun)
        self.labelAzimuthElevationValue.setText("")
        self.labelAzimuthElevationValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelAzimuthElevationValue.setObjectName("labelAzimuthElevationValue")
        self.gridLayout_2.addWidget(self.labelAzimuthElevationValue, 7, 1, 1, 2)
        self.tabWidgetOutput.addTab(self.tabSun, "")
        self.tabMoon = QtWidgets.QWidget()
        self.tabMoon.setObjectName("tabMoon")
        self.gridLayout_3 = QtWidgets.QGridLayout(self.tabMoon)
        self.gridLayout_3.setVerticalSpacing(2)
        self.gridLayout_3.setObjectName("gridLayout_3")
        self.labelMoonRise = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonRise.setObjectName("labelMoonRise")
        self.gridLayout_3.addWidget(self.labelMoonRise, 0, 0, 1, 1)
        self.labelMoonRiseValue = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonRiseValue.setText("")
        self.labelMoonRiseValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelMoonRiseValue.setObjectName("labelMoonRiseValue")
        self.gridLayout_3.addWidget(self.labelMoonRiseValue, 0, 1, 1, 1)
        self.labelMoonRiseIcon = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonRiseIcon.setText("")
        self.labelMoonRiseIcon.setPixmap(QtGui.QPixmap(":/plugins/Ephem/icons/moonrise.svg"))
        self.labelMoonRiseIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.labelMoonRiseIcon.setObjectName("labelMoonRiseIcon")
        self.gridLayout_3.addWidget(self.labelMoonRiseIcon, 0, 2, 1, 1)
        self.labelMoonPhase = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonPhase.setObjectName("labelMoonPhase")
        self.gridLayout_3.addWidget(self.labelMoonPhase, 1, 0, 1, 1)
        self.labelMoonPhaseValue = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonPhaseValue.setText("")
        self.labelMoonPhaseValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelMoonPhaseValue.setObjectName("labelMoonPhaseValue")
        self.gridLayout_3.addWidget(self.labelMoonPhaseValue, 1, 1, 1, 1)
        self.labelMoonSet = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonSet.setObjectName("labelMoonSet")
        self.gridLayout_3.addWidget(self.labelMoonSet, 2, 0, 1, 1)
        self.labelMoonSetValue = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonSetValue.setText("")
        self.labelMoonSetValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelMoonSetValue.setObjectName("labelMoonSetValue")
        self.gridLayout_3.addWidget(self.labelMoonSetValue, 2, 1, 1, 1)
        self.labelMoonSetIcon = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonSetIcon.setPixmap(QtGui.QPixmap(":/plugins/Ephem/icons/moonset.svg"))
        self.labelMoonSetIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.labelMoonSetIcon.setObjectName("labelMoonSetIcon")
        self.gridLayout_3.addWidget(self.labelMoonSetIcon, 2, 2, 1, 1)
        self.labelMoonAzimuthElevation = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonAzimuthElevation.setObjectName("labelMoonAzimuthElevation")
        self.gridLayout_3.addWidget(self.labelMoonAzimuthElevation, 3, 0, 1, 1)
        self.labelMoonAzimuthElevationValue = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonAzimuthElevationValue.setText("")
        self.labelMoonAzimuthElevationValue.setTextInteractionFlags(QtCore.Qt.LinksAccessibleByMouse|QtCore.Qt.TextSelectableByKeyboard|QtCore.Qt.TextSelectableByMouse)
        self.labelMoonAzimuthElevationValue.setObjectName("labelMoonAzimuthElevationValue")
        self.gridLayout_3.addWidget(self.labelMoonAzimuthElevationValue, 3, 1, 1, 2)
        self.labelMoonPhaseIcon = QtWidgets.QLabel(self.tabMoon)
        self.labelMoonPhaseIcon.setPixmap(QtGui.QPixmap(":/plugins/Ephem/icons/moon_0.svg"))
        self.labelMoonPhaseIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.labelMoonPhaseIcon.setObjectName("labelMoonPhaseIcon")
        self.gridLayout_3.addWidget(self.labelMoonPhaseIcon, 1, 2, 1, 1)
        self.tabWidgetOutput.addTab(self.tabMoon, "")
        self.verticalLayout.addWidget(self.tabWidgetOutput)
        self.label_3 = QtWidgets.QLabel(EphemToolWidget)
        self.label_3.setWordWrap(True)
        self.label_3.setObjectName("label_3")
        self.verticalLayout.addWidget(self.label_3)

        self.retranslateUi(EphemToolWidget)
        self.tabWidgetOutput.setCurrentIndex(0)
        QtCore.QMetaObject.connectSlotsByName(EphemToolWidget)

    def retranslateUi(self, EphemToolWidget):
        _translate = QtCore.QCoreApplication.translate
        self.label_2.setText(_translate("EphemToolWidget", "<html><head/><body><p><span style=\" font-weight:600;\">Ephemeris</span></p></body></html>"))
        self.label.setText(_translate("EphemToolWidget", "Date and time:"))
        self.checkBoxRelief.setText(_translate("EphemToolWidget", "Consider relief"))
        self.labelPosition.setText(_translate("EphemToolWidget", "Position:"))
        self.labelPositionValue.setText(_translate("EphemToolWidget", "Click on the map to select a position"))
        self.label_4.setText(_translate("EphemToolWidget", "<html><head/><body><p><span style=\" font-weight:600;\">Timezone:</span></p></body></html>"))
        self.labelTimezone.setText(_translate("EphemToolWidget", "-"))
        self.labelSunAzimuthElevation.setText(_translate("EphemToolWidget", "Azimuth / Elevation:"))
        self.labelZeith.setText(_translate("EphemToolWidget", "Zenith:"))
        self.labelSunRise.setText(_translate("EphemToolWidget", "Rise:"))
        self.labelSunSet.setText(_translate("EphemToolWidget", "Set:"))
        self.tabWidgetOutput.setTabText(self.tabWidgetOutput.indexOf(self.tabSun), _translate("EphemToolWidget", "Sun"))
        self.labelMoonRise.setText(_translate("EphemToolWidget", "Rise:"))
        self.labelMoonPhase.setText(_translate("EphemToolWidget", "Phase:"))
        self.labelMoonSet.setText(_translate("EphemToolWidget", "Set:"))
        self.labelMoonAzimuthElevation.setText(_translate("EphemToolWidget", "Azimuth / Elevation:"))
        self.tabWidgetOutput.setTabText(self.tabWidgetOutput.indexOf(self.tabMoon), _translate("EphemToolWidget", "Moon"))
        self.label_3.setText(_translate("EphemToolWidget", "<html><head/><body><p>Note: In locations with steep relief and/or large height differences over a short distance, several rises and sets are possible, but only one result will be displayed.</p></body></html>"))
import resources_rc
