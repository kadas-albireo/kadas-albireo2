import os
from copy import deepcopy

from qgis.PyQt import uic

from kadas_routing.utilities import localeName

WIDGET, BASE = uic.loadUiType(
    os.path.join(os.path.dirname(__file__), "patrolwarning.ui")
)


class PatrolWarning(BASE, WIDGET):
    def __init__(self, parent=None):
        super(PatrolWarning, self).__init__(parent)
        self.setupUi(self)
        self.btnClose.clicked.connect(self.close)

        content_en = {
            "title": "Warning",
            "description": "The calculation for the patrol route failed from point {start} to point {end}. Please try one of the following options:",  # noqa: E501
            # TODO: Use a list
            "first_item": "Redraw a smaller patrol area",
            "second_item": "Avoid drawing a patrol area that intersects major roads",
            "third_item": "If the problem persists even for a smaller well-defined area then the algorithm is unable to find a connection for at least one of the enclosed nodes and the patrol can not be calculated for this area.",  # noqa: E501
        }
        content_de = {
            "title": "Warnung",
            "description": "Die Berechnung der Patrouillen-Route von Punkt {start} nach Punkt {end} ist fehlgeschlagen. Bitte versuchen Sie eine der folgenden Optionen:",  # noqa: E501
            # TODO: Use a list
            "first_item": "Zeichnen Sie ein kleineres zu befahrendes Gebiet",
            "second_item": "Zeichnen Sie das Gebiet so, dass es keine Hauptverkehrsstraßen kreuzt",
            "third_item": "Wenn das Problem auch bei einem kleineren klar definierten Gebiet bestehen bleibt, ist der Algorithmus in dieser Version nicht in der Lage eine Verbindung für mindestens einen der eingeschlossenen Knotenpunkte zu finden und die Patrouille kann für dieses Gebiet nicht berechnet werden.",  # noqa: E501
        }
        content_fr = {
            "title": "Atencion",
            "description": "Le calcul de l'itinéraire de patrouille a échoué du point {start} au point {end}. Veuillez essayer l'une des options suivantes :",  # noqa: E501
            # TODO: Use a list
            "first_item": "Redessinez une zone de patrouille plus petite.",
            "second_item": "Évitez de dessiner une zone de patrouille qui croise des routes principales.",
            "third_item": "Si le problème persiste même pour une zone plus petite et bien définie, cela signifie que l'algorithme est incapable de trouver une connexion pour au moins un des nœuds inclus et que la patrouille ne peut pas être calculée pour cette zone.",  # noqa: E501
        }
        content_it = {
            "title": "Attenzione",
            "description": "Il calcolo del percorso di ronda non è riuscito dal punto {start} al punto {end}. Prova una delle seguenti opzioni:",  # noqa: E501
            # TODO: Use a list
            "first_item": "Ridisegnare un'area di pattuglia più piccola",
            "second_item": "Evitare di disegnare un'area di pattuglia che intersechi le strade principali",
            "third_item": "Se il problema persiste anche per un'area più piccola e ben definita, allora l'algoritmo non è in grado di trovare una connessione per almeno uno dei nodi racchiusi e la ronda non può essere calcolata per quest'area.",  # noqa: E501
        }

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
