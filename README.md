# KADAS Albireo

KADAS Albireo is a mapping application based on [QGIS](http://qgis.org/) and targeted at non-specialized users, providing enhanced functionalities in areas such as drawing, measuring, terrain analysis, etc.

![kadas-splash](kadas/resources/splash.png)

## Main features:

 * Streamlined user interface
 * Redlining functionality for adding geometries, pins, georeferenced pictures and other symbols
 * Measurement tools, incl. geodetic measurements.
 * Terrain analysis: slope, hillshade, viewshed, line of sight
 * Numeric inputs for all drawing operations
 * Integrated GPS geolocation support and GPX waypoint and route editor
 * Multiple map views
 * 3D globe view
 * Advanced map grids, including UTM/MGRS and guide grids
 * Tightly integrated search functionality
 * Integrated geodata catalog
 * Online-/Offline detection with online-/offline project templates
 * User friendly printing
 * KML and GPKG data export and import
 * Support for the MSS-MilX Military-Symbols library from [gs-soft](https://www.gs-soft.com/)

## Manuals

* [User manual](https://kadas-albireo.github.io/)
* [Technical manual](https://github.com/kadas-albireo/kadas-manuals/tree/master/technical/src)

## Downloads

* From the [releases](https://github.com/kadas-albireo/kadas-albireo2/releases) page.

## Quick start for Windows x64

* Download and extract the latest `kadas-portable-win64.zip` portable build from the [releases](https://github.com/kadas-albireo/kadas-albireo2/releases) page.
* Run `..\kadas\bin\kadas.exe`.
* Read the [technical manual](https://github.com/kadas-albireo/kadas-manuals/tree/master/technical/src) to learn how to configure the application.
* *Note*: To be able to use the terrain analysis functions, a heightmap needs to be defined in the project. The portable build contains a sample project which references a 1km resolution SRTM model. You can use any other model by adding the corresponding layer to the project, and marking it as heightmap from its context menu in the layer tree.

## Screenshots

### Redlining tools

Redlining sketches:

![redlining](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/redlining.png)

Georeferenced pictures:

![exif](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/exif.png)

### Multiple views
![mapwindows](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/mapwindows.png)

### Mesurement tools

Lines, circles, angles, areas:

![measure](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/measure.png)

Geodectic distance measurement:

![distance](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/distance.png)

Geodectic radius with real-time display:

![radius](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/radius.png)

### Terrain analysis

Line of Sight calculations:

![line of sight](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/los.png)

Slopes:

![slope](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/slope.png)

More tools like hillshade and viewshed analysis are available as well.

### Search

![search](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/search.png)

### GPS tools (GPX)

![gpx](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/gpx.png)

### Military symbols

![milx](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/milx.png)

### 3D visualization and interaction

![globe](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/globe.png)
![globe2](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/globe2.png)

### Printing

![print](https://github.com/kadas-albireo/kadas-albireo2/blob/gh-pages/images/print.png)


## Feedback and support:

This software is commercially supported by OPENGIS.ch, Switzerland. 
To get more information, please contact info@opengis.ch
