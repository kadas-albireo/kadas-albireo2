<!-- Recovered from: share/docs/html/en/en/index.html -->
<!-- Language: en | Section: index -->

# General

## Overview

KADAS Albireo is mapping application based on the professional open source GIS software "QGIS" and targeted at non-specialized users. In cooperation with the company Ergnomen, a new user interface was developed, hiding much of the functionality aimed at advanced users, while enhancing the functionalities in area such drawing, terrain analysis, printing and interoperability.

## Terms of Use

KADAS Albireo is under the General Public License 2.0 (GPLv2).

The MSS / MilX component is the property of gs-soft AG.

The terms of use for the data are listed in the application under Help → About.

## Changelog

### Version 2.3 (August 2025)

With the release of KADAS 2.3 in Q3 2025, numerous new features, improvements, and bug fixes have been implemented.

- _New functionalities and geodata_
    - Geoservices and geodata with a time component can be played back as a chronological animation.
    - Theme comparisons (SWIPE tool) where two map views can be interactively shown and hidden using a slider.
    - Open attribute tables are saved in the project and displayed when the project is opened.
    - Support for NetCDF raster data format (used primarily for meteorology).
    - A new “Feedback” button. This button takes you directly to a form on the Geo Info Portal V, where you can provide feedback on software features and bugs.
    - Creating elevation profiles provides additional statistics, such as highest/lowest point, length, and elevation difference to the profile line.
    - Vector tiles geodata services from the MGDI are listed in the geocatalog and can be loaded.
    - Offline data now consists of the World Briefing Map.
    - MSS export is now possible as KML (cannot be further edited after KML export).
- _Improvements_
    - Plugin Manager: When KADAS is started, it checks whether more recent versions of the installed plugins are available in the repository. If so, the latest version of the plugin is automatically installed, and the user is informed with a message.
    - When querying by clicking, the display of results has been improved, especially when multiple results are returned.
    - The Bookmarks function now also saves layers in groups and subgroups.
    - The Routing plugin is now a standard plugin and is automatically installed from the repository. Additional plugins can also be defined as mandatory.
    - Restricted geoservices in the MGDI can be displayed in KADAS by authorized users.
    - The coordinate search has been improved and standardized.
    - Support for WCS geoservices now also enables analyses with high-resolution elevation models.
    - The text function in redlining contains additional settings options.
    - The display and labeling of the MGRS grid have been optimized.
    - The print templates have been updated and now include the creation date and projection system by default.
    - Analyses can be started directly by right-clicking with the mouse.
    - The extent of the GPKG export of vector data can be defined.
- _Bug fixes_
    - Crash when exporting GeoPDF has been fixed.
    - When exporting GeoPDF, all layers are considered and displayed in the correct position.
    - Crash when using the ephemeris tool has been fixed.
    - Crash when exporting geopackages has been fixed.
    - Missing translations in the languages (DE, FR, IT) have been fixed.
- _Technical adjustments_
    - KADAS 2.3 is based on QGIS version 3.44.0-Solothurn.
    - The two elevation models for Switzerland and worldwide (dtm\_analysis.tif + general\_dtm\_globe.tif) are available in COG format (Cloud Optimized GeoTIFF). This allows for faster processing and visualization.
    - The 3D viewer (Globe) had to be replaced because the technology used is end-of-life. The 3D viewer is now used by the QGIS application.
    - The MSS/MilX library used by gs-soft is now MSS-2025.
    - The Valhalla Routing Engine has been updated and now allows elevation to be included in route and distance calculations.
    - All productive plugins have been adapted for version 2.3.
    - Help has been externalized to the browser.

### Version 2.2.0 (June 2023)

- _General_:
    - Add support for loading Esri VectorTile layers
    - Add support for loading Esri MapService layers
    - Layertree: support configuring data source refresh interval for auto-refreshing layers
    - Support GeoPDF print export
    - Allow locking map scale
    - Configurable News Popup dialog
    - Improved import of 3D geometries from KML files
- _View_:
    - Allow taking snapshot pictures of 3D view
    - Improved MGRS grid labeling
- _Analysis_:
    - New min/max tool for querying lowest/highest point in selected area
    - Timezone selection in ephemeris tool
    - Correctly handle NODATA values in height profile
- _Draw_:
    - Allow undo/redo for entire drawing session
    - Allow modifying z-order of drawings
    - Allow adding pictures from URL
- _MSS_:
    - Allow undo/redo for entire drawing session
    - Allow styling leader lines (width, color)
    - Update to MSS-2024
- _Help_:
    - Allow searching through help

### Version 2.1.0 (December 2021)

- _General_:
    - Print: Properly scale symbols (MSS, pins, pictures, ...) according to print DPI
    - GPKG: Allow importing project layers
    - Layer tree: Possibility to zoom to and remove all selected layers
    - Scale based visibility also for redlining/MSS layers
    - Attribute table: various new selection and zoom tools to
- _View_:
    - New bookmarks function
- _Analysis_:
    - Viewshed: Possibility to limit observer vertical angle range
    - Height profile / Line of sight: show marker in plot when hovering over line on the map
    - New ephemeris tool
- _Draw_:
    - Pins: Add rich text editor
    - Pins: Allow interacting with tooltip content with mouse
    - Guide grid: Allow labeling only one quadrant
    - Bullseye: Quandrant labeling
    - New coordinate cross drawing item
- _MSS_:
    - Per layer symbol settings
    - Update to MSS-2022

### Version 2.0.0 (July 2020)

- Complete architectural redesign: KADAS is now a separate application, built on top of the QGIS 3.x libraries
- New map item architecture, for consistent workflow when drawing and editing redlining objects, MSS symbols, etc
- Uses the new qgz file format, avoiding the previous `<projectname>_files` folder
- Project autosave
- New plugin manager for managing external plugins directly from within KADAS
- Fullscreen mode
- New map grid implementation, supporting also UTM/MGRS grids on the main map
- KML/KMZ export by bounding box
- GPKG data export by bounding box
- Styles of redlining geometries are honoured when displayed as 2.5D or 3D objects on the Globe
- Enhanced guide grid
- Update to MSS-2021

### Version 1.2 (December 2018)

- _General_:
    - Improved KML/KMZ export functionality
    - New KML/KMZ import functionality
    - New GeoPackage export and import functionality
    - Allow adding CSV/WMS/WFS/WCS layers from ribbon GUI
    - Allow adding actions to ribbon GUI via Python API
    - Add keyboard shortcuts for many actions in ribbon GUI
    - Improved fuzzy matching when searching coordinates
- _Analysis_:
    - Show node markers in height profile
- _Draw_:
    - Support numeric input when drawing redlining objects
    - Allow setting scaling factor for annotation layers
    - Allow toggling frames of image annotations
    - Allow manipulating groups of annotation items
    - New Guide Grid functionality
    - New Bullseye functionality
- _GPS_:
    - Allow conversion between waypoints and pins
    - Allow changing color of waypoints and routes
- _MSS_:
    - Update to MSS-2019

### Version 1.1 (November 2017)

- _General_:
    - Freely positionable cursor in the search field
    - Height display in the status bar
    - Speed ​​improvements in map display
    - Attribute table for vector layers
- _Analysis_:
    - Geodetic distance and area measurement
    - Option to measure azimuth relative to the map north or geographical north
- _Draw_:
    - Optional snapping when drawing
    - Undo/redo when drawing
    - Drawings can be moved, copied, cut and pasted, individually or as a group
    - Existing geometries can be continued
    - Loading of SVG graphics (including SymTaZ graphics)
    - Loading non-georeferenced images
    - Pictures and pins are now stored in corresponding layers
- _MSS_:
    - Upgrade to MSS-2018
    - Correct size ratio of MSS symbols when printing
    - Cartouche content can be imported or exported to and from MilX or XML files
    - Numeric input of attributes when drawing MSS symbols
- _3D_:
    - Support for 3D geometries in the 3D view
- _Printing_:
    - Print templates contained in the project can be managed

### Version 1.0 (September 2016)

- Initial version
