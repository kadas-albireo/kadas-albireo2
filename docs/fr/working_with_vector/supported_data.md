<!-- Recovered from: share/docs/html/fr/fr/working_with_vector/supported_data/index.html -->
<!-- Language: fr | Section: working_with_vector/supported_data -->

# Formats de données gérés

KADAS utilise la bibliothèque OGR pour lire et écrire des données vectorielles incluant les formats ESRI shapefiles, MapInfo et MicroStation; les bases de données AutoCAD DXF, PostGIS, SpatiaLite, Oracle Spatial et MS SQL Spatial et de nombreux autres formats. Les données vectorielles GRASS et PostgreSQL sont gérées par des extensions natives de KADAS. Les données vectorielles peuvent également être lues depuis des archives zip ou gzip. A ce jour, 69 formats de données vectorielles sont gérés par la bibliothèque OGR. La liste complète est disponible sur <http://www.gdal.org/ogr/ogr_formats.html>.

## Shapefiles ESRI

Le format de fichier vecteur standard utilisé par QGIS est le shapefile ESRI. Il est géré à travers la bibliothèque OGR Simple Feature Library (<http://www.gdal.org/ogr/>).

Un shapefile est en réalité composé de plusieurs fichiers. Les trois suivants sont requis:

1. `.shp` fichier contenant la géométrie des entités.
2. `.dbf` fichier contenant les attributs au format dBase.
3. `.shx` fichier d'index.

Un shapefile inclus également un fichier ayant l'extension `.prj` qui contient les informations sur le système de coordonnées. Bien que ces informations soient très utiles elles ne sont pas obligatoires. Il peut y avoir encore d'autres fichiers associés aux données shapefile. Si vous souhaitez avoir plus de détails, nous vous recommandons de vous reporter aux spécifications techniques du format shapefile, qui se trouve notamment sur <http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf>.

### Charger un Shapefile

When loading a vector layer, the following dialog opens:

![](../../../../images/addvectorlayerdialog.png)

Cliquez sur ![radiobuttonon](../../../../images/radiobuttonon.png) _Fichier_ puis sur le bouton **[Parcourir]**. L'outil ouvre alors une fenêtre de dialogue standard qui vous permet de naviguer dans les répertoires et les fichiers, et charger le shapefile ou tout autre format géré. La boîte de sélection _Fichiers de type_ ![](../../../../images/selectstring.png) vous permet de présélectionner un format de fichier géré par OGR.

Si vous le souhaitez, vous pouvez également sélectionner le type de codage du shapefile.

![](../../../../images/shapefileopendialog.png)

Selecting a shapefile from the list and clicking **[Open]** loads it into KADAS.

**Couleur des couches**

Quand vous ajoutez une couche sur une carte, une couleur aléatoire lui est assignée. En ajoutant plusieurs couches en une fois, différentes couleurs sont assignées à chacune des couches.

Une fois le shapefile chargé, vous pouvez zoomer dessus en utilisant les outils de navigation sur la carte. Pour changer la symbologie d'une couche, ouvrez la fenêtre _Propriétés de la Couche_ en double-cliquant sur le nom de la couche ou en faisant un clic droit sur son nom dans la légende et en choisissant _Propriétés_ dans le menu qui apparait. Pour plus de détails sur les paramètres de la symbologie des couches vectorielles.

### Améliorer les performances d'affichage des Shapefiles

Pour améliorer les performances de dessin d'un shapefile, vous pouvez créer un index spatial. Un index spatial améliorera à la fois la vitesse d'exécution du zoom et du déplacement panoramique. Les index spatiaux utilisés par QGIS ont une extension `.qix`.

Voici les étapes de création d'un index spatial:

- Chargez un shapefile en cliquant sur le bouton ![](../../../../images/mActionAddOgrLayer.png) _Ajouter une couche vecteur_ de la barre d'outils ou en pressant les touches `Ctrl+Shift+V`.
- Ouvrez la fenêtre _Propriétés de la Couche_ en double-cliquant sur le nom de la couche dans la légende ou en faisant un clic droit et en choisissant _Propriétés_ dans le menu qui apparait.
- Dans l'onglet _Général_, cliquez sur le bouton **[Créer un index spatial]**.

### Problème de chargement de fichier .prj

Si vous ouvrez un shapefile disposant d'un fichier `.prj` et que KADAS ne parvient pas à lire le système de coordonnées de référence, vous allez devoir le définir manuellement via l'onglet _Général_ de la fenêtre de _Propriétés de la Couche_ en cliquant sur le bouton **[Specifier...]**. Cela est dû au fait que ce fichier `.prj` ne fournit pas les paramètres complets de la projection requis par KADAS et listés dans la fenêtre _SCR_.

C'est pour la même raison que lorsque vous créez un nouveau shapefile avec KADAS, deux fichiers de projection différents sont créés. Un fichier `.prj` contenant un nombre limité de paramètres, compatible avec les logiciels ESRI et un fichier `.qpj`, fournissant la totalité des paramètres du SCR utilisé. Chaque fois que KADAS trouve un fichier `.qpj`, il l'utilisera à la place du fichier `.prj`.
