# -*- coding: utf-8 -*-

import os

from qgis.PyQt.QtCore import QLocale, QTranslator, QCoreApplication, QSettings
from qgis.PyQt.QtWidgets import QMessageBox

# Setup internationalisation for the plugin.
#
# See if QGIS wants to override the system locale
# and then see if we can get a valid translation file
# for whatever locale is effectively being used.
# Adapted from: https://github.com/inasafe/inasafe/blob/develop/__init__.py

override_flag = QSettings().value(
    'locale/overrideFlag', True, type=bool)

if override_flag:
    locale_name = QSettings().value('locale/userLocale', 'en_US', type=str)
else:
    locale_name = QLocale.system().name()
    # NOTES: we split the locale name because we need the first two
    # character i.e. 'id', 'af, etc
    locale_name = str(locale_name).split('_')[0]

os.environ['LANG'] = str(locale_name)

root = os.path.abspath(os.path.join(os.path.dirname(__file__)))
translation_path = os.path.join(
    root, 'i18n',
    'kadasrouting_' + str(locale_name) + '.qm')

if os.path.exists(translation_path):
    translator = QTranslator()
    result = translator.load(translation_path)
    if not result:
        message = 'Failed to load translation for %s' % locale_name
        raise Exception(message)
    # noinspection PyTypeChecker,PyCallByClass
    QCoreApplication.installTranslator(translator)
else:
    QMessageBox.information(
        None,
        'Translation path',
        'Translation path not found: %s' % translation_path)

def classFactory(iface):
    from .plugin import RoutingPlugin

    return RoutingPlugin(iface)
