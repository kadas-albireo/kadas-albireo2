<!-- Recovered from: share/docs/html/fr/fr/working_with_raster/raster_properties/index.html -->
<!-- Language: fr | Section: working_with_raster/raster_properties -->

# Fenêtre Propriétés de la couche raster

Pour voir et définir les propriétés d'une couche raster, double-cliquez sur le nom de la couche dans la légende de la carte ou faites un clic-droit son nom et choisissez _Propriétés_ dans le menu qui apparaît. La fenêtre des _Propriétés de la couche_ apparaîtra.

Il y a plusieurs onglets dans cette fenêtre:

- _Général_
- _Style_
- _Transparence_
- _Pyramides_
- _Histogramme_
- _Métadonnées_

![](../../../../images/rasterPropertiesDialog.png)

## Onglet Général

### Informations sur la couche

L'onglet _Général_ affiche des informations basiques sur le raster sélectionné, dont la source de la couche, le nom affiché dans la légende (qui peut être modifié), le nombre de colonnes, lignes et les valeurs _no-data_.

### Système de coordonnées de référence

Le système de coordonnées de référence (SCR) est également affiché ici au format PROJ.4. S'il est incorrect, il peut être modifié en cliquant sur le bouton **[Spécifier]**.

### Visibilité dépendante de l'échelle

La visibilité en fonction de l'échelle se définit également dans cet onglet. Vous devez activer la case à cocher et définir une échelle appropriée pour l'affichage de vos données sur la carte.

Tout en bas, sont affichés un aperçu de la couche, son symbole de légende et sa palette.

## Onglet Style

### Rendu des bandes raster

KADAS propose quatre _Types de rendu_. Le choix s'effectue en fonction du type de données.

1. Couleur à Bandes Multiples - Si le fichier raster est multibande et contient plusieurs bandes (par exemple, avec une image satellite)
2. Palette - Si le fichier ne contient qu'une seule bande indexée (par exemple, pour les cartes topographiques)
3. Bande Grise Unique - (Une seule bande de gris). Le rendu de l'image sera gris. KADAS choisit ce rendu si ce fichier n'est ni multibande, ni une palette indexée, ni une palette continue (utilisée par exemple pour les cartes avec des reliefs ombrés)
4. Pseudo-Couleur à Banque Unique - vous pouvez utiliser ce rendu pour les fichiers contenant une palette continue ou des cartes en couleur (par exemple pour une carte des altitudes)

**Couleur à bandes multiples**

Avec ce type de rendu, trois bandes de l'image seront utilisées, chacune correspondant à la composante rouge, verte ou bleue de l'image colorée finale. Vous pouvez choisir parmi différentes méthodes d’_Amélioration du contraste_: _Pas d'amélioration_, _Étirer jusqu'au MinMax_, _Étirer et couper jusqu'au MinMax_ ou _Couper jusqu'au MinMax_.

![](../../../../images/rasterMultibandColor.png)

Ces options vous offrent de nombreuses possibilités de modifier l'apparence de votre couche raster. Premièrement vous devez connaître la plage de valeurs de votre image. Vous pouvez utiliser pour cela l’_Emprise_ et cliquer sur **[Charger]**. Pour les valeurs de _Min_ et de _Max_ de vos bandes, KADAS vous laisse le choix entre une _Précision_ ![radiobuttonon](../../../../images/radiobuttonon.png) _Estimée (plus rapide)_ ou ![radiobuttonon](../../../../images/radiobuttonon.png) _Réelle (plus lente)_.

Maintenant vous pouvez échelonner les couleurs grâce à la partie _Charger les valeurs min/max_. Beaucoup d'images n'ont que très peu de valeurs très faibles ou très élevées. Ces extrêmes peuvent être ignorés en utilisant l'option ![radiobuttonon](../../../../images/radiobuttonon.png) _Bornes d'exclusion des valeurs extrêmes_. Par défaut la plage proposée va de 2% à 98% des valeurs de données et peut être ajustée manuellement. Avec ce paramétrage, l'aspect gris de l'image peut disparaitre. Avec l'option ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min / max_, KADAS crée une table de couleur à partir de toutes les données de l'image originale (par exemple, KADAS crée une table de couleur avec 256 valeurs, si vous avez des bandes codées sur 8 bits). Vous pouvez également calculer votre table de couleur en utilisant l'option _Moyenne +/- écart-type x_ ![](../../../../images/selectnumber.png). Ainsi, seules les valeurs comprises dans cet intervalle (écart-type ou multiple de l'écart-type) seront considérées. Ceci est utile lorsqu'un ou deux pixels ont des valeurs anormalement élevées et ont un impact négatif sur le rendu du raster.

Tous les calculs peuvent également être réalisés pour l'emprise ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Actuelle_.

**Visualiser une seule bande d'un raster multibande**

Si vous désirez visualiser une seule bande d'une image multibande (par exemple la bande rouge), vous pouvez penser que vous pourriez définir les bandes Verte et Bleue à “Non définie”. Mais ce n'est pas la manière correcte. Pour afficher la bande Rouge, définissez le type d'image à Bande grise unique, puis sélectionnez la bande Rouge comme bande à utiliser pour le gris.

**Palette**

C'est l'option standard pour les fichiers à une seule bande qui incluent déjà une table de couleurs, où à chaque valeur de pixel a été assignée une couleur. Dans ce cas, la palette est utilisée automatiquement. Si vous désirez modifier l'assignement des couleurs pour certaines valeurs, double cliquez simplement sur la couleur et la boîte de dialogue de _Sélection de couleur_ apparaîtra. Il est maintenant possible depuis KADAS 2.2 d'assigner un label aux valeurs de couleur. L'étiquette apparaîtra alors dans la légende de la couche raster.

![](../../../../images/rasterPaletted.png)

**Amélioration de contraste**

Lors de l'ajout d'une couche raster GRASS, l'option _Amélioration de contraste_ sera automatiquement _Étirer jusqu'au MinMax_, quelles que soient les options générales de KADAS définies pour cette option.

**Bande grise unique**

Ce type de rendu vous permet de représenter une bande d'un raster par un _Dégradé de couleur_: _Noir vers blanc_ ou _Blanc vers noir_. Vous pouvez choisir un _Min_ et un _Max_ en choisissant d'abord une _Emprise_ puis en cliquant sur **[Charger]**. KADAS peut utiliser les valeurs _Min_ et _Max_ ![radiobuttonon](../../../../images/radiobuttonon.png) _Estimée (plus rapide)_ ou utiliser les valeurs ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Réelle (plus lent)_.

![](../../../../images/rasterSingleBandGray.png)

Grâce à la partie _Charger les valeurs min/max_, vous pouvez échelonner les couleurs. Les valeurs extrêmes peuvent être ignorées en utilisant l'option ![radiobuttonon](../../../../images/radiobuttonon.png) _Bornes d'exclusion des valeurs extrêmes_. Par défaut la plage proposée va de 2% à 98% des valeurs de données et peut être ajustée manuellement. Avec ce paramétrage, l'aspect gris de l'image peut disparaitre. D'autres réglages peuvent être effectués via les boutons ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min / max_ and ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Moyenne +/- écart-type x_ ![](../../../../images/selectnumber.png). Le premier crée une table de couleur à partir de toutes les données de l'image originale alors que le deuxième crée une table de couleur qui ne considère que les valeurs comprises dans l'intervalle constitué par l'écart-type ou un multiple de l'écart-type. Ceci est utile lorsqu'un ou deux pixels ont des valeurs anormalement élevées et ont un impact négatif sur le rendu du raster.

**Pseudo-couleur à bande unique**

C'est une option de rendu pour les fichiers à bande unique, incluant une palette de couleurs continues. Vous pouvez aussi créer des palettes de couleurs pour les fichiers à bande unique

![](../../../../images/rasterSingleBandPseudocolor.png)

Trois manières de faire une interpolation de couleurs sont disponibles:

1. Discrète
2. Linéaire
3. Exacte

Sur la partie gauche, le bouton ![](../../../../images/mActionSignPlus.png) _Ajouter une valeur manuellement_, permet d'ajouter une valeur individuelle à la table de couleurs. Le bouton ![](../../../../images/mActionSignMinus.png) _Supprimer la valeur sélectionnée_ efface une valeur et ![](../../../../images/mActionArrowDown.png) _Trier les éléments de la palette de couleurs_ permet de trier la table de couleurs en fonction des valeurs de pixels. En double-cliquant sur une valeur, vous pouvez l'éditer manuellement. Un double-clic sur une couleur ouvre la fenêtre _Modifier la couleur_ où vous pouvez la modifier. De plus, vous pouvez ajouter une étiquette de légende pour chaque valeur (mais cette information n'apparaîtra pas lors de l'utilisation de l'outil d'identification). Vous pouvez également ![](../../../../images/mActionDraw.png) _Charger une palette de couleur depuis la bande_, si une table de couleurs a été définie pour la bande. Enfin vous pouvez utiliser les boutons ![](../../../../images/mActionFileOpen.png) _Charger une palette de couleur depuis un fichier_ ou ![](../../../../images/mActionFileSaveAs.png) _Exporter une palette de couleur vers un fichier_ pour importer ou exporter une table de couleur depuis ou vers une autre session.

Sur la partie droite, il est possible de _Générer une nouvelle palette de couleur_. Pour le _Mode_ de classification par ![](../../../../images/selectstring.png) _Intervalles égaux_, vous n'avez qu'à choisir le nombre de _Classes_ ![](../../../../images/selectnumber.png) et cliquez sur le bouton _Classer_. Vous pouvez inverser l'ordre des couleurs de la palette en cochant la case ![](../../../../images/checkbox.png) _Inverser_. Dans le cas d'une classification par _Mode_ ![](../../../../images/selectstring.png) _Continu_, KADAS crée automatiquement les classes en fonction des _Min_ et _Max_. La partie située en dessous, _Charger les valeurs min/max_, permet d'ajuster ces valeurs. En effet, beaucoup d'images n'ont que très peu de valeurs très faibles ou très élevées. Ces extrêmes peuvent être ignorés en utilisant l'option ![radiobuttonon](../../../../images/radiobuttonon.png) _Bornes d'exclusion des valeurs extrêmes_. Par défaut la plage proposée va de 2% à 98% des valeurs de données et peut être ajustée manuellement. Avec ce paramétrage, l'aspect gris de l'image peut disparaitre. Avec l'option ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min / max_, KADAS crée une table de couleur à partir de toutes les données de l'image originale (par exemple, KADAS crée une table de couleur avec 256 valeurs, si vous avez des bandes codées sur 8 bits). Vous pouvez également calculer votre table de couleur en utilisant l'option _Moyenne +/- écart-type x_ ![](../../../../images/selectnumber.png). Ainsi, seules les valeurs comprises dans cet intervalle (écart-type ou multiple de l'écart-type) seront considérées.

### Rendu des couleurs

Pour chaque type de _Rendu par bande_, des options de _Rendu de la couleur_ sont disponibles.

Vous pouvez réaliser des effets spéciaux sur le rendu de vos rasters en utilisant un des modes de fusion.

D'autres paramètres permettent de modifier la _Luminosité_, la _Saturation_ et le _Contraste_. Vous pouvez également utiliser un _Dégradé de gris_ et le faire _Par clarté_, _Par luminosité_, ou _Par moyenne_. Pour une teinte de couleur, vous pouvez en modifier la _Force_

### Ré-échantillonnage

Les options de _Ré-échantillonnage_ déterminent l'apparence d'un raster quand vous zoomez ou dé-zoomez. Différents modes de ré-échantillonnage permettent d'optimiser l'apparence d'un raster. Ils calculent une nouvelle matrice de valeurs via une transformation géométrique.

![](../../../../images/rasterRenderAndRessampling.png)

En appliquant la méthode _Plus proche voisin_, le raster peut apparaître pixelisé lorsque l'on zoome dessus. Ce rendu peut être amélioré en choisissant les méthodes _Bilinéaire_ ou _Cubique_ qui adoucissent les angles. L'image est alors lissée. Ces méthodes sont adaptées par exemple aux rasters d'élévation.

## Onglet Transparence

KADAS permet d'afficher chaque raster à des niveaux de transparence différents. Utilisez le curseur de transparence ![slider](../../../../images/slider.png) pour indiquer dans quelle mesure les couches sous-jacentes (s'il y en a) pourront être visibles à travers cette couche raster. Cela est très utile, si vous désirez superposer plus d'une couche raster (par exemple une carte des reliefs ombrés superposée à une carte raster classifiée). Cela donnera un rendu proche d'un rendu en trois dimensions.

De plus, vous pouvez entrer une valeur raster qui sera traitée comme _NODATA_ dans _Valeur nulle supplémentaire_.

Un moyen encore plus flexible de personnaliser la transparence est d'utiliser la section _Options de transparence personnalisée_. La transparence de chaque pixel peut être définie dans cet onglet.

Par exemple, pour donner une transparence de 20% à l'eau sur notre raster d'exemple `landcover.tif`, les étapes suivantes sont nécessaires:

1. Chargez le raster `landcover.tif`.
2. Ouvrez la boîte de dialogue _Propriétés de la couche_ en double-cliquant sur le nom du raster dans la légende ou avec un clic droit et en choisissant _Propriétés_ dans le menu qui apparaît.
3. Sélectionnez l'onglet _Transparence_.
4. Dans la liste _Bande de transparence_, choisissez _Aucune_.
5. Cliquez sur le bouton ![](../../../../images/mActionSignPlus.png) _Ajouter des valeurs manuellement_. Une nouvelle ligne apparait dans la liste des pixels.
6. Entrez la valeur raster dans les colonnes _De_ et _Vers_ (mettez la valeur 0) puis ajustez la transparence à 20%.
7. Cliquez sur le bouton **[Appliquer]** et regardez la carte.

Vous pouvez répéter les étapes 5 et 6 pour personnaliser la transparence d'autres valeurs.

Comme vous pouvez le voir, il est assez facile de définir une transparence personnalisée, mais cela peut prendre un peu de temps. Par conséquent, vous pouvez utiliser le bouton ![](../../../../images/mActionFileSave.png) _Exporter dans un fichier_ pour sauver vos paramètres de transparence dans un fichier. Le bouton ![](../../../../images/mActionFileOpen.png) _Importer depuis le fichier_ charge vos paramètres de transparence et les applique à la couche raster actuelle.

## Onglet Métadonnées

L'onglet _Métadonnées_ affiche de nombreuses informations sur la couche raster, dont les statistiques sur chaque bande de la couche raster. Les informations sont regroupées par section: _Description_, _Attribution_, _URL Métadonnées_ et _Propriétés_. Les statistiques sont recueillies _à la demande_, de sorte qu'il est possible que les statistiques sur une couche n'aient pas encore été collectées.

![](../../../../images/rasterMetadata.png)
