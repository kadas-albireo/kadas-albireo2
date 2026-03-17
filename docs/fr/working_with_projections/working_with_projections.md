<!-- Recovered from: share/docs/html/fr/fr/working_with_projections/working_with_projections/index.html -->
<!-- Language: fr | Section: working_with_projections/working_with_projections -->

# Utiliser les projections

KADAS permet à l'utilisateur de définir un système de coordonnées de référence (SCR) par défaut et pour l'ensemble des projets, pour les couches démunies de SCR prédéfini. Il lui permet également de définir des systèmes de coordonnées de référence personnalisés et autorise la projection à la volée de couches vecteur et raster. Toutes ces fonctionnalités permettent à l'utilisateur d'afficher des couches avec différents SCR et de les superposer correctement.

## Aperçu de la gestion des projections

KADAS gère approximativement 2 700 SCR connus. Leur définition est stockée dans une base de données SQLite qui est installée avec KADAS. Normalement vous n'avez pas besoin de manipuler cette base de données directement. En fait, cela peut poser des problèmes de gestion de projections. Les SCR personnalisés y sont stockés dans une base de données utilisateur. Reportez-vous à la section _sec\_custom\_projections_ pour avoir des informations sur la gestion de vos systèmes de coordonnées de référence personnalisées.

Les SCR disponibles dans KADAS sont basés sur ceux définis par l'EPSG (European Petroleum Search Group) et l'Institut National Géographique (IGNF) et sont en grande partie extraits des tables spatiales de référence de GDAL. Les identifiants EPSG sont présents dans la base de données et peuvent être utilisés pour définir un SCR dans KADAS.

Pour utiliser la projection à la volée, soit vos données contiennent des informations sur leur système de coordonnées de référence soit vous avez défini un SCR global, par projet, ou bien par couche. Pour les couches PostGIS, KADAS utilise l'identifiant de référence spatiale qui a été défini quand la couche a été créée. Pour les données gérées par OGR, KADAS utilise un moyen spécifique au format pour définir le SCR. Dans le cas du shapefile, il s'agit d'un fichier contenant une spécification well-known text (WKT) (WKT) de la projection. Le fichier de projection a le même nom que le fichier shape et une extension `.prj`. Par exemple, un shapefile nommé `alaska.shp` aura un fichier de projection correspondant nommé `alaska.prj`.

L'onglet _SCR_ contient les éléments importants suivants:

1. **Rechercher** — Si vous connaissez le code EPSG, l'identifiant ou le nom d'un système de coordonnées de référence, vous pouvez utiliser la fonction rechercher pour le retrouver. Entrez le code EPSG, l'identifiant ou le nom à chercher.
2. **système de coordonnées de référence récemment uilisés** — Si vous utilisez certains SCR fréquemment dans vos travaux quotidiens, ils seront affichés dans cette liste. Cliquez sur l'un d'entre eux pour sélectionner le SCR du projet.
3. **Liste des SCR mondiaux** — C'est une liste de tous les SCR gérés par KADAS, incluant les systèmes de coordonnées de référence géographiques, projetés et personnalisés. Pour utiliser un SCR, sélectionnez-le dans la liste en dépliant le nœud approprié et en choisissant le système de coordonnées. Le SCR actif est présélectionné.
4. **Texte PROJ.4** — C'est la liste des paramètres décrivant le SCR telle qu'elle est utilisée par le moteur de projection Proj4. Ce texte est en lecture seule et est fourni à titre informatif.

## Transformations géodésiques par défaut

La projection à la volée dépend de la capacité à transformer les données dans un _SCR par défaut_ et KADAS utilise ici le WGS84. Pour certains SCR, plusieurs méthodes de transformation sont disponibles. KADAS vous permet de choisir laquelle utiliser, sinon une transformation par défaut sera utilisée.

KADAS demande quelle transformation utiliser en ouvrant une fenêtre qui affiche au format texte PROJ.4 les transformations de la source et de la cible. De plus amples informations s'affichent au passage de la souris sur une transformation. Les transformations à utiliser par défaut sont sauvegardées en cochant ![radiobuttonon](/images/radiobuttonon.png) _Se souvenir de la sélection_.
