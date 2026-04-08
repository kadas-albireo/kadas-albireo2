<!-- Recovered from: docs_old/html/en/en/map/index.html -->
<!-- Language: en | Section: map -->

# Map

## Projects

Maps can be loaded and saved as projects. The QGIS project format is used, ending with _\*.qgz_. Projects are created from templates. When starting the application, a project is automatically created from an online or offline template, depending on whether the computer is connected to the network.

## Creating and saving maps

The functions **New**, **Open**, **Save** and **Save as** allow creating new projects (from a template), opening existing projects and saving open projects.

## Open projects

Saved maps (projects) can be opened with the **Open** functionality.

## Printing

The **Print** functionality allows printing the current map or exporting it to a file.

Printing is based on layouts. By default, layouts for A0 through A6 are available in both landscape and portrait modes, as well as a Custom layout.

When the layout is selected, the main map window displays a blue semi-transparent rectangle that corresponds to the section to be printed. For fixed-size layouts, this rectangle can be moved in the main map to adjust the print area. The size of the rectangle is derived from the paper size and the scale specified in the print dialog. In the case of the _Custom_ layout, the section is numerically defined along with the scale in the print dialog, and the resulting paper size is dynamically calculated according to this information.

When printing additional elements, coordinate grid, card cartridge, legend, and scale bar can be shown or hidden as desired. The position of these elements is defined in the layout.

![](../media/image12.png)

### Printing dialog

- **Layout**: Choice of the printing template. A print-preview is displayed.
- **Title**: The title to be displayed.
- **Scale**: The scale of the printed map
- **Grid**: If the Grid secion is expanded, a grid is overlayed on the map.
- **Coordinate System**: Choice of the coordinate system to use for the grid
- **Interval X**: Grid line spacing in X direction
- **Interval Y**: Grid line spacing in Y direction
- **Coordinate labels**: Toggling of the coordinate labels
- **Map cartouche**: Toggling of the map cartouche
- **Edit map cartouche**: Setup of the map cartouche
- **Scalebar**: Toggling of the scale bar
- **Legend**: Enable or disable the map legend, via the _Configure_ button it is furthermore possible to separately choose which layers are shown in the legend
- **File format**: Choice of the file format when exporting to a file

### Print Layouts

The print layouts contained in the project can be managed in the _Print Layout Manager_ dialog, which can be opened by pressing the button to the right of the print layout selection. There, individual layouts can be imported, exported and removed from the project.

![](../media/image12.1.png)

### Map Cartouche

This dialog allows the user to set up the map **cartouche**. The meaning of each field is show as a place-holder text. If the **Exercise** checkbox is active, fields for describing an exercise will be activated.

Furthermore, the map cartouche content can be imported and exported as a separate XML file in the cartouche dialog.

### Print output

- **Export**: A file-based export will be performed, using the chosen file format.
- **Print**: A printer selection dialog will be shown and the printing job will be submitted to the selected printer.
- **Close**: Will close the printing dialog.
- **Advanced**: Will open the advanced print layout editor

## Copy map / Save map

These functions allow the user to save the map extent visible in the main map frame to the clipboard or to a file. The contents of the map is saved exactely as it is rendered in the application.

The function **Save map** opens a file dialog where the output path and image type (PNG, JPG, etc.) can be selected. A world file (with extension PNGW, JPGW, etc.) saved in the same folder georeferences the image.

## KML / KMZ export and import

The map content can be exported as KML or KMZ. Images, raster layers as well as MSS symbols are only exported in the KMZ format.

KMZ/KML files can also be imported into KADAS.

_Note_: KMZ and KML are lossy export formats, and therefore not suitable for exchange between KADAS users. The native _\*.qgs_ project format should be used for this.

## GPKG Export and Import

The KADAS GeoPackage (GPKG) is a SQLite based file format, which packs both local geodata contained in a project and the project itself into one file, and thus offers a practical possibility to exchange projects and data.

When performing a GPKG Project Export, you can choose which geodata should be written to the GeoPackage, the project saved in the GPKG will then reference the data directly from the GPKG. In addition, it can be decided whether an existing GPKG should be updated or completely replaced. In the first case, existing data will be preserved in the GPKG, even if not referenced by the project.

During import, a KADAS project is searched and opened from the GPKG, and geodata referenced from the GPKG are loaded directly from it.
