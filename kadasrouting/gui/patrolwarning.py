import os
from copy import deepcopy

from qgis.PyQt import uic

from kadasrouting.utilities import localeName

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "patrolwarning.ui")
)


class PatrolWarning(BASE, WIDGET):
    def __init__(self, parent=None):
        super(PatrolWarning, self).__init__(parent)
        self.setupUi(self)
        indexes = ["en", "de", "fr", "it"]
        try:
            index = indexes.index(localeName())
        except ValueError:
            index = 0
        self.btnClose.clicked.connect(self.close)

        content_en = {
            "title": "Warning",
            "description": "The calculation for the patrol route failed from point {start} to point {end}. Please try one of the following options:",
            # TODO: Use a list
            "first_item": "Redraw a smaller patrol area",
            "second_item": "Avoid drawing a patrol area that intersects major roads",
            "third_item": "If the problem persists even for a smaller well-defined area then the algorithm is unable to find a connection for at least one of the enclosed nodes and the patrol can not be calculated for this area.",
        }

        content_de = {}
        content_fr = {}
        content_it = {}

        self.contents = {
            "en": content_en,
            "de": content_de,
            "fr": content_fr,
            "it": content_it
        }

        self.html_template = """
            <h1 align="center">{title}</h1>
            <p>{description}</p>
            <ol style="list-style-type: upper-alpha;">
                <li>{first_item}</li>
                <li>{second_item}</li>
                <li>{third_item}</li>
            </ol>
        """

    def setWarningMessage(self, start, end):
        content = deepcopy(self.contents.get(localeName(), self.contents["en"]))
        content["description"] = content["description"].format(start=start, end=end)
        html_message = self.html_template.format(**content)
        self.textBrowser.setHtml(html_message)
