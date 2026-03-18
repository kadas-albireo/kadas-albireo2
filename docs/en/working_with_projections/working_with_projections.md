<!-- Recovered from: share/docs/html/en/en/working_with_projections/working_with_projections/index.html -->
<!-- Language: en | Section: working_with_projections/working_with_projections -->

# Working with Projections

KADAS allows users to define a global and project-wide CRS (coordinate reference system) for layers without a pre-defined CRS. It also allows the user to define custom coordinate reference systems and supports on-the-fly (OTF) projection of vector and raster layers. All of these features allow the user to display layers with different CRSs and have them overlay properly.

## Overview of Projection Support

KADAS has support for approximately 2,700 known CRSs. Definitions for each CRS are stored in a SQLite database that is installed with KADAS. Normally, you do not need to manipulate the database directly. In fact, doing so may cause projection support to fail. Custom CRSs are stored in a user database.

The CRSs available in KADAS are based on those defined by the European Petroleum Search Group (EPSG) and the Institut Geographique National de France (IGNF) and are largely abstracted from the spatial reference tables used in GDAL. EPSG identifiers are present in the database and can be used to specify a CRS in KADAS.

In order to use OTF projection, either your data must contain information about its coordinate reference system or you will need to define a global, layer or project-wide CRS. For PostGIS layers, KADAS uses the spatial reference identifier that was specified when the layer was created. For data supported by OGR, KADAS relies on the presence of a recognized means of specifying the CRS. In the case of shapefiles, this means a file containing the well-known text (WKT) specification of the CRS. This projection file has the same base name as the shapefile and a `.prj` extension. For example, a shapefile named `alaska.shp` would have a corresponding projection file named `alaska.prj`.

The _CRS_ tab contains the following important components:

1. **Filter** — If you know the EPSG code, the identifier, or the name for a coordinate reference system, you can use the search feature to find it. Enter the EPSG code, the identifier or the name.
2. **Recently used coordinate reference systems** — If you have certain CRSs that you frequently use in your everyday GIS work, these will be displayed in this list. Click on one of these items to select the associated CRS.
3. **Coordinate reference systems of the world** — This is a list of all CRSs supported by KADAS, including Geographic, Projected and Custom coordinate reference systems. To define a CRS, select it from the list by expanding the appropriate node and selecting the CRS. The active CRS is preselected.
4. **PROJ.4 text** — This is the CRS string used by the PROJ.4 projection engine. This text is read-only and provided for informational purposes.

## Default datum transformations

OTF depends on being able to transform data into a ‘default CRS’, and KADAS uses WGS84. For some CRS there are a number of transforms available. KADAS allows you to define the transformation used otherwise KADAS uses a default transformation.

KADAS asks which transformation to use by opening a dialogue box displaying PROJ.4 text describing the source and destination transforms. Further information may be found by hovering over a transform. User defaults can be saved by selecting ![radiobuttonon](../../images/radiobuttonon.png) _Remember selection_.
