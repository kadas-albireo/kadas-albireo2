# -*- coding: utf-8 -*-

import os

from qgis.PyQt.QtCore import QLocale, QTranslator, QCoreApplication, QSettings
from qgis.PyQt.QtWidgets import QMessageBox

from kadasrouting.utilities import localeName

# Setup internationalisation for the plugin.
#
# See if QGIS wants to override the system locale
# and then see if we can get a valid translation file
# for whatever locale is effectively being used.
# Adapted from: https://github.com/inasafe/inasafe/blob/develop/__init__.py


os.environ['LANG'] = str(localeName())

root = os.path.abspath(os.path.join(os.path.dirname(__file__)))
translation_path = os.path.join(
    root, 'i18n',
    'kadasrouting_' + str(localeName()) + '.qm')

if os.path.exists(translation_path):
    translator = QTranslator()
    result = translator.load(translation_path)
    if not result:
        message = 'Failed to load translation for %s' % localeName()
        raise Exception(message)
    # noinspection PyTypeChecker,PyCallByClass
    QCoreApplication.installTranslator(translator)
elif not os.path.exists(translation_path) and localeName().lower() in ['it', 'de', 'fr']:
    # Show warning if the IT, FR, or DE translation file is not found and the current system on it
    QMessageBox.warning(
        None,
        'Translation file missing',
        'Translation file is not found for %s in %s' % (localeName().upper(), translation_path))

def classFactory(iface):
    from .plugin import RoutingPlugin

    return RoutingPlugin(iface)
