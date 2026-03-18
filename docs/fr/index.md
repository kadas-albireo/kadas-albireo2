<!-- Recovered from: docs_old/html/fr/fr/index.html -->
<!-- Language: fr | Section: index -->

# Généralités

## Création

KADAS Albireo est une application d'affichage des cartes basée sur le logiciel SIG professionnel open source "QGIS" et destinée aux utilisateurs non spécialisés. En coopération avec la compagnie Ergnomen, une nouvelle interface utilisateur a été développée, cachant une grande partie des fonctionnalités destinées aux utilisateurs avancés, tout en améliorant les fonctionnalités dans des domaines tels que le dessin, l'analyse de terrain, l'impression et l'interopérabilité.

## Conditions d’utilisation

KADAS Albireo est sous General Public License 2.0 (GPLv2).

La composante MSS/MilX est la propriété de l’entreprise gs-soft AG.

Les conditions d’utilisation des données sont énumérées dans l’application sous Aide → À propos.

## Protocole de version

### Version 2.3 (août 2025)

La version KADAS 2.3, sortie au troisième trimestre 2025, apporte de nombreuses nouveautés, améliorations et corrections.

- _Nouvelles fonctionnalités et géodonnées_
    - Les géoservices et les géodonnées comportant une composante temporelle peuvent être lus sous forme d'animation chronologique.
    - La comparaison de thèmes (outil SWIPE) permettant d'afficher et de masquer de manière interactive deux vues cartographiques à l'aide d'un curseur.
    - Les tableaux d'attributs ouverts sont enregistrés dans le projet et s'affichent à l'ouverture de celui-ci.
    - La prise en charge du format de données raster NetCDF (utilisé principalement en météorologie).
    - Un nouveau bouton « Feedback ». Ce bouton mène directement à un formulaire sur le Geo Info Portal V et permet de donner son avis sur les fonctionnalités du logiciel et de signaler des bugs.
    - La création de profils d'altitude fournit des statistiques supplémentaires, telles que le point le plus haut/le plus bas, la longueur et la différence d'altitude par rapport à la ligne du profil.
    - Les géoservices Vector Tiles de la MGDI sont répertoriés dans le géocatalogue et peuvent être chargés.
    - Les données hors ligne proviennent désormais de la World Briefing Map.
    - L'exportation MSS est désormais possible au format KML (non modifiable après l'exportation KML).
- _Améliorations_
    - Gestionnaire de plugins : au démarrage de KADAS, le système vérifie si des versions plus récentes des plugins installés sont disponibles dans le référentiel. Si tel est le cas, la dernière version du plugin est automatiquement installée et l'utilisateur en est informé par un message.
    - Lors d'une requête par clic, l'affichage des résultats a été amélioré, en particulier lorsque plusieurs résultats sont renvoyés.
    - La fonction Signets enregistre désormais également les couches dans des groupes et des sous-groupes.
    - Le plugin Routing est désormais un plugin standard et est installé automatiquement à partir du référentiel. D'autres plugins peuvent également être définis comme obligatoires.
    - Les géoservices restreints dans la MGDI peuvent être affichés dans KADAS par les utilisateurs autorisés.
    - La recherche de coordonnées a été améliorée et standardisée.
    - La prise en charge des géoservices WCS permet désormais également d'effectuer des analyses avec des modèles altimétriques à haute résolution.
    - La fonction Texte dans Redlining contient des options de configuration supplémentaires.
    - L'affichage et le libellé de la grille MGRS ont été optimisés.
    - Les modèles d'impression ont été mis à jour et contiennent désormais par défaut la date de création et le système de projection.
    - Les analyses peuvent être lancées directement par un clic droit de la souris.
    - L'extension de l'exportation GPKG des données vectorielles peut être définie.
- _Corrections_
    - Le crash lors de l'exportation de GeoPDF a été corrigé.
    - Lors de l'exportation de GeoPDF, toutes les couches sont prises en compte et affichées correctement.
    - Le crash lors de l'utilisation de l'outil Éphémérides a été corrigé.
    - Le crash lors de l'exportation de GeoPackage a été corrigé.
    - Corrigé : traductions manquantes dans les langues (DE, FR, IT)
- _Adaptations techniques_
    - KADAS 2.3 est basé sur la version QGIS 3.44.0-Solothurn.
    - Les deux modèles altimétriques Suisse et mondial (dtm\_analysis.tif + general\_dtm\_globe.tif) sont disponibles au format COG (Cloud Optimized GeoTIFF). Cela permet un traitement et une visualisation plus rapides.
    - Le visualiseur 3D (Globe) a dû être remplacé, car la technologie utilisée est en fin de vie. Le visualiseur 3D est désormais utilisé par l'application QGIS.
    - La bibliothèque MSS/MilX utilisée par gs-soft est désormais MSS-2025.
    - Le « Valhalla Routing Engine » a été mis à jour et permet désormais de prendre en compte l'altitude dans le calcul des itinéraires et des distances.
    - Tous les plugins productifs ont été adaptés à la version 2.3.
    - L'aide a été externalisée dans le navigateur.

### Version 2.2.0 (juin 2023)

- _Général_:
    - Possibilité de ajouter des couches Esri VectorTile
    - Possibilité de ajouter des couches Esri MapService
    - Layertree: possibilité de définir l'intervalle de rafraîchissement de la source de données pour les couches à rafraîchissement automatique.
    - Permettre l'exportation de l'impression à GeoPDF
    - Permet de verrouiller l'échelle de la carte
    - Boîte de dialogue configurable pour les nouvelles
    - Amélioration de l'importation de géométries 3D à partir de fichiers KML
- _Vue_:
    - Permet de prendre des photos instantanées de la vue 3D
    - Amélioration de l'étiquetage de la grille MGRS
- _Analyse_:
    - Nouvel outil min/max pour interroger le point le plus bas/le plus haut dans la zone sélectionnée
    - Sélection du fuseau horaire dans l'outil éphéméride
    - Gestion correcte des valeurs NODATA dans le profil de hauteur
- _Dessin_:
    - Permettre d'annuler/de refaire toute la session de dessin
    - Permet de modifier l'ordre z des dessins
    - Permet d'ajouter des images à partir de l'URL
- _MSS_:
    - Permet d'annuler/de refaire toute la session de dessin
    - Permet de styliser les lignes de repère (largeur, couleur)
    - Mise à jour de MSS-2024
- _Aide_:
    - Permettre la recherche dans l'aide

### Version 2.1.0 (décembre 2021)

- _Général_:
    - Imprimer: Mise à l'échelle correcte des symboles (MSS, épingles, images, ...) en fonction du DPI d'impression.
    - GPKG: Permet l'importation de couches de projet
    - Arbre des couches: Possibilité de zoomer et de supprimer toutes les couches sélectionnées
    - Visibilité basée sur l'échelle également pour les couches redlining/MSS
    - Table d'attributs: divers nouveaux outils de sélection et de zoom pour les tables d'attributs
- _Vue_:
    - Nouvelle fonction de signets
- _Analyse_:
    - Bassin visuel: Possibilité de limiter la plage d'angle vertical de l'observateur
    - Profil d’altitude et visibilité: affichage du marqueur dans le graphe lors du survol de la ligne sur la carte
    - Nouvel outil éphéméride
- _Dessin_:
    - Épingles: Ajout d'un éditeur de texte riche
    - Epingles: Permettre d'interagir avec le contenu de l'infobulle avec la souris
    - Grille de guidage: Permettre l'étiquetage d'un seul quadrant
    - Bullseye: Étiquetage des quadrants
    - Nouvel élément de dessin en croix de coordonnées
- _MSS_:
    - Paramètres des symboles par couche
    - Mise à jour de MSS-2022

### Version 2.0.0 (juliet 2020)

- Nouvelle architecture d'application: KADAS est maintenant une application distincte, construite sur les bibliothèques QGIS 3.x
- Nouvelle architecture des éléments de la carte, pour un flux de travail cohérent lors du dessin et de l'édition des objets Redlining, des symboles MSS, etc.
- Utilise le nouveau format de fichier qgz, en évitant l'ancien dossier `<nom du projet>_files`
- Autosave du projet
- Nouveau gestionnaire de plugins pour gérer les plugins externes directement depuis KADAS
- Mode plein écran
- Nouvel outil de grille, supportant aussi les grilles UTM/MGRS sur la carte principale
- Exportation KML/KMZ limitée à une étendue
- Exportation de données GPKG limitée à une étendue
- Les styles de géométrie redlining sont honorés lorsqu'ils sont affichés sous forme d'objets 2,5D ou 3D sur le Globe
- Grille de guidage améliorée
- Mise à jour du MSS à MSS-2021

### Version 1.2 (décembre 2018)

- _Général_:
    - Fonctionnalité d'exportation KML/KMZ améliorée
    - Nouvelle fonctionnalité d'importation KML/KMZ
    - Nouvelle fonction d'exportation et d'importation GeoPackage
    - Possibilité d'ajouter des niveaux CSV/WMS/WFS/WFS/WCS depuis l'interface ribbon
    - Possibilité d'ajouter des fonctionnalités à l'interface ribbon à partir de l'API Python
    - Ajout de raccourcis clavier pour diverses fonctions d'interface ribbon
    - Améliore le "fuzzy matching" dans la recherche de coordonnées
- _Analyse_:
    - Tracer les sommets de la ligne de mesure dans le profil d'élévation
- _Dessigner_:
    - Prise en charge de la saisie numérique dans le dessin d'objets redlining
    - Possibilité du réglage du facteur d'échelle pour les couches d'annotation
    - Possibilité d'activer et de désactiver les cadres d'images
    - Possibilité de manipuler des groupes d'annotations
    - Nouvelle fonctionnalité: grille de guidage
    - Nouvelle fonctionnalité: Bullseye
- _GPS_:
    - Possibilité de conversion entre les waypoints et les pins
    - Possibilité de changer la couleur des waypoints et des itinéraires
- _MSS_:
    - Mise à niveau vers MSS-2019

### Version 1.1 (Novembre 2017)

- _Général_:
    - Curseur librement positionnable dans le champ de recherche
    - Affichage de la hauteur dans la barre d'état
    - Amélioration de la vitesse dans l'affichage de la carte
    - Table d'attributs pour les couches vectorielles
    - Chargement des graphiques SVG (y compris les graphiques SymTaZ)
- _Analyse_:
    - Mesure géodésique de la distance et de la superficie
    - Azimut sélectionnable par rapport au nord de la carte ou géographique
- _Dessigner_:
    - Accrochage aux objets commutable lors du dessin
    - Annuler / refaire pendant le dessin
    - Les géométries peuvent être déplacées, copiées, coupées et collées, individuellement ou en groupe
    - Les géométries existantes peuvent être continuées
    - Graphiques SVG (comprenant symboles SymTaZ) peuvent être ajouté à la carte
    - Images pas géoréférencées peuvent être ajouté à la carte
    - Images et punaises sont stockées dans des couches appropriées
- _MSS_:
    - Mise à niveau vers MSS-2018
    - Corriger le format des symboles MSS lors de l'impression
    - Le contenu du cartouche peut être importé ou exporté depuis et vers des fichiers MilX ou XML
    - Entrée numérique d'attributs lors du dessin de symboles MSS
- _3D_:
    - Support pour les géométries 3D en vue 3D
- _Imprimer_:
    - Possibilité de gérer les modèles d'impression contenus dans un projet

### Version 1.0 (Septembre 2016)

- Version initiale
