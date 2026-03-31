<!-- Recovered from: docs_old/html/fr/fr/map/index.html -->
<!-- Language: fr | Section: map -->

# Carte

## Projets

Les cartes peuvent être chargées et enregistrées comme projets. On utilise le format de projet QGIS, qui se termine par _\*.qgz_. Les projets sont créés comme propositions. Au lancement de l’application, un projet est automatiquement créé comme proposition en ligne ou hors ligne, indépendamment du fait que l’ordinateur soit relié à Internet.

## Créer et sauvegarder des cartes

Les fonctions **Nouveau**, **Ouvrir**, **Enregistrer** et **Enregistrer sous** permettent de créer de nouveaux projets (à partir d’une proposition), d’ouvrir des projets existants et d’enregistrer des projets.

## Ouvrir des projets

Les cartes sauvegardées (projets) peuvent être chargées avec la fonction **Ouvrir**.

## Imprimer

La carte actuelle peut être imprimée avec la fonction **Imprimer** ou sauvegardée dans un fichier.

L’impression se base sur des modèles. Par défaut, des modèles sont proposés en format A0-A6, aussi bien en portrait qu’en paysage, ainsi qu’un modèle Custom.

Une fois le modèle choisi, un rectangle bleu semi-transparent apparaît dans la fenêtre principale de la carte, qui correspond à l’extrait à imprimer. Pour les modèles avec un format de papier fixe, ce rectangle peut être déplacé sur la carte principale afin d’adapter la zone à imprimer. La taille de l’extrait dépend du format du papier ainsi que des mesures indiquées dans la boîte de dialogue d’impression. Avec le modèle _Custom_, l’extrait est défini numériquement avec l’échelle dans la boîte de dialogue d’impression, et le format de papier en résultant est calculé de manière dynamique selon ces indications.

Lors de l'impression d'éléments supplémentaires, la grille de coordonnées, la cartouche de cartes, la légende et la barre d'échelle peuvent être affichées ou masquées comme vous le souhaitez. La position de ces éléments est définie dans le modèle.

![](../media/image12.png)

### Boîte de dialogue d’impression

- **Modèle** : Choix du modèle d’impression.Un aperçu de l’impression s’affiche.
- **Titre** : Titre qui apparaît sur le document imprimé.
- **Échelle** : échelle d’impression
- **Grille** : Si la section Grille est ouverte, une grille quadrillée apparaîtra en fond sur le document imprimé.
- **System de coordonnées** : choix du système de coordination de la grille
- **Intervalle X** : écart des lignes de la grille dans le sens X
- **Intervalle Y** : écart des lignes de la grille dans le sens Y
- **Afficher les coordonnées** : activation/désactivation des annotations sur la grille quadrillée
- **Cartouche** : activation/désactivation du cartouche
- **Éditer cartouche** : configuration du cartouche
- **Barre d'échelle** : activation/désactivation de l’indication de l’échelle
- **Légende** : Activer/désactiver la légende, via _Configurer_ on peut sélectionner séparément les couches qui apparaissent dans la légende
- **Format de fichier** : choix du format pour la fonction d’exportation du fichier

### Modèles d'impression

Les modèles d'impression contenus dans le projet peuvent être gérés dans la boîte de dialogue Imprimer composition, qui peut être ouverte à l'aide du bouton situé à droite de la sélection du modèle d'impression. Là, des modèles individuels peuvent être importés, exportés et supprimés du projet.

![](../media/image12.1.png)

### Cartouche de la carte

Cette boîte de dialogue montre le contenu de la **cartouche**. Dans les champs de saisie, la fonction du texte est enregistrée. Si la case checkbox **Exercise** est cochée, les données de test du cartouche s’affichent.

De plus, le contenu du cartouche peut être importé et exporté en tant que fichier XML distinct dans la boîte de dialogue du cartouche.

### Document imprimé

- **Exporter** : un fichier est créé au format choisi.
- **Imprimer** : dans la boîte de dialogue d’impression, on peut sélectionner une imprimante installée et lancer l’impression.
- **Fermer** : la boîte de dialogue d’impression est fermée.
- **Avancé** : utilisation des fonctions de layout avancées

## Copier carte / Enregistrer carte

Ces fonctions permettent d’enregistrer l’extrait de carte visible dans la fenêtre principale dans le presse-papier ou dans un fichier. C’est toujours le contenu exact de la fenêtre de carte qui est enregistré.

La fonction **Enregistrer carte** ouvre une boîte de dialogue dans laquelle le chemin de sortie et le type d'image (PNG, JPG, etc.) peuvent être sélectionnés. Un fichier "world" (avec extension PNGW, JPGW, etc.) enregistré dans le même dossier géoréfère l'image.

## Exportation et importation KML/KMZ

Le contenu de la carte peut être exporté comme KML ou KMZ. Les images, les couches raster que les symboles MSS sont seulement exportés au format KMZ.

Les fichiers KML/KMZ peuvent également être importés dans KADAS.

_Remarque_: KMZ et KML sont des formats d’exportation qui génèrent souvent des pertes de données et qui ne conviennent donc pas pour les échanges entre utilisateurs de KADAS. Pour de tels échanges, il faut utiliser le format de projet natif _\*.qgs_.

## Exportation et importation GPKG

Le GeoPackage (GPKG) KADAS est un format de fichier basé sur SQLite, qui regroupe à la fois les géodonnées locales contenues dans un projet et le projet lui-même dans un seul fichier, et offre ainsi une possibilité pratique d'échanger des projets et données.

Lors de l'exportation du projet au GPKG, vous pouvez choisir quelles géodonnées doivent être écrites dans le GeoPackage, le projet sauvegardé dans le GPKG chargera alors les données directement depuis le GPKG. Il est également possible de décider si un GPKG existant doit être mis à jour ou complètement remplacé. Dans le premier cas, les données existantes restent dans le GPKG, même si elles ne sont pas référencées par le projet.

Lors de l'importation, un projet KADAS est recherché et ouvert dans le GPKG, et les données géospatiales référencées depuis le GPKG seront lus à partir du même.
