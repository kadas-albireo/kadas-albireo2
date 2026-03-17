<!-- Recovered from: share/docs/html/en/en/working_with_raster/supported_data/index.html -->
<!-- Language: en | Section: working_with_raster/supported_data -->

# Working with Raster Data

This section describes how to visualize and set raster layer properties. KADAS uses the GDAL library to read and write raster data formats, including ArcInfo Binary Grid, ArcInfo ASCII Grid, GeoTIFF, ERDAS IMAGINE, and many more. GRASS raster support is supplied by a native KADAS data provider plugin. The raster data can also be loaded in read mode from zip and gzip archives into KADAS.

As of the date of this document, more than 100 raster formats are supported by the GDAL library. A complete list is available at <http://www.gdal.org/formats_list.html>.

Note

Not all of the listed formats may work in KADAS for various reasons. For example, some require external commercial libraries, or the GDAL installation of your OS may not have been built to support the format you want to use. Only those formats that have been well tested will appear in the list of file types when loading a raster into KADAS. Other untested formats can be loaded by selecting the `[GDAL] All files (*)` filter.

## What is raster data?

Raster data in GIS are matrices of discrete cells that represent features on, above or below the earth’s surface. Each cell in the raster grid is the same size, and cells are usually rectangular (in KADAS they will always be rectangular). Typical raster datasets include remote sensing data, such as aerial photography, or satellite imagery and modelled data, such as an elevation matrix.

Unlike vector data, raster data typically do not have an associated database record for each cell. They are geocoded by pixel resolution and the x/y coordinate of a corner pixel of the raster layer. This allows KADAS to position the data correctly in the map canvas.

KADAS makes use of georeference information inside the raster layer (e.g., GeoTiff) or in an appropriate world file to properly display the data.
