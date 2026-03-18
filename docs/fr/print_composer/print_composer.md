<!-- Recovered from: docs_old/html/fr/fr/print_composer/print_composer/index.html -->
<!-- Language: fr | Section: print_composer/print_composer -->

# Composeur d'Impression

Avec le Composeur d'Impression vous pouvez créer de jolies cartes et des atlas qui peuvent être imprimés ou sauvegardés en tant que fichier PDF, image ou fichier SVG. C'est un moyen puissant de partager de l'information géographique produite avec KADAS qui peut être incluse dans des rapports ou publiée.

Le Composeur d'Impression fournit des fonctionnalités de plus en plus riches de mise en page et d'impression. Il vous permet d'ajouter des éléments tels que le canevas de carte KADAS, des zones de textes, des images, des légendes, des échelles graphiques, des formes de base, des flèches, des tables attributaires et des cadres HTML. Vous pouvez modifier la taille, grouper, aligner positionner et faire tourner chaque élément et ajuster leurs propriétés pour créer votre mise en page. Le résultat peut être imprimé ou exporté dans plusieurs formats d'image, en PostScript, PDF et SVG. Vous pouvez aussi l'enregistrer dans un modèle de mise en page de carte pour l'utiliser dans d'autres projets. Enfin vous pouvez générer un ensemble de cartes automatiquement grâce au Générateur d'Atlas. Une vue d'ensemble des fonctions disponibles est donnée dans la liste suivante:

- ![](../../images/mActionFileSave.png) _Enregistrer le projet_
- ![](../../images/mActionNewComposer.png) _Nouveau composeur_
- ![](../../images/mActionDuplicateComposer.png) _Dupliquer une composition_
- ![](../../images/mActionComposerManager.png) _Gestionnaire de Compositions_
- ![](../../images/mActionFileOpen.png) _Charger depuis un modèle_
- ![](../../images/mActionFileSaveAs.png) _Enregistrer le modèle_
- ![](../../images/mActionFilePrint.png) _Imprimer ou exporter en PostScript_
- ![](../../images/mActionSaveMapAsImage.png) _Exporter dans un format d'image_
- ![](../../images/mActionSaveAsSVG.png) _Exporter au format SVG_
- ![](../../images/mActionSaveAsPDF.png) _Exporter au format PDF_
- ![](../../images/mActionUndo.png) _Annuler la dernière modification_
- ![](../../images/mActionRedo.png) _Restaurer la dernière modification_
- ![](../../images/mActionZoomFullExtent.png) _Zoom sur l'emprise totale_
- ![](../../images/mActionZoomActual.png) _Zoomer à 100%_
- ![](../../images/mActionZoomIn.png) _Zoom +_
- ![](../../images/mActionZoomIn.png) _Zoom -_
- ![](../../images/mActionDraw.png) _Rafraîchir la vue_
- ![](../../images/mActionPan.png) _Déplacer le composeur_
- ![](../../images/mActionZoomToSelected.png) _Zoomer sur une zone spécifique_
- ![](../../images/mActionSelect.png) _Sélectionner/déplacer les objets dans le composeur de cartes_
- ![](../../images/mActionMoveItemContent.png) _Déplacer le contenu_
- ![](../../images/mActionAddMap.png) _Ajouter une nouvelle carte à partir de la fenêtre principale de KADAS_
- ![](../../images/mActionAddImage.png) _Ajouter une image au composeur de cartes_
- ![](../../images/mActionLabel.png) _Ajouter une étiquette au composeur de cartes_
- ![](../../images/mActionAddLegend.png) _Ajouter une nouvelle légende au composeur de cartes_
- ![](../../images/mActionScaleBar.png) _Ajouter une nouvelle échelle graphique au composeur d'impression_
- ![](../../images/mActionAddBasicShape.png) _Ajouter une forme basique au composeur de cartes_
- ![](../../images/mActionAddArrow.png) _Ajouter une flèche au composeur de cartes_
- ![](../../images/mActionOpenTable.png) _Ajouter une table d'attributs dans le composeur de cartes_
- ![](../../images/mActionAddHtml.png) _Ajouter du HTML_
- ![](../../images/mActionGroupItems.png) _Grouper des objets du composeur d'impression_
- ![](../../images/mActionUngroupItems.png) _Dégrouper des objets du composeur d'impression_
- ![](../../images/locked.png) _Verrouiller les objets sélectionnés_
- ![](../../images/unlocked.png) _Déverrouiller tous les objets_
- ![](../../images/mActionRaiseItems.png) _Remonter l'objet sélectionné_
- ![](../../images/mActionLowerItems.png) _Descendre l'objet sélectionné_
- ![](../../images/mActionMoveItemsToTop.png) _Amener les objets sélectionnés au premier plan_
- ![](../../images/mActionMoveItemsToBottom.png) _Descendre les objets sélectionnés en arrière plan_
- ![](../../images/mActionAlignLeft.png) _Aligner les objets sélectionnés à gauche_
- ![](../../images/mActionAlignRight.png) _Aligner les objets sélectionnés à droite_
- ![](../../images/mActionAlignHCenter.png) _Aligner les objets sélectionnés au centre_
- ![](../../images/mActionAlignVCenter.png) _Aligner les objets sélectionnés au centre verticalement_
- ![](../../images/mActionAlignTop.png) _Aligner les objets sélectionnés en haut_
- ![](../../images/mActionAlignBottom.png) _Aligner les objets sélectionnés en bas_
- ![](../../images/mIconAtlas.png) _Aperçu de l'atlas_
- ![](../../images/mActionAtlasFirst.png) _Première entité_
- ![](../../images/mActionAtlasPrev.png) _Entité précédente_
- ![](../../images/mActionAtlasNext.png) _Entité suivante_
- ![](../../images/mActionAtlasLast.png) _Dernière entité_
- ![](../../images/mActionFilePrint.png) _Impression de l'Atlas_
- ![](../../images/mActionSaveMapAsImage.png) _Exporter l'Atlas en tant qu'images_
- ![](../../images/mActionAtlasSettings.png) _Paramètres de l'Atlas_

Tous les outils du composeur d'impression sont disponibles dans les menus et la barre d'outils. Cette barre peut être affichée ou masquée en effectuant un clic droit dessus.

## Aperçu du Composeur d'impression

L'ouverture du Composeur d'Impression vous donne un canevas blanc qui représente la surface de papier lorsqu'on utilise l'option d'impression. Initialement, vous trouvez des boutons sur le côté gauche du canevas pour ajouter des éléments du composeur de carte; le canevas de carte KADAS courant, les étiquettes de texte, les images, les légendes, les échelles graphiques, les formes de base, les flèches, les tables attributaires et les cadres HTML. Dans cette barre d'outils, vous trouver aussi des boutons de barre d'outils pour naviguer, zoomer sur une zone et déplacer la vue sur le composeur et des boutons de barre d'outils pour sélectionner un élément du composeur de carte et déplacer le contenu de l'élément carte.

La figure suivante montre la vue initiale du Composeur d'Impression avant qu'aucun élément n'y soit ajouté.

![](../../images/print_composer_blank.png)

Sur la droite, à côté du canevas, vous trouverez deux panneaux. Le panneau supérieur contient les onglets _Éléments_ et _Historique des commandes_ et le panneau inférieur contient les onglets _Composition_, _Propriétés de l'objet_ et _Génération d'atlas_.

- L'onglet _Éléments_ fournit une liste de tous les éléments du Composeur d'Impression ajouté au canevas.
- L'onglet _Historique des commandes_ affiche un historique des changements effectués dans le Composeur d'Impression. Avec un clic droit, il est possible de défaire et refaire des actions jusqu'à l'état choisi.
- L'onglet _Composition_ vous permet de définir la taille du papier, l'orientation, l'arrière-plan, le nombre de pages et la qualité d'impression pour le fichier de sortie en dpi/ppp. De plus vous pouvez activer l’![](../../images/checkbox.png) _Impression raster_ qui permet de convertir tous les éléments en raster avant impression ou export en Postscript ou PDF. Vous pouvez également personnaliser les paramètres pour la grille et les guides.
- L'onglet _Propriétés de l'objet_ affiche les propriétés pour l'élément sélectionné sur la mise en page. Cliquez sur l'icône ![](../../images/mActionSelect.png) _Sélectionner/déplacer un objet_ pour sélectionner un élément (par exemple l'échelle graphique ou une étiquette) dans la feuille. Puis cliquez sur l'onglet _Propriétés de l'objet_ et personnalisez les paramètres de l'élément sélectionné.
- L'onglet _Génération d'atlas_ permet d'activer la création d'un atlas en sortie de composeur et d'en gérer les paramètres.
- Enfin, vous pouvez enregistrer votre mise en page avec le bouton ![](../../images/mActionFileSave.png) _Enregistrer le projet_.

En bas de la fenêtre de Composeur d'Impression, vous trouverez la barre d'état avec la position du curseur de la souris, le numéro de page et une liste déroulante permettant de choisir un niveau de zoom.

Vous pouvez ajouter de nombreux éléments au composeur. Il est également possible d'avoir plusieurs vues d'une carte, légendes ou échelles graphiques dans le canevas du Composeur d'Impression, sur une ou plusieurs pages. Chaque élément possède ses propres propriétés et dans le cas d'une carte, sa propre emprise géographique. Si vous voulez supprimer un élément du canevas du Composeur, vous pouvez le faire en utilisant les touches `Suppr.` ou `Retour arrière`.

### Outils de navigation

Pour se déplacer sur la mise en page, quelques outils sont proposés:

- ![](../../images/mActionZoomIn.png) _Zoom +_
- ![](../../images/mActionZoomOut.png) _Zoom -_
- ![](../../images/mActionZoomFullExtent.png) _Zoom sur l'emprise totale_
- ![](../../images/mActionZoomActual.png) _Zoomer à 100%_
- ![](../../images/mActionDraw.png) _Rafraîchir la vue_ pour actualiser l'affichage si nécessaire
- ![](../../images/mActionPan.png) _Déplacer le composeur_
- ![](../../images/mActionZoomToSelected.png) _Zoom_ (zoom sur une zone spécifique du Composeur)

Vous pouvez changer le niveau de zoom avec la molette de la souris ou la liste déroulante de la barre d'état. Si vous avez besoin de vous déplacer au sein du Composeur, vous pouvez maintenir la `barre espace` ou la molette de la souris enfoncée. Avec `Ctrl + barre espace`, vous passez temporairement en mode zoom + et avec `Ctrl + Shift + barre espace`, en mode zoom -.

## Session test

Pour savoir comme créer une carte, veuillez suivre les instructions suivantes.

1. Sur le côté gauche, sélectionnez le bouton de barre d'outils ![](../../images/mActionAddMap.png) _Ajouter une nouvelle carte_ et dessinez un rectangle. A l'intérieur du rectangle dessiné, la carte de la vue KADAS est affichée .
2. Sélectionnez le bouton de la barre d'outils ![](../../images/mActionScaleBar.png) _Ajouter une nouvelle échelle graphique_ et placez l'élément de la carte avec le bouton gauche de la souris dans le canevas du Composeur d'Impression. Une barre d'échelle sera ajoutée au canevas.
3. Sélectionnez le bouton de la barre d'outils ![](../../images/mActionAddLegend.png) _Ajouter une nouvelle légende_ et dessinez un rectangle dans le canevas en maintenant enfoncé le bouton gauche de la souris. À l'intérieur du rectangle dessiné, la légende sera affichée.
4. Sélectionnez l'icône ![](../../images/mActionSelect.png) _Sélectionner/Déplacer un objet_ pour sélectionner la carte sur le canevas et déplacez-le un peu.
5. Tant que l'élément de carte est toujours sélectionné, vous pouvez aussi changer sa taille. Cliquez tout en maintenant enfoncé le bouton gauche de la souris sur un des coins de l'élément marqué par un petit rectangle blanc et déplacez-le pour changer la taille de la carte.
6. Cliquez sur l'onglet _Propriétés de l'élément_ dans le panneau inférieur gauche et trouvez le réglage pour l'orientation. Modifiez la valeur du paramètre _Orientation de la carte_ sur'15.00° '. Vous devriez voir l'orientation de l'élément de carte changer.
7. Enfin, vous pouvez enregistrer votre mise en page avec le bouton ![](../../images/mActionFileSave.png) _Enregistrer le projet_.

## Options du Composeur d'Impression

Via le menu _Préférences ‣ Options du composeur_, vous pouvez définir les paramètres qui seront utilisés par défaut pendant votre travail.

- _Valeurs par défaut pour les compositions_ permet de spécifier la police de caractère par défaut.
- Dans _Apparence de la grille_, vous définissez le style et la couleur de la grille. Il y a trois styles de grille: lignes en **Pointillés** ou **Continues** et **Croix**.
- _Grille et guides par défaut_ définit l'espacement, le décalage et la tolérance de la grille.

## Onglet Composition — Paramètres généraux de mise en page

Dans l'onglet _Composition_, vous pouvez définir les paramètres généraux de votre mise en page.

- Vous pouvez choisir un des formats _Préconfigurés_ de papier ou entrer vos valeurs personnelles de _Largeur_ et de _Hauteur_.
- Une composition peut maintenant se répartir sur plusieurs pages. Par exemple, une première page montrant la carte, une deuxième la table d'attributs d'une des couches et une troisième un cadre HTML en lien avec le site internet de votre organisme. Choisissez le _Nombre de pages_ à votre convenance. Vous pouvez choisir l’_Orientation_ et la _Résolution de l'export_. Lorsque la case ![](../../images/checkbox.png) _Impression raster_ est cochée, tous les éléments seront rasterisés avant l'impression ou la sauvegarde en PostScript ou PDF.
- _Grille et guides_ vous permet de personnaliser les paramètres de la grille, comme _Espacement de la grille_, _Décalage de la grille_ et _Tolérance d'accrochage_, à vos besoins. La tolérance est la distance maximale en-dessous de laquelle un objet est aimanté sur les guides intelligents.

L'accrochage à la grille et/ou aux guides peut s'activer depuis le menu _Vue_. Vous pouvez également y choisir de cacher ou afficher la grille et les guides.

## Options communes des éléments du composeur

Les éléments du composeur disposent d'un ensemble de propriétés générales définies en bas de l'onglet _Propriétés de l'objet_: Position et taille, Rotation, Cadre, Fond, Identifiant de l'objet, Rendu.

![](../../images/print_composer_common_properties.png)

- _Position et taille_ permet de définir la taille et la position du cadre contenant l'élément. Vous pouvez également choisir le _Point de référence_ dont les coordonnées **X** et **Y** sont définies.
- _Rotation_ permet de définir un angle de rotation (en degrés) pour l'élément.
- Le ![](../../images/checkbox.png) _Cadre_ permet d'afficher ou de cacher le cadre autour du label. Cliquez sur _Couleur du cadre_ et _Épaisseur_ pour modifier ces propriétés.
- Utilisez le menu _Couleur d'arrière-plan_ pour définir une couleur d'arrière-plan. Avec la boîte de dialogue, vous pouvez choisir une couleur.
- Utilisez l’_Identifiant de l'objet_ pour créer une lien avec d'autres éléments du Composeur. Ceci est utilisé par KADAS Serveur et tout client web potentiel. Vous pouvez donner un ID à un élément (par ex. une carte, une zone de texte), puis le client web peut envoyer des informations pour spécifier les propriétés de cet objet. La commande GetProjectSettings listera les éléments disponibles dans la mise en page avec leurs ID.
- _Rendu_ permet de choisir différents modes.
- L'icône ![](../../images/mIconDataDefine.png) _Défini par des données_ à côté d'un champ signifie que vous pouvez associer le champ avec des données dans l'élément de carte ou utiliser des expressions. Elles sont particulièrement utiles avec la génération d'atlas.

KADAS propose maintenant des modes de rendu avancés pour les éléments du composeur, comme pour les couches vecteur et raster.

![](../../images/rendering_mode.png)

- _Transparence_ ![slider](../../images/slider.png): Vous permet de rendre visible les couches situées en dessous. Utiliser le curseur pour adapter la visibilité de la couche vectorielle à vos besoins. Vous pouvez également définir directement le pourcentage de transparence dans la zone de texte située à côté.
- ![](../../images/checkbox.png) _Exclure cet élément des exports_: Vous pouvez décider de faire un élément non visible dans tous les exports. Après avoir activé cette option, l'élément ne sera pas inclus dans les PDF, impressions etc...
- _Mode de fusion_: Vous pouvez donner des effets de rendu spéciaux grâce à cet outil bien connu des logiciels de dessin. Les pixels de l'objet et des éléments sous-jacents sont fusionnés selon les manières décrites ci-dessous.

  > - Normal: Il s'agit du mode de fusion standard qui utilise la valeur de transparence (canal alpha) du pixel supérieur pour le fusionner avec le pixel sous-jacent, les couleurs ne sont pas mélangées.
  > - Eclaircir: Sélectionne le maximum entre chaque composante depuis les pixels du premier-plan et de l'arrière-plan. Soyez attentif au fait que le résultat obtenu peut présenter un aspect dur et crénelé.
  > - Filtrer: Les pixels lumineux de la source sont affichés par dessus la destination, alors que les pixels sombres ne le sont pas. Ce mode est utile pour mélanger la texture d'une couche avec une autre (ie vous pouvez utiliser un relief ombré pour texturer une autre couche).
  > - Éviter: Ce mode va éclaircir et saturer les pixels sous-jacents en se basant sur la luminosité du pixel au-dessus. La brillance des pixels supérieurs vont donc provoquer une augmentation de la saturation et de la brillance des pixels inférieurs. Cela fonctionne mieux si les pixels supérieurs ne sont pas lumineux, sinon l'effet sera trop prononcé.
  > - Addition: Ce mode de fusion ajoute simplement les valeurs de pixels d'une couche avec une autre. Dans le cas de valeurs obtenues au-dessus de 1 (en ce qui concerne le RVB), du blanc sera affiché. Ce mode est approprié pour mettre en évidence des entités.
  > - Assombrir: Ce mode créé un pixel résultant qui conserve le plus petit composants parmi les pixels du premier-plan et de l'arrière-plan. Comme avec le mode éclaircir, le résultat peut présenter un aspect dur et crénelé.
  > - Multiplier: Dans ce cas, les valeurs pour chaque pixel de la couche supérieure sont multipliées par celles des pixels correspondants de la couche inférieure. Les images obtenues sont plus sombres.
  > - Découper: Les couleur sombres de la couche supérieure provoquent un obscurcissement des couches inférieures. Découper peut être utilisé pour ajuster et teinter les couches inférieures.
  > - Revêtement: Ce mode combine les modes multiplier et filtrer. Dans l'image résultante, les parties lumineuses deviennent plus lumineuses et les parties sombres plus sombres.
  > - Lumière douce: Ce mode est très similaire au mode revêtement, mais au lieu d'utiliser multiplier/filtrer il utilise découper/éviter. Il est censé émuler une lumière douce rayonnante dans l'image.
  > - Lumière dure: Ce mode est très similaire au mode revêtement. Il est censé émuler une lumière très intense projetée dans l'image.
  > - Différencier: Ce mode soustrait le pixel supérieur au pixel inférieur et vice-versa, de façon à toujours obtenir une valeur positive. Le mélange avec du noir ne produit aucun changement, étant donné que toutes les couleurs sont nulles.
  > - Soustraire: Ce mode soustrait les valeurs de pixel d'une couche avec une autre. En cas de valeurs négatives obtenues, du noir est affiché.

  ## L'élément Carte

Cliquez sur le bouton ![](../../images/mActionAddMap.png) _Ajouter une nouvelle carte_ de la barre d'outils du composeur pour ajouter la carte telle qu'affichée dans la fenêtre principale de KADAS. Tracez ensuite un rectangle sur la mise en page avec le bouton gauche de la souris. Concernant l'affichage de la carte, vous pouvez choisir entre trois modes différents depuis l'onglet _Propriétés de l'objet_:

- **Rectangle** est l'option par défaut. Elle n'affiche qu'un cadre vide avec un message _La carte sera imprimée ici_.
- **Cache** affiche la carte dans sa résolution d'écran actuelle. Si vous zoomez sur le Composeur, la carte ne sera pas actualisée, mais l'image sera mise à l'échelle.
- **Rendu** signifie que, si vous faites un zoom sur le Composeur, la carte sera actualisée, mais pour des raisons de performances, une résolution maximale a été prédéfinie.

**Cache** est le mode d'aperçu par défaut pour un Composeur nouvellement créé.

Vous pouvez redimensionner l'élément de la carte en cliquant sur le bouton ![](../../images/mActionSelect.png) _Sélectionner/déplacer un objet_, en sélectionnant l'élément, et en déplaçant un des curseurs bleus dans le coin de la carte. Avec la carte sélectionnée, vous pouvez maintenant adapter plus de propriétés dans l'onglet _Propriétés de l'objet_.

Pour déplacer les couches au sein de l'élément carte, sélectionnez-le puis cliquez sur l'icône ![](../../images/mActionMoveItemContent.png) _Déplacer le contenu de l'objet_ et déplacez les couches dans le cadre de l'élément de carte avec le bouton gauche de la souris. Après avoir trouvé le bon emplacement, vous pouvez figer la position de cet élément au sein du Composeur. Sélectionnez l'élément de carte et utilisez l'outil ![](../../images/locked.png) _Verrouiller les objets sélectionnés_ ou encore dans la colonne verrouillage de l'onglet _Éléments_. Un objet verrouillé ne peut être sélectionné qu'en utilisant l'onglet _Éléments_. Une fois sélectionné, il est possible d'utiliser ce même onglet pour le déverrouiller. L'icône ![](../../images/unlocked.png)_Déverrouiller tous les objets_ permet de déverrouiller tous les objets du composeur.

### Propriétés principales

La zone _Propriétés principales_ de l'onglet _Propriétés de l'objet_ de la carte propose les fonctionnalités suivantes:

![](../../images/print_composer_map1.png)

- Les options d’**Aperçu** vous permettent de choisir parmi les modes _Cache_, _Rendu_ ou _Rectangle_ comme décrits ci-dessus. Si vous changez la vue dans la fenêtre principale de KADAS en modifiant des couches vecteurs ou raster, vous pouvez mettre à jour le Composeur en sélectionnant l'élément carte puis en cliquant sur le bouton **[Mise à jour de l'aperçu]**.
- Le champ _Échelle_ ![](../../images/selectnumber.png) permet de préciser manuellement une valeur d'échelle.
- Le champ _Rotation de carte_ ![](../../images/selectnumber.png) vous permet de définir l'angle de rotation de l'élément de la carte, dans le sens horaire, en degrés. C'est ici que la rotation de la vue de carte peut être imitée. Notez qu'un cadre de coordonnées correct ne peut être obtenu que lorsque l'angle de rotation a la valeur par défaut de 0 et que lorsqu'une _Rotation de carte_ est définie, il ne peut plus être modifié.
- ![](../../images/checkbox.png) _Dessiner les objets du canevas de la carte_ permet de montrer les annotations placées sur la carte dans la fenêtre principale de KADAS.
- Vous pouvez choisir de verrouiller les couches affichées sur la carte. Cochez ![](../../images/checkbox.png) _Verrouiller les couches pour cette carte_. Après cela, toute couche qui serait rendue visible ou invisible sur la carte de la fenêtre principale de KADAS n'apparaîtra ou ne disparaîtra pas de la carte dans le composeur. Mais, le style et l'étiquetage des couches verrouillées sont toujours mis à jour par la fenêtre principale de KADAS. Vous pouvez éviter ce comportement en utilisant _Verrouiller les styles de couche pour cette carte_.
- Le bouton ![](../../images/mActionShowPresets.png) vous permet d'ajouter rapidement toutes les vues prédéfinies que vous avez préparées dans QGIS. En cliquant sur le bouton ![](../../images/mActionShowPresets.png), vous pourrez consulter la liste des vues prédéfinies et sélectionner celle que vous voulez afficher. Le canevas de carte verrouillera automatiquement les couches prédéfinies en activant la ![](../../images/checkbox.png) _Verrouiller les couches pour cette carte_. Si vous voulez désélectionner ce qui est prédéfini, décochez la ![](../../images/checkbox.png) et appuyez sur le bouton ![](../../images/mActionDraw.png). Consultez _Légende de la carte_ pour voir comment créer des vues prédéfinies.

### Emprise

La zone _Aperçus_ de l'onglet _Propriétés de l'objet_ de la carte propose les fonctionnalités suivantes:

![](../../images/print_composer_map2.png)

- L’**Emprise** vous permet de définir l'emprise de la carte en utilisant les valeurs X et Y minimales et maximales puis de cliquer sur le bouton **[Fixer sur l'emprise courante du canevas de la carte]**. Ce bouton paramètre l'emprise de la carte du composeur avec l'emprise de la vue courante dans l'application KADAS. Le bouton **[Voir l'étendue sur la carte]** fait exactement l'inverse: il met à jour l'emprise de la carte dans l'application QGIS avec l'étendue de la carte dans le composeur.

Si vous changez l'affichage sur la toile du canevas en changeant les propriétés vectorielles ou matricielles, vous pouvez mettre à jour l'affichage de Print Composer en sélectionnant l'élément de carte dans Print Composer et en cliquant sur le bouton **[Actualiser prévisualisation]** dans l'onglet _Propriétés des éléments_ de l'élément carte.

### Graticules

La boîte de dialogue _Graticules_ de l'onglet _Propriétés de l'objet_ de la carte propose la possibilité d'ajouter plusieurs graticules à l'élément carte.

- Avec les boutons plus et moins, vous pouvez ajouter ou enlever une grille sélectionnée.
- Avec les boutons haut et bas, vous pouvez déplacer une grille dans la liste et configurer la priorité d'affichage.

Lorsque vous double-cliquez sur la grille ajoutée, vous pouvez lui donner un autre nom.

![](../../images/map_grids.png)

Après avoir ajouté un graticule, vous pouvez activer l'option ![](../../images/checkbox.png) _Afficher le graticule_ pour superposer une grille sur l'élément carte. Développez cette option pour accéder à de nombreuses options de configuration.

![](../../images/draw_grid.png)

Comme type de graticule, vous pouvez utiliser _Continue_, _Croix_, _Marqueurs_ ou _Cadre et annotation seulement_. _Cadre et annotation seulement_ est tout particulièrement utile lorsque vous travaillez avec des cartes qui ont subi une rotation ou des graticules reprojectés. Dans la section divisions de la boîte de dialogue Cadre du graticule mentionnée ci-dessous vous avez un tel paramètre. La symbologie du graticule peut être définie. De plus, vous pouvez définir l'intervalle dans les directions X et Y, un décalage en X et Y et l'épaisseur utilisée pour les croix ou les lignes du type de graticule.

![](../../images/grid_frame.png)

- Il y a différentes options pour créer le cadre qui contient la carte. Les options suivantes sont disponibles: Pas de cadre, Zébré, Marqueurs à l'intérieur, Marqueurs à l'extérieur, Marqueurs à l'intérieur et à l'extérieur et Cadre simple.
- Avec les paramètres Afficher uniquement la latitude et Afficher uniquement la longitude dans la section Afficher les coordonnées, vous avez la possibilité de prévenir la confusion entre les coordonnées de latitude/y et longitude/x affichées sur le côté lorsque vous travaillez avec des cartes tournées ou des grilles re-projetées.
- Un mode de rendu avancé est également disponible pour les graticules.
- La ![](../../images/checkbox.png) _Afficher les coordonnées_ permet d'ajouter les coordonnées au cadre de la carte. Vous pouvez choisir le format numérique des annotations, les options vont de décimal à degré, minute, seconde, avec ou sans suffixe, alignés ou non Vous pouvez choisir quelles annotations afficher. Les options sont: Tout afficher, Afficher uniquement la latitude, Afficher uniquement la longitude, Désactivé (aucune). Ceci est utile quand une rotation est appliquée à la carte. Les annotations peuvent être placées à l'intérieur ou à l'extérieur du cadre. L'orientation des annotations peut être définie par Horizontal, Ascendant vertical ou Descendant vertical. Finalement, vous pouvez définir la police, la couleur de police, la distance par rapport au cadre et la précision des coordonnées.

![](../../images/grid_draw_coordinates.png)

### Aperçus

La zone _Aperçus_ de l'onglet _Propriétés de l'objet_ de la carte propose les fonctionnalités suivantes:

![](../../images/print_composer_map4.png)

Vous pouvez choisir de créer un aperçu de carte, qui montre l'étendue des autres carte(s) qui sont disponibles dans le composeur. Premièrement, vous devez créer la carte(s) que vous voulez inclure dans l'aperçu de carte. Ensuite, vous créez une carte que vous voulez utiliser comme aperçu de carte, simplement comme une carte normale.

- Avec les boutons plus et moins, vous pouvez ajouter ou enlever un aperçu.
- Avec les boutons haut ou bas, vous pouvez déplacer un aperçu dans la liste et configurer la priorité d'affichage.

Ouvrez _Aperçus_ et cliquez sur le bouton-icône plus vert pour ajouter un aperçu. Initialement, cet aperçu est nommé _Aperçu 1_. Vous pouvez changer le nom lorsque vous double-cliquez sur l'élément nommé _Aperçu 1_ et ensuite le renommer.

Lorsque vous sélectionnez l'élément aperçu dans la liste, vous pouvez le personnaliser.

- L'option ![](../../images/checkbox.png) _Afficher l'aperçu_ <nom\_aperçu>\* doit être activée pour afficher l'étendue du cadre de la carte sélectionnée.
- La liste combo _Cadre de carte_ peut être utilisée pour sélectionner l'élément carte dont les extensions seront affichées sur l'élément carte présent.
- Le _Style du cadre_ vous permet de changer le style du cadre de l'aperçu.
- Le _Mode de fusion_ vous permet de mettre une transparence et un mode de fusion différent.
- Si la case ![](../../images/checkbox.png) _Inverser l'aperçu_ est cochée, un masque est créé: l'emprise de l'autre zone de carte apparaît clairement alors que le reste est mis en transparence en utilisant le mode de fusion choisi.
- La ![](../../images/checkbox.png) _Centrer sur l'aperçu_ paramètre l'emprise du cadre d'aperçu au centre de la carte d'aperçu. Vous pouvez activer uniquement un seul élément d'aperçu au centre lorsque vous avez plusieurs aperçus.

## L'élément Étiquette

Pour ajouter une zone de texte, cliquez sur le bouton ![](../../images/mActionLabel.png) _Ajouter une nouvelle étiquette_, placez l'élément sur la page par un clic-gauche et personnalisez son apparence grâce aux _Propriétés de l'objet_.

L'onglet _Propriétés de l'objet_ d'un élément étiquette propose la fonctionnalité suivante pour l'élément étiquette:

![](../../images/print_composer_label1.png)

### Propriétés principales

- C'est l'endroit où le texte (HTMLou pas) ou l'expression sont à insérer pour être affichés dans le Composeur.
- Le texte saisi peut être interprété comme du code HTML si vous cochez la case ![](../../images/checkbox.png) _Afficher en HTML_. Vous pouvez ainsi insérer une URL, une image cliquable qui renvoie à une page web ou tout autre code plus complexe.
- Vous pouvez également insérer une expression. Cliquez sur **[Insérer une expression...]** pour ouvrir une nouvelle fenêtre. Construisez une expression en choisissant parmi les fonctions disponibles dans la partie gauche de cette fenêtre. Deux catégories de fonctions sont très utiles, notamment lorsque l'on utilise la génération d'atlas: les fonctions de géométrie et d'enregistrement. En bas de la fenêtre, un aperçu du résultat s'affiche.

### Apparence

- Définissez la _Police_ en cliquant sur le bouton **[Police...]** ou une _Couleur de police_ en sélectionnant une couleur via l'outil de sélection de couleur.
- Vous pouvez spécifier des marges horizontale et verticale différentes, en millimètres. Il s'agit de la marge à partir du coin de l'objet. L'étiquette peut être positionnée en dehors de ses limites par exemple lors d'un alignement avec d'autres objets. Dans ce cas, utilisez des valeurs négatives pour les marges.
- Utiliser _Alignement_ est un autre moyen pour positionner votre étiquette. Notez que par exemple, en utilisant _Alignement horizontal_ avec ![radiobuttonon](../../images/radiobuttonon.png)_Au centre_, la _Marge horizontale_ n'est pas prise en compte.

## L'élément Image

Pour ajouter une image, cliquez sur l'icône ![](../../images/mActionAddImage.png) _Ajouter une image_ et placez l'élément sur le Composeur avec le bouton gauche de votre souris. Vous pouvez modifier la position et l'apparence avec l'onglet _Propriétés de l'objet_ après avoir sélectionné l'élément.

L'onglet des _Propriétés principales_ d'une image proposent les fonctionnalités suivantes:

![](../../images/print_composer_image1.png)

Vous devez d'abord sélectionner l'image que vous voulez afficher. Il y a plusieurs moyens de configurez la _Source de l'image_ dans la zone **Propriétés principales**.

1. Utilisez le bouton parcourir ![](../../images/browsebutton.png) de la _Source de l'image_ pour sélectionner un fichier sur votre ordinateur en utilisant la boîte de dialogue de l'explorateur. L'explorateur commencera dans la librairie SVG fournie avec KADAS. Outre _SVG_, vous pouvez aussi sélectionner d'autres formats d'image comme _PNG_ ou _JPG_.
2. Vous pouvez entrer la source directement dans la zone de texte _Source de l'image_. Vous pouvez même fournir une adresse URL distante à une image.
3. Depuis la zone **Rechercher dans les répertoires**, vous pouvez également sélectionner une image depuis _Chargement des aperçus..._ pour définir l'image source.
4. Utilisez le bouton Source de définition ![](../../images/mIconDataDefine.png) pour définir l'image source depuis un enregistrement ou en utilisant une expression régulière.

Avec l'option _Mode de redimensionnement_, vous pouvez définir comment l'image est affichée lorsque le cadre change, ou choisir de redimensionner le cadre de l'élément image afin qu'il s'ajuste avec la taille originale de l'image.

Vous pouvez sélectionner un des modes suivants:

- Zoom: Agrandit l'image au cadre tout en conservant les proportions de l'image.
- Étirement: Étire une image pour l'ajuster à l'intérieur du cadre, ignore les proportions.
- Découper: Utilisez ce mode uniquement pour des images raster, il définit la taille de l'image à la taille de l'image originale sans mise à l'échelle, et le cadre est utilisé pour découper l'image, donc seule la partie de l'image à l'intérieur du cadre est visible.
- Zoom et redimensionnement du cadre: Agrandit l'image pour s'ajuster avec le cadre, puis redimensionne le cadre pour s'ajuster à l'image résultante.
- Redimensionner le cadre à la taille de l'image: Définit la taille du cadre pour correspondre à la taille originale de l'image sans mise à l'échelle.

Sélectionner un mode de redimensionnement peut désactiver les options de l'élément _Placement_ et _Rotation de l'image_. La _Rotation de l'image_ est active pour les modes de redimensionnement _Zoom_ et _Découper_.

Avec le _Position_, vous pouvez sélectionner la position de l'image à l'intérieur de son cadre. La zone **Rechercher dans les répertoires** vous permet d'ajouter ou de supprimer des répertoires avec des images au format SVG de la base de données d'images. Un aperçu des images trouvées dans les répertoires sélectionnés est affiché dans un panneau et peut être utilisé pour sélectionner et configurer la source de l'image.

Les images peuvent être tournées avec le champ _Rotation de l'image_. L'activation de l'option ![](../../images/checkbox.png) _Synchroniser avec la carte_ synchronise la rotation d'une image dans le canevas de carte KADAS (par exemple, une flèche orientée nord) avec l'image appropriée du Composeur d'Impression.

Il est aussi possible de sélectionner directement une flèche nord. Si vous sélectionnez d'abord une image de flèche nord depuis **Rechercher dans les répertoires** et utilisez ensuite le bouton parcourir ![](../../images/browsebutton.png) du champ _Source de l'image_, vous pouvez dès lors sélectionner une des flèches nord de la liste.

Beaucoup de flèches Nord n'ont pas un _N_ ajouté à la flèche Nord, cela est fait exprès pour les langues qui n'utilisent pas un _N_ pour le Nord, de sorte qu'elles puissent utiliser une autre lettre.

![](../../images/north_arrows.png)

## L'élément Légende

Pour ajouter une légende, cliquez sur l'icône ![](../../images/mActionAddLegend.png) _Ajouter une nouvelle légende_ et placez l'élément sur le Composeur avec le bouton gauche de votre souris. Vous pouvez modifier la position et l'apparence avec l'onglet _Propriétés de l'objet_ après avoir sélectionné l'élément.

Les _Propriétés principales_ d'une légende proposent les fonctionnalités suivantes:

![](../../images/print_composer_legend1.png)

### Propriétés principales

La zone _Propriétés principales_ de l'onglet _Propriétés de l'objet_ de la légende propose les fonctionnalités suivantes:

![](../../images/print_composer_legend2.png)

Dans les Propriétés Principales vous pouvez:

- Changer le titre de la légende.
- Définir l'alignement du titre A Gauche, Au centre ou A droite.
- Vous pouvez également choisir à quelle _Carte_ doit correspondre la légende.
- Vous pouvez choisir un caractère qui permet d'insérer des retours à la ligne.

### Éléments de légende

La zone _Objets de légende_ de l'onglet _Propriétés de l'objet_ de la légende propose les fonctionnalités suivantes:

![](../../images/print_composer_legend3.png)

- La légende sera automatiquement mise à jour si ![](../../images/checkbox.png) _Mise à jour auto_ est cochée. Lorsque _Mise à jour auto_ n'est pas cochée, cela vous donnera plus de contrôle sur les éléments de la légende. Les icônes en-dessous de la liste des éléments de légende seront activés.
- La fenêtre des éléments de légende répertorie tous les éléments de la légende et vous permet de changer l'ordre des éléments, de grouper les couches, de supprimer ou de restaurer des éléments de la liste, de modifier les noms des couche et d'ajouter un filtre.
    - L'ordre des éléments peut être changé en utilisant les boutons **[Monter]** et **[Descendre]** ou avec la fonctionnalité _glisser-déposer_. L'ordre ne peut pas être changé pour les graphiques de légende WMS.
    - Utilisez le bouton **[Ajouter un groupe]** pour ajouter un groupe de légende.
    - Utilisez les boutons **[plus]** et **[moins]** pour ajouter ou supprimer des couches.
    - Le bouton **[Éditer]** est utilisé pour modifier le nom de la couche, le nom du groupe ou le titre, vous devez d'abord sélectionner l'élément de la légende.
    - Le bouton **[Sigma]** ajoute un nombre d'entités pour chaque couche vectorielle.
    - Utilisez le bouton **[filtre]** pour filtrer la légende avec le contenu de la carte, seuls les éléments de la légende visibles dans la carte seront listés dans la légende.

  Après avoir changé la symbologie dans la fenêtre principale KADAS, vous pouvez cliquer sur **[Tout mettre à jour ]** pour adapter les changements dans l'élément légende du Composeur d'impression.

### Polices, Colonnes, Symbole et Espacement

Les zones _Polices_, _Colonnes_ et _Symbole_ de la légende dans l'onglet _Propriétés de l'objet_ fournissent les fonctionnalités suivantes:

![](../../images/print_composer_legend4.png)

- Vous pouvez changer la police du titre de la légende, du groupe, du sous-groupe et de l'élément (de couche) dans la légende. Cliquez sur la catégorie concernée pour ouvrir la fenêtre **Choisir une police**.
- Vous pouvez choisir une **Couleur** pour les étiquettes avec le sélecteur de couleur avancé, cependant la couleur sélectionnée sera donnée à tous les éléments de police dans la légende.
- Les éléments de légende peuvent être organisés sur plusieurs colonnes. Configurez le nombre de colonnes dans le champ _Compter_ ![](../../images/selectnumber.png).
    - La case ![](../../images/checkbox.png) _Égaliser la largeur des colonnes_ permet d'ajuster la taille des colonnes de la légende.
    - L'option ![](../../images/checkbox.png) _Séparer les couches_ permet de présenter sur plusieurs colonnes les éléments de légende d'une couche ayant un style catégorisé ou gradué.
- Vous pouvez changer la largeur et la hauteur du symbole de légende ici.

### Légende Graphique WMS et Espacement

Les zones _Légende WMS_ et _Espacement_ de l'onglet _Propriétés de l'objet_ fournissent les fonctionnalités suivantes:

![](../../images/print_composer_legend5.png)

Lorsque vous avez ajouté une couche WMS et que vous insérez un élément de légende du composeur, une requête sera envoyée au serveur WMS pour fournir une légende WMS. Cette Légende sera uniquement affichée si le serveur WMS fournit la capacité GetLegendGraphic. Le contenu de la légende WMS sera fourni comme une image raster.

La _Légende WMS_ est utilisée pour ajuster la _Largeur de la légende_ et la _Hauteur de la légende_ pour la légende WMS des images raster.

L'espacement autour du titre, des groupes, sous-groupes, symboles, libellés de légende, colonnes peut se personnaliser ici.

## L'élément Échelle graphique

Pour ajouter une barre d'échelle, cliquez sur l'icône ![](../../images/mActionScaleBar.png) _Ajouter une nouvelle échelle graphique_, placez l'élément sur le Composeur avec le bouton gauche de votre souris. Vous pouvez modifier la position et son apparence avec le panneau de _Propriétés de l'objet_ après avoir sélectionné l'élément.

Les _Propriétés principales_ d'une barre d'échelle proposent les fonctionnalités suivantes:

![](../../images/print_composer_scalebar1.png)

### Propriétés principales

La zone _Propriétés principales_ de l'onglet _Propriétés de l'objet_ de la barre d'échelle propose les fonctionnalités suivantes:

![](../../images/print_composer_scalebar2.png)

- Choisissez tout d'abord à quelle carte la barre d'échelle sera associée.
- Ensuite, choisissez le style de la barre d'échelle. Six sont disponibles:
    - Les styles **Boîte unique** ou **Boîte double** correspondent à une ou deux lignes de boîtes de couleurs alternées.
    - Repères **au milieu**, **en-dessous** ou **au-dessus** de la ligne,
    - **Numérique**: le ratio d'échelle est affiché (par exemple, 1:50000).

### Unités et segments

Les zones _Unités_ et _Segments_ de l'onglet _Propriétés de l'objet_ de la barre d'échelle proposent les fonctionnalités suivantes:

![](../../images/print_composer_scalebar3.png)

Avec ces deux séries de paramètres, vous pouvez choisir la manière dont la barre d'échelle sera représentée.

- Sélectionnez les unités à utiliser. Il y a quatre choix possibles: **Unités de carte** est l'unité sélectionnée automatiquement; **Mètres**, **Pied** ou **Miles Nautiques** forcent la conversion des unités.
- Le champ _Étiquette_ permet de rentrer le texte à afficher concernant les unités de la barre d'échelle.
- _Unités de carte par unité de l'échelle graphique_ vous permet de préciser un ratio entre l'unité de la carte et les unités utilisées pour la barre d'échelle.
- Vous pouvez définir combien de _Segments_ seront dessinés à gauche et / ou à droite de la barre d'échelle ainsi que leur longueur (champ _Taille_) et leur hauteur (champ _Hauteur_).

### Affichage

La boîte de dialogue _Affichage_ de l'onglet _Propriétés de l'objet_ de l'échelle graphique propose les fonctionnalités suivantes:

![](../../images/print_composer_scalebar4.png)

Vous pouvez définir comment l'échelle graphique sera affichée dans son cadre.

- _Marge de la boîte_: espace entre le texte et les bords du cadre
- _Marge de l'étiquette_: espace entre le texte et l'échelle graphique dessinée
- _Largeur de ligne_: largeur de ligne de l'échelle graphique dessinée
- _Style de jointure_: Coins à la fin de l'échelle graphique dans le style Oblique, Rond ou Angle droit (seulement disponible pour le style d'échelle graphique Boîte unique & Boîte double)
- _Style d'extrémités_: Fin de toutes les lignes dans le style Carré, Rond ou Plat (seulement disponible pour le style d'échelle graphique Repères en-dessus, en-dessous et au milieu de la ligne)
- _Alignement_: Met le texte sur la gauche, au milieu ou à droite du cadre (fonctionne uniquement pour l'Échelle graphique de style Numérique)

### Polices et couleurs

La boîte de dialogue _Polices et couleurs_ de l'onglet _Propriétés de l'objet_ de l'échelle graphique propose les fonctionnalités suivantes:

![](../../images/print_composer_scalebar5.png)

Vous pouvez définir les polices et couleurs utilisées pour l'échelle graphique.

- Utilisez le bouton **[Police]** pour configurer la police
- _Couleur de police_: configure la couleur de police
- _Couleur de remplissage_: configure la première couleur de remplissage
- _Couleur de remplissage secondaire_: configure la seconde couleur de remplissage
- _Couleur du contour_: configure la couleur des lignes de l'Échelle graphique

Les couleurs de remplissage sont uniquement utilisées pour les boîtes de style d'échelle Boîte Unique et Boîte Double. Pour sélectionner une couleur, vous pouvez utiliser l'option liste en utilisant la flèche descendante pour ouvrir une option de sélection de couleur simple ou l'option de sélection de couleur avancée, qui s'ouvre lorsque vous cliquez dans la boîte colorée dans la boîte de dialogue.

## Les éléments Formes simples

Pour ajouter une forme simple (ellipse, rectangle, triangle), cliquez sur l'icône ![](../../images/mActionAddBasicShape.png) _Ajouter une forme simple_ ou sur l'icône ![](../../images/mActionAddArrow.png) _Ajouter une flèche_, placez l'élément en maintenant enfoncé le clic gauche de la souris. Personnalisez l'apparence dans l'onglet _Propriétés de l'objet_.

Lorsque vous maintenez également enfoncé la touche `Shift` lors du placement de la forme simple, vous pouvez créer un carré, un cercle ou un triangle parfait.

![](../../images/print_composer_shape.png)

L'onglet _Forme_ des propriétés de l'objet vous permet de sélectionner si vous voulez dessiner une ellipse, un rectangle ou un triangle à l'intérieur du cadre donné.

Vous pouvez définir le style de la forme en utilisant la boîte de dialogue avancée du style de symbole avec laquelle vous pouvez définir ses couleurs de bordure et de remplissage, son motif de remplissage, son utilisation de symboles etc.

Pour la forme rectangulaire, vous pouvez configurer la valeur du rayon de coin pour arrondir les coins.

À la différence des autres objets du composeur, vous ne pouvez pas personnaliser le cadre ou la couleur du fond du cadre.

## L'élément Flèche

Pour ajouter une flèche, cliquez sur l'icône ![](../../images/mActionAddArrow.png) _Ajouter une flèche_, placez l'élément en maintenant enfoncé le bouton gauche de la souris et tirez une ligne pour dessiner la flèche dans le canevas du Composeur d'Impression et la positionner, puis personnalisez l'apparence de la flèche dans l'onglet _Propriétés de l'objet_.

Lorsque vous maintenez également enfoncée la touche _Shift_ tout en plaçant la flèche, celle-ci est placée dans un angle d'exactement 45°.

L'élément flèche peut être utilisé pour ajouter une ligne ou une simple flèche qui peut être ajoutée, par exemple, pour montrer la relation entre les autres éléments du Composeur d'Impression. Pour créer une flèche nord, l'élément image devrait être considéré d'abord. KADAS a un jeu de flèches Nord en format SVG. De plus, vous pouvez connecter un élément d'image avec une carte donc elle peut pivoter automatiquement avec la carte.

![](../../images/print_composer_arrow.png)

### Propriétés de l'objet

L'onglet _Flèche_ des propriétés de l'objet vous permet de configurer un élément flèche.

Le bouton **[Style de ligne ...]** peut être utilisé pour configurer le style de ligne en utilisant l'éditeur de symbole de style de ligne.

Dans les _Symboles de flèches_, vous pouvez sélectionner un des trois boutons radio.

- _Défaut_: Pour dessiner une flèche régulière, vous donne des options pour personnaliser la tête de flèche
- _Aucun_: Pour dessiner une ligne sans tête de flèche
- _Symbole SVG_: Pour dessiner une ligne avec un _Symbole de départ_ SVG et/ou un _Symbole de fin_ SVG

Pour un symbole Flèche par _Défaut_, vous pouvez utiliser les options suivantes pour personnaliser la tête de flèche.

- _Couleur de bordure de la flèche_: Configure la couleur de bordure de la tête de flèche
- _Couleur de remplissage de la flèche_: Configure la couleur de remplissage de la tête de flèche
- _Largeur de bordure de la flèche_: Configure la largeur de bordure de la tête de flèche
- _Largeur de la tête_: Configure la taille de la tête de flèche

Pour le _Symbole SVG_ vous pouvez utiliser les options suivantes.

- _Marqueur de début_: Choisit une image SVG à dessiner au début de la ligne
- _Marqueur de fin_: Choisit une image SVG à dessiner à la fin de la ligne
- _Largeur de la tête de flèche_: Configure la taille du départ et/ou d'arrivée du symbole de tête

Les images SVG pivotent automatiquement avec la ligne. La couleur de l'image SVG ne peut pas être changée.

## L'élément Table Attributaire

Il est possible d'ajouter des tables attributaires de couches vecteur au Composeur: cliquez sur le bouton ![](../../images/mActionOpenTable.png) _Ajouter une table d'attributs_, placez l'élément sur le Composeur avec un clic-gauche puis personnalisez son apparence via l'onglet des _Propriétés de l'objet_.

Les _Propriétés principales_ d'une table attributaire proposent les fonctionnalités suivantes:

![](../../images/print_composer_attribute1.png)

### Propriétés principales

La zone _Propriétés principales_ de l'onglet _Propriétés de l'objet_ de la table attributaire propose les fonctionnalités suivantes:

![](../../images/print_composer_attribute2.png)

- Pour _Source_, vous pouvez normalement sélectionner seulement _Entités de la couche_.
- Avec _Couche_, vous pouvez choisir à partir des couches vecteurs chargées dans le projet.
- Le bouton **[Actualiser la table de données]** peut être utilisé pour actualiser la table lorsque le contenu actuel de la table a changé.
- Dans le cas où l'option ![](../../images/checkbox.png)_Générer un atlas_ de l'onglet _Génération d'atlas_ est activée, il y a deux nouvelles _Source_ possibles: _Entité courante de l'atlas_ ou _Relation enfant_. Choisir _Entité courante de l'atlas_ implique que vous ne verrez aucune option pour choisir la couche, et l'objet table affichera seulement une ligne avec les attributs de l'entité courante de la couche de couverture. Choisir _Relation enfant_ affichera une nouvelle option pour spécifier le nom de la relation. L'option _Relation enfant_ ne peut être utilisée que si vous avez défini une relation utilisant la couche de couverture comme parent, et affichera les enregistrements enfants de l'objet courant de la couche de couverture.

![](../../images/print_composer_attribute2b.png)

![](../../images/print_composer_attribute2c.png)

- Le bouton **[Attributs...]** ouvre le menu _Sélection d'attributs_, qui peut être utilisé pour changer le contenu visible de la table. Après avoir fait les changements, utilisez le bouton **[OK]** pour appliquer les changements à la table.

  Dans la section _Colonnes_, vous pouvez:

    - Supprimer un attribut: sélectionnez simplement une ligne d'attribut en cliquant n'importe où sur une ligne et cliquez sur le bouton moins pour supprimer l'attribut sélectionné.
    - Pour ajouter de nouveaux attributs, utilisez le bouton plus. A la fin de la liste des colonnes, une nouvelle ligne vide apparaît et vous pouvez sélectionner une cellule vide de la colonne _Attribut_. Vous pouvez sélectionner un champ attributaire à partir de la liste ou vous pouvez construire un nouvel attribut en utilisant une expression (![](../../images/mIconExpression.png) button). Bien sûr vous pouvez modifier tous les attributs existants par le biais d'une expression régulière.
    - Utiliser les flèches monter et descendre pour changer l'ordre des attributs dans la table.
    - Sélectionner une cellule dans la colonne En-tête pour changer l'En-tête, en tapant simplement un nouveau nom.
    - Sélectionner une cellule dans la colonne Alignement et vous pouvez choisir entre alignement Gauche, Centre ou Droit.
    - Sélectionner une cellule dans la colonne Largeur et vous pouvez la changer de Automatique à une largeur en mm, simplement en tapant un nombre. Lorsque vous voulez la remettre à Automatique, utilisez la croix.
    - Le bouton **[Réinitialiser]** peut toujours être utilisé pour le restaurer à ses paramètres d'attribut original.

  Dans la section _Trier_, vous pouvez:

    - Ajouter un attribut pour trier la table avec. Sélectionnez un attribut et Définissez l'ordre de tri en _Croissant_ ou _Décroissant_ et cliquez sur le bouton plus. Une nouvelle ligne est ajoutée à la liste d'ordre de tri.
    - Sélectionner une ligne dans la liste et utiliser les boutons monter et descendre pour changer la priorité du tri au niveau de l'attribut.
    - Utiliser le bouton moins pour supprimer un attribut de la liste de l'ordre de tri.

![](../../images/print_composer_attribute3.png)

### Filtrage des entités

La zone _Filtrage des entités_ de l'onglet _Propriétés de l'objet_ de la table attributaire propose les fonctionnalités suivantes:

![](../../images/print_composer_attribute4.png)

Vous pouvez:

- Définir un nombre de _Lignes maximales_ à afficher.
- Activer ![](../../images/checkbox.png) _Supprimer les lignes en double de la table_ pour montrer seulement les enregistrements uniques.
- Activer ![](../../images/checkbox.png) _Ne montrer que les entités visibles sur la carte_ et sélectionner le _Composeur de carte_ correspondant pour afficher seulement les attributs des entités visibles sur la carte sélectionnée.
- Activer ![](../../images/checkbox.png) _Ne montrer que les entités intersectant l'entité de l'atlas_ est seulement disponible lorsque ![](../../images/checkbox.png) _Générer un atlas_ est activé. Lorsqu'il est activé, il affichera une table avec seulement les entités indiquées sur la carte de cette page en particulier de l'atlas.
- Activer ![](../../images/checkbox.png) _Filtrer avec_ et fournir un filtre en tapant dans la ligne d'entrée ou insérer une expression régulière en utilisant le bouton d'expression ![](../../images/mIconExpression.png). Voici quelques exemples de déclarations de filtrage que vous pouvez utiliser lorsque vous avez chargé la couche des aéroports à partir du jeu de données exemples:
    - `ELEV > 500`
    - `NAME = 'ANIAK'`
    - `NAME NOT LIKE 'AN%`
    - `regexp_match( attribute( $currentfeature, 'USE' )  , '[i]')`

  La dernière expression régulière inclura seulement les aéroports qui ont une lettre _i_ dans le champ d'attribut _USE_.

### Apparence

La zone _Apparence_ de l'onglet _Propriétés de l'objet_ de la table attributaire propose les fonctionnalités suivantes:

![](../../images/print_composer_attribute5.png)

- Cliquer sur ![](../../images/checkbox.png) _Afficher des lignes vides_ pour rendre visible les entrées vides de la table attributaire.
- Avec les _Marges de cellule_, vous pouvez définir les marges autour du texte dans chaque cellule de la table.
- Avec _Afficher l'en-tête_, vous pouvez sélectionner à partir d'une liste une des options par défaut _Sur le premier cadre_, _Sur tous les cadres_, ou \*Pas d'en-tête’.
- L'option _Tables vides_ contrôle ce qui sera affiché lorsque la sélection des résultats est vide.
    - **N'afficher que les en-têtes** affichera seulement l'en-tête, excepté si vous avez choisi _Pas d'en-tête_ pour _Afficher l'en-tête_.
    - **Masquer la table entière** affichera seulement le fond de la table. Vous pouvez activer ![](../../images/checkbox.png) _Ne pas afficher le fond si le cadre est vide_ dans _Cadres_ pour cacher complètement la table.
    - **Afficher des lignes vides** remplira la table attributaire avec des cellules vides, cette option peut aussi être utilisée pour proposer des cellules vides supplémentaires lorsque vous avez un résultat à montrer !
    - **Afficher le message défini** affichera l'en-tête et ajoutera une cellule couvrant toutes les colonnes et affichera un message comme _Pas de résultat_ qui peut être proposé dans l'option _Message à afficher_
- L'option _Message à afficher_ est seulement activée lorsque vous avez sélectionné **Afficher le message défini** pour _Table vide_. Le message proposé sera affiché dans la table sur la première ligne, lorsque le résultat est une table vide.
- Avec _Couleur de fond_, vous pouvez définir la couleur de fond de la table.

### Afficher les bordures

La boîte de dialogue _Afficher les bordures_ de l'onglet _Propriétés de l'objet_ de la table attributaire propose les fonctionnalités suivantes:

![](../../images/print_composer_attribute6.png)

- Activer ![](../../images/checkbox.png) _Afficher les bordures_ lorsque vous voulez afficher les bordures des cellules de la table.
- Avec _Épaisseur du trait_ vous pouvez définir l'épaisseur des lignes utilisées pour les bordures.
- La _Couleur_ des bordures peut être définie en utilisant la boîte de dialogue de sélection de couleur.

### Styles de polices et de textes

La boîte de dialogue _Styles de polices et de textes_ de l'onglet _Propriétés de l'objet_ de la table attributaire propose les fonctionnalités suivantes:

![](../../images/print_composer_attribute7.png)

- Vous pouvez définir la _Police_ et la _Couleur_ pour l’_En-tête de table_ et le _Contenu de la table_.
- Pour l’_En-tête de table_ vous pouvez en plus définir l’_Alignement_ et choisir entre Suivre l'alignement de la colonne, A gauche, Au centre ou A droite. L'alignement de la colonne est défini en utilisant la boîte de dialogue _Sélection d'attributs_.

### Cadres

La boîte de dialogue _Cadres_ de l'onglet _Propriétés de l'objet_ de la table attributaire propose les fonctionnalités suivantes:

![](../../images/print_composer_attribute8.png)

- Avec le _Mode de redimensionnement_ vous pouvez sélectionner la façon de rendre le contenu de la table attributaire:
    - Utiliser les cadres existants affiche le résultat seulement dans le premier cadre et les cadres ajoutés.
    - Étendre à la page suivante créera autant de cadres (et pages correspondantes) que nécessaire pour afficher l'intégralité de la sélection de la table attributaire. Chaque cadre peut être déplacé autour de la couche. Si vous redimensionnez un cadre, la table résultante sera répartie entre les autres cadres. Le dernier cadre sera rogné pour s'adapter à la table.
    - Répéter jusqu'à la fin créera autant de cadre que pour l'option Étendre à la page suivante sauf que tous les cadres auront la même taille.
- Utiliser le bouton **[Ajouter un cadre]** pour ajouter un autre cadre avec la même taille que le cadre sélectionné. Le résultat de la table qui ne rentre pas dans le premier cadre continuera dans le cadre suivant lorsque vous utilisez le mode Redimensionner Utiliser les cadres existants.
- Activer ![](../../images/checkbox.png) _Ne pas exporter la page si le cadre est vide_ empêche la page d'être exportée lorsque le cadre de la table n'a pas de contenu. Cela signifie que tous les autres éléments du composeur, cartes, échelles graphiques, légendes, etc. ne seront pas visibles dans le résultat.
- Activer ![](../../images/checkbox.png) _Ne pas afficher le fond si le cadre est vide_ empêche le fond d'être affiché lorsque le cadre de la table n'a pas de contenu.

## L'élément cadre HTML

Il est possible d'ajouter un cadre qui affiche le contenu d'un site web ou même de créer et personnaliser votre propre page HTML et de l'afficher !

Cliquez sur l'icône ![](../../images/mActionAddHtml.png) _Ajouter du HTML_, placez l'élément en glissant un rectangle dans le canevas du Composeur d'Impression en maintenant enfoncé le bouton gauche de la souris et positionnez puis personnalisez l'apparence dans l'onglet _Propriétés de l'élément_.

![](../../images/print_composer_html1.png)

### Source du HTML

Comme une source du HTML, vous pouvez soit configurer une URL et activer le bouton radio URL, ou entrer la source du HTML directement dans la zone de texte fournie et activer le bouton radio Source.

La boîte de dialogue _Source du HTML_ de l'onglet _Propriétés de l'objet_ du cadre HTML propose les fonctionnalités suivantes:

![](../../images/print_composer_html2.png)

- Dans _URL_, vous pouvez entrer l'URL d'une page internet que vous avez copiée depuis votre navigateur internet ou sélectionner un fichier HTML en utilisant le bouton Parcourir ![](../../images/browsebutton.png). Il y a aussi la possibilité d'utiliser le bouton de valeurs définies par les données, pour proposer une URL à partir du contenu d'un champ d'attribut d'une table ou en utilisant une expression régulière.
- Dans _Source_, vous pouvez entrer un texte dans la zone de texte avec quelques balises HTML ou proposer une page HTML entière.
- Le bouton **[Insérer une expression]** peut être utilisé pour insérer une expression comme `[%Year($now)%]` dans la zone de texte Source pour afficher l'année courante. Ce bouton est seulement activé lorsque le bouton radio _Source_ est sélectionné. Après avoir inséré l'expression, cliquez quelque part dans la zone de texte avant de rafraîchir le cadre HTML, autrement vous perdrez l'expression.
- Activez ![](../../images/checkbox.png) _Évaluer l'expression QGIS dans la source du HTML_ pour voir le résultat de l'expression que vous avez incluse, autrement vous verrez l'expression à la place.
- Utilisez le bouton **[Rafraîchir la page]** pour rafraîchir le cadre(s) HTML pour voir le résultat des changements.

### Cadres

La boîte de dialogue _Cadres_ de l'onglet _Propriétés de l'objet_ du cadre HTML propose les fonctionnalités suivantes:

![](../../images/print_composer_html3.png)

- Avec _Mode de redimensionnement_, vous pouvez sélectionner la façon de rendre le contenu HTML:
    - Utiliser les cadres existants affiche le résultat seulement dans le premier cadre et les cadres ajoutés.
    - Étendre à la page suivante créera autant de cadres (et de pages) que nécessaire pour afficher la page en entier. Chaque cadre peut être déplacé sur la mise en page. Si vous redimensionnez un cadre, la page web sera à nouveau répartie dans les cadres. Le dernier cadre sera rogné pour s'ajuster à la page web.
    - Répéter sur chaque page répètera la partie supérieure gauche de la page web sur chaque pages du composeur dans des cadres de taille identique.
    - Répéter jusqu'à la fin créera autant de cadre que pour l'option Étendre à la page suivante sauf que tous les cadres auront la même taille.
- Utilisez le bouton **[Ajouter un cadre]** pour ajouter un autre cadre avec la même taille que le cadre sélectionné. Si la page HTML ne va pas dans le premier cadre, elle ira dans le cadre suivant lorsque vous utilisez _Mode de redimensionnement_ ou _Utiliser les cadres existants_.
- Activez ![](../../images/checkbox.png) _Ne pas exporter la page si le cadre est vide_ empêche que la carte mise en page soit exportée lorsque le cadre n'a pas de contenu HTML. Cela signifie que tous les autres éléments du composeur, cartes, barres d'échelle, légendes etc. ne seront pas visibles dans le résultat.
- Activez ![](../../images/checkbox.png) _Ne pas afficher le fond si le cadre est vide_ empêche que le cadre HTML soit affiché si le cadre est vide.

### Utiliser des sauts de page intelligents

La boîte de dialogue _Utiliser des sauts de page intelligents_ de l'onglet _Propriétés de l'objet_ du cadre HTML propose les fonctionnalités suivantes:

![](../../images/print_composer_html4.png)

- Activez ![](../../images/checkbox.png) _Utiliser des sauts de pages intelligents_ pour empêcher le contenu du cadre html de se casser à mi-chemin d'une ligne de texte afin qu'il continue bien dans le cadre suivant.
- Paramètre la _Distance maximale_ autorisée lors du calcul de l'emplacement du saut de page dans le html. Cette distance est la quantité maximale d'espace vide autorisé dans le bas du cadre après calcul de l'emplacement optimal du saut de page. Indiquer une grande valeur permettra de mieux définir l'emplacement du saut de page mais une plus grande quantité d'espace vide sera présent dans le bas des cadres. Cette valeur est utilisée uniquement lorsque _Utiliser des sauts de page intelligents_ est activé.
- Activez ![](../../images/checkbox.png) _Feuille de style utilisateur_ pour appliquer des styles HTML qui sont souvent fournis dans des feuilles de style en cascade. Un exemple de code de style est fourni ci-dessous pour définir la couleur de la balise d'en-tête `<h1>` au vert et définir la police et la taille de police du texte inclu dans les balises de paragraphe `<p>`.

  ```
  h1 {color: #00ff00;
  }
  p {font-family: "Times New Roman", Times, serif;
     font-size: 20px;
  }
  ```
- Utilisez le bouton **[Mise à jour du HTML]** pour voir le résultat des paramètres de la feuille de style.

#### Gestion des éléments

## Taille et position

Chaque élément du Composeur peut être déplacé / redimensionné pour créer une mise en page parfaite. Pour chacune de ces opérations, la première étape est d'activer l'outil ![](../../images/mActionSelect.png) _Sélectionner/Déplacer un objet_ et de cliquer sur l'élément. Vous pouvez ensuite le déplacer avec la souris en maintenant le bouton gauche. Si vous souhaitez limiter les mouvements sur les axes horizontaux ou verticaux, pressez la touche `Shift` du clavier pendant le déplacement de la souris. Si vous avez besoin de plus de précision, vous pouvez déplacer l'élément sélectionné en utilisant les `flèches` du clavier et si les mouvements sont trop lents, utilisez en même temps la touche `Shift`.

Un élément sélectionné apparait avec des carrés à chaque coin du rectangle englobant. Déplacer un de ces carrés avec la souris redimensionnera l'élément dans la direction correspondante. Pendant le redimensionnement, presser la touche `Shift` permettra de maintenir les proportions. Presser la touche `Alt` redimensionnera depuis le centre de l'élément.

La position correcte d'un élément peut être obtenue en utilisant les guides ou l'accrochage à la grille. Les guides sont créés en cliquant et en dessinant dans les règles. Le guide est déplacé en cliquant dans la règle, en alignant le guide et en glissant vers un nouvel endroit. Pour supprimer un guide, déplacez-le en dehors du canevas. Si vous devez désactiver la comportement d'accrochage à la volée, maintenez appuyé la touche `Ctrl` lorsque vous déplacez la souris.

Vous pouvez sélectionner plusieurs éléments en même temps avec le bouton ![](../../images/mActionSelect.png) _Sélectionner/Déplacer un objet_. Pressez simplement la touche `Shift` et cliquez sur tous les éléments souhaités. Vous pouvez ensuite les redimensionner ou les déplacer tous en même temps.

Une fois que vous avez trouvé la position correcte pour un élément, vous pouvez le verrouiller en utilisant les boutons sur la barre d'outils ou en cochant la boîte près de l'élément dans l'onglet _Éléments_. Les éléments verrouillés ne sont **pas** sélectionnables sur le canevas.

Les éléments verrouillés peuvent être déverrouillés en sélectionnant l'élément dans l'onglet _Éléments_ et décochés dans la case à cocher, ou vous pouvez utiliser les boutons dans la barre d'outils.

Pour désélectionner un objet, cliquez dessus en maintenant la touche `Shift` appuyée.

Dans le menu _Éditer_, vous trouverez les actions permettant de sélectionner ou dé-selectionner tous les éléments ou d'inverser la sélection.

## Alignement

Les fonctionnalités pour monter ou descendre des éléments sont présentes dans le menu déroulant ![](../../images/mActionRaiseItems.png) _Relever les objets sélectionnés_. Prenez un élément dans le Composeur de carte et sélectionnez la fonction correspondante pour le monter ou le descendre par rapport aux autres éléments. L'ordre est affiché dans l'onglet _Éléments_. Vous pouvez également monter ou descendre des éléments dans l'onglet _Éléments_ par glissé-déposé dans cette liste.

![](../../images/alignment_lines.png)

Plusieurs options d'alignement sont disponibles via le menu déroulant ![](../../images/mActionAlignLeft.png) _Aligner les objets sélectionnés_. Pour en utiliser une, sélectionner d'abord les éléments puis cliquez sur l'outil d'alignement désiré. Tous les éléments sélectionnés seront alors alignés au sein de leur rectangle englobant commun. Lors du déplacement d'éléments dans le Composeur, des guides apparaissent lorsque les bords, les centres ou les coins sont alignés.

## Copier / Coller des éléments

Le Composeur d'Impression propose des outils permettant de copier/couper/coller des éléments. Comme toujours vous devez d'abord sélectionner les éléments puis utiliser une des options. Vous les trouverez via le menu _Éditer_. Lorsque vous collez des éléments, ils seront positionnés au niveau du curseur de la souris.

Les objets HTML ne peuvent pas être copiés de cette manière. En guise de contournement, utilisez le bouton **[Ajouter un cadre]** dans l'onglet _Propriétés de l'objet_.

Pendant la mise en page de la carte, il est possible d'annuler et refaire des modifications. Cela peut être réaliser à l'aide des outils Annuler la dernière modification et Restaurer la dernière modification:

- ![](../../images/mActionUndo.png) _Annuler la dernière modification_
- ![](../../images/mActionRedo.png) _Restaurer la dernière modification_

Il est également possible de le faire via l’_Historique des commandes_.

![](../../images/command_hist.png)

Le Composeur d'Impression fournit des outils vous permettant de générer automatiquement un ensemble de cartes. L'idée est d'utiliser la géométrie et les attributs d'une couche vectorielle. Pour chaque entité de la couche, une nouvelle carte est générée et son emprise correspond à la géométrie de l'entité. Les attributs de la couche peuvent être utilisés dans des zones de texte.

Un page est générée par entité de la couche. Pour générer un atlas et le paramétrer, allez sur l'onglet Génération d'atlas. Cet onglet propose les éléments suivants:

![](../../images/print_composer_atlas.png)

- ![](../../images/checkbox.png) _Générer un atlas_, qui permet d'activer ou de désactiver la génération d'atlas.
- La liste déroulante _Couche de couverture_ ![](../../images/selectstring.png) permet de choisir la couche (vecteur) contenant les géométries à partir desquelles générer chaque planche.
- La case optionnelle ![](../../images/checkbox.png) _Cacher la couche de couverture_ permet de cacher la couche de couverture sur les planches en sortie.
- La possibilité de _Filtrer avec_ une expression les entités de la couche de couverture. Si une expression est rentrée, seules les entités satisfaisant la condition seront utilisées. Le bouton à droite permet d'ouvrir un constructeur de requête.
- Le _Nom du fichier en sortie_ est utilisé pour générer un nom de fichier pour chaque planche. Il est basé sur une expression. Il n'est utile que lorsque plusieurs fichiers sont produits.
- L’![](../../images/checkbox.png) _Export d'un seul fichier (si possible)_ vous permet de forcer la création d'un unique fichier quand le format de sortie choisi le permet (par exemple le PDF). Si cette case est cochée, le _Nom du fichier en sortie_ n'est pas pris en compte.
- La case optionnelle ![](../../images/checkbox.png) _Trier par_ vous permet de trier les entités de la couche de couverture. La liste déroulante associée permet de choisir un champ à utiliser pour le tri. L'ordre de tri (ascendant ou descendant) est spécifié par le bouton à droite représenté par une flèche ascendante ou descendante.

Vous pouvez utiliser plusieurs objets carte dans la génération d'atlas, chacun sera rendu en fonction de la couche de couverture. Pour activer la génération d'atlas pour un objet carte, vous devez cocher la case ![](../../images/checkbox.png) _Paramètres contrôlés par l'Atlas_ dans les propriétés de l'objet carte. Une fois cochée, vous pouvez définir:

- Un bouton radio ![radiobuttonon](../../images/radiobuttonon.png) _Marge autour des entités_ vous permet de sélectionner la quantité d'espace ajouté autour de chaque géométrie dans la carte. Sa valeur n'a de sens que si vous utilisez le mode mise à l'échelle automatique.
- ![radiobuttonoff](../../images/radiobuttonoff.png) _Echelle prédéfinie_ (meilleur ajustement). Utilise la meilleure option d'ajustement de la liste des échelles prédéfinies dans votre projet (voir _Projet → Propriétés du projet → Général → Echelles du projet_ pour configurer ces échelles prédéfinies).
- Une ![radiobuttonoff](../../images/radiobuttonoff.png) _Échelle fixe_ qui permet de basculer du mode _Marge_ au mode _Échelle fixe_. En échelle fixe, la carte est simplement translatée et centrée sur chaque entité. En mode _Marge_, l'emprise de la carte est calculée de telle sorte que l'entité de la couche de couverture apparaisse entièrement.

## Zones de texte

Pour adapter les étiquettes aux entités à partir desquelles l'atlas génère les planches, vous pouvez utiliser des expressions. Par exemple, pour une couche de ville ayant les champs CITY\_NAME et ZIPCODE, vous pouvez insérer ceci:

```
The area of [% upper(CITY_NAME) || ',' || ZIPCODE || ' is ' format_number($area/1000000,2) %] km2
```

L'information [% upper(CITY\_NAME) || ',' || ZIPCODE || ‘ is ‘ format\_number($area/1000000,2) %] est une expression utilisée dans la zone de texte. Cela sera traduit dans l'atlas généré par:

La superficie de PARIS,75001 est de 1.94 km2

## Boutons de Valeurs définies par des données

Il y a plusieurs endroits où vous pouvez utiliser un bouton ![](../../images/mIconDataDefine.png) _Valeurs définies par des données_ pour définir le paramètre sélectionné. Ces options sont particulièrement utiles avec la Génération d'Atlas.

Pour les exemples suivants, la couche Regions du jeu de données d'exemple de KADAS est utilisée et sélectionnée pour la Génération d'Atlas. Nous supposons également que le format de la page A4 (210X297) est sélectionné dans l'onglet _Composition_ pour le champ _Réglages_.

Avec un bouton Valeurs définies par des données, vous pouvez définir dynamiquement l'orientation de la page. Lorsque la hauteur (nord-sud) de l'emprise d'une région est plus grande que sa largeur (est-ouest), vous devriez plutôt utiliser l'orientation portrait plutôt que paysage pour optimiser l'utilisation de la page.

Dans la _Composition_, vous pouvez définir le champ _Orientation_ et sélectionner Paysage ou Portrait. Nous voulons définir l'orientation dynamiquement en utilisant une expression dépendant de la géométrie de la région. Cliquez sur le bouton ![](../../images/mIconDataDefine.png) du champ _Orientation_, sélectionnez _Éditer_ afin d'ouvrir la boîte de dialogue _Constructeur de chaîne d'expression_. Entrez l'expression suivante:

```
CASE WHEN bounds_width($atlasgeometry) > bounds_height($atlasgeometry) THEN 'Landscape' ELSE 'Portrait' END
```

Maintenant, le papier s'oriente automatiquement pour chaque région où vous devez également repositionner l'élément du composeur. Pour l'élément carte, vous pouvez utiliser le bouton ![](../../images/mIconDataDefine.png) du champ _Largeur_ pour définir dynamiquement cette dernière en utilisant l'expression suivante:

```
(CASE WHEN bounds_width($atlasgeometry) > bounds_height($atlasgeometry) THEN 297 ELSE 210 END) - 20
```

Utilisez le bouton ![](../../images/mIconDataDefine.png) du champ _Hauteur_ pour proposer l'expression suivante:

```
(CASE WHEN bounds_width($atlasgeometry) > bounds_height($atlasgeometry) THEN 210 ELSE 297 END) - 20
```

Lorsque vous voulez donner un titre au-dessus de la carte au centre de la page, insérez un élément de zone de texte au-dessus de la carte. Utilisez d'abord les propriétés de l'objet de l'élément zone de texte pour définir un alignement horizontal à ![radiobuttonon](../../images/radiobuttonon.png) _Au centre_. Ensuite activez l'option du milieu supérieur à partir du _Point de référence_. Vous pouvez proposer l'expression suivante pour le champ _X_:

```
(CASE WHEN bounds_width($atlasgeometry) > bounds_height($atlasgeometry) THEN 297 ELSE 210 END) / 2
```

Pour tous les autres éléments du composeur, vous pouvez définir la position de façon similaire de sorte qu'ils soient correctement positionnés lorsque la page est automatiquement tournée en portrait ou paysage.

Les informations fournies sont tirées de l'excellent blog (en anglais et portugais) sur les options de Valeurs définies par des données [Multiple\_format\_map\_series\_using\_QGIS\_2.6\_](#id8) .

Ceci est seulement un exemple de comment vous pouvez utiliser les Valeurs de définies par des données.

## Aperçu

Une fois les paramètres de l'atlas configurés et les objets carte sélectionnés, vous pouvez créer un aperçu de toutes les pages en cliquant sur _Atlas ‣ Aperçu de l'Atlas_ puis utiliser les flèches, depuis ce même menu, pour parcourir les planches.

## Génération

La génération de l'atlas peut se faire de différentes façons. Par exemple via _Atlas ‣ Impression de l'Atlas_, vous pouvez directement l'imprimer. Vous pouvez également créer un PDF via _Atlas ‣ Exporter l'Atlas au format PDF_ et l'utilisateur devra donner un répertoire pour sauvegarder tous les fichiers (sauf si la case ![](../../images/checkbox.png) _Export d'un seul fichier (si possible)_ est cochée). Si vous souhaitez n'imprimer qu'une seule page de l'atlas, lancez l'aperçu, sélectionnez la page puis cliquez sur _Composeur → Imprimer_ (ou créez un PDF).

Pour maximiser l'espace disponible pour interagir avec une composition, vous pouvez utiliser _Vue → Masquer les panneaux_ ou appuyez sur `F10`.

Il est également possible de passer en mode plein écran pour avoir plus d'espace pour interagir en appuyant sur `F11` ou en utilisant _Vue → Basculer en mode plein écran_.

La figure suivante montre une mise en page incluant un exemple de chaque type d'élément décrit dans les paragraphes précédents.

![](../../images/print_composer_complete.png)

Avant d'imprimer une mise en page vous avez la possibilité de voir votre composition sans les boîtes de délimitation. Cela peut être activé en décochant _Vue → Afficher les zones d'emprise_ ou en appuyant sur `Ctrl+Shift+B`.

Le Composeur d'Impression vous permet de choisir plusieurs formats de sortie et il est possible de définir la résolution (qualité d'impression) et le format du papier:

- Le bouton ![](../../images/mActionFilePrint.png) _Imprimer_ vous permet d'imprimer la mise en page sur une imprimante ou dans un fichier PostScript en fonction des pilotes d'imprimante installés.
- Le bouton ![](../../images/mActionSaveMapAsImage.png) _Exporter comme image_ exporte le Composeur dans plusieurs formats d'image tels que PNG, BPM, TIF, JPG...
- ![](../../images/mActionSaveAsPDF.png) _Exporter au format PDF_ enregistre le contenu du Composeur directement dans un fichier PDF.
- Le bouton ![](../../images/mActionSaveAsSVG.png) _Exporter au format SVG_ sauve le contenu du Composeur en SVG (Scalable Vector Graphic).

Si vous devez exporter votre mise en page en tant qu’**image géoréférencée** (pour la charger ensuite dans KADAS), vous devez activer cette fonctionnalité dans l'onglet Composition. Cochez ![](../../images/checkbox.png) _Générer fichier World file_ et choisissez l'objet carte concerné. Avec cette option, _Exporter comme image_ créera également un world file.

- Actuellement le rendu SVG est très basique. Il ne s'agit pas d'un problème lié à KADAS mais à la bibliothèque Qt utilisée. Nous pouvons espérer que cela soit corrigé dans les versions futures.
- L'export de gros raster échoue parfois, même s'il semble qu'il y ait assez de ressource mémoire. Il s'agit également d'un problème lié à la gestion des raster par Qt.
