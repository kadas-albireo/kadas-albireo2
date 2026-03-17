<!-- Recovered from: share/docs/html/en/en/working_with_raster/raster_properties/index.html -->
<!-- Language: en | Section: working_with_raster/raster_properties -->

# Raster Properties Dialog

To view and set the properties for a raster layer, double click on the layer name in the map legend, or right click on the layer name and choose _Properties_ from the context menu. This will open the _Raster Layer Properties_ dialog.

There are several menus in the dialog:

- _General_
- _Style_
- _Transparency_
- _Pyramids_
- _Histogram_
- _Metadata_

![](../../../../images/rasterPropertiesDialog.png)

## General Menu

### Layer Info

The _General_ menu displays basic information about the selected raster, including the layer source path, the display name in the legend (which can be modified), and the number of columns, rows and no-data values of the raster.

### Coordinate reference system

Here, you find the coordinate reference system (CRS) information printed as a PROJ.4 string. If this setting is not correct, it can be modified by clicking the **[Specify]** button.

### Scale Dependent visibility

Additionally scale-dependent visibility can be set in this tab. You will need to check the checkbox and set an appropriate scale where your data will be displayed in the map canvas.

At the bottom, you can see a thumbnail of the layer, its legend symbol, and the palette.

## Style Menu

### Band rendering

KADAS offers four different _Render types_. The renderer chosen is dependent on the data type.

1. Multiband color - if the file comes as a multiband with several bands (e.g., used with a satellite image with several bands)
2. Paletted - if a single band file comes with an indexed palette (e.g., used with a digital topographic map)
3. Singleband gray - (one band of) the image will be rendered as gray; KADAS will choose this renderer if the file has neither multibands nor an indexed palette nor a continous palette (e.g., used with a shaded relief map)
4. Singleband pseudocolor - this renderer is possible for files with a continuous palette, or color map (e.g., used with an elevation map)

**Multiband color**

With the multiband color renderer, three selected bands from the image will be rendered, each band representing the red, green or blue component that will be used to create a color image. You can choose several _Contrast enhancement_ methods: ‘No enhancement’, ‘Stretch to MinMax’, ‘Stretch and clip to MinMax’ and ‘Clip to min max’.

![](../../../../images/rasterMultibandColor.png)

This selection offers you a wide range of options to modify the appearance of your raster layer. First of all, you have to get the data range from your image. This can be done by choosing the _Extent_ and pressing **[Load]**. KADAS can ![radiobuttonon](../../../../images/radiobuttonon.png) _Estimate (faster)_ the _Min_ and _Max_ values of the bands or use the ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Actual (slower)_ _Accuracy_.

Now you can scale the colors with the help of the _Load min/max values_ section. A lot of images have a few very low and high data. These outliers can be eliminated using the ![radiobuttonon](../../../../images/radiobuttonon.png) _Cumulative count cut_ setting. The standard data range is set from 2% to 98% of the data values and can be adapted manually. With this setting, the gray character of the image can disappear. With the scaling option ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min/max_, KADAS creates a color table with all of the data included in the original image (e.g., KADAS creates a color table with 256 values, given the fact that you have 8 bit bands). You can also calculate your color table using the ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Mean +/- standard deviation x_ ![](../../../../images/selectnumber.png). Then, only the values within the standard deviation or within multiple standard deviations are considered for the color table. This is useful when you have one or two cells with abnormally high values in a raster grid that are having a negative impact on the rendering of the raster.

All calculations can also be made for the ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Current_ extent.

**Viewing a Single Band of a Multiband Raster**

If you want to view a single band of a multiband image (for example, Red), you might think you would set the Green and Blue bands to “Not Set”. But this is not the correct way. To display the Red band, set the image type to ‘Singleband gray’, then select Red as the band to use for Gray.

**Paletted**

This is the standard render option for singleband files that already include a color table, where each pixel value is assigned to a certain color. In that case, the palette is rendered automatically. If you want to change colors assigned to certain values, just double-click on the color and the _Select color_ dialog appears. Also, in KADAS 2.2. it’s now possible to assign a label to the color values. The label appears in the legend of the raster layer then.

![](../../../../images/rasterPaletted.png)

**Contrast enhancement**

When adding GRASS rasters, the option _Contrast enhancement_ will always be set automatically to _stretch to min max_, regardless of if this is set to another value in the KADAS general options.

**Singleband gray**

This renderer allows you to render a single band layer with a _Color gradient_: ‘Black to white’ or ‘White to black’. You can define a _Min_ and a _Max_ value by choosing the _Extent_ first and then pressing **[Load]**. KADAS can ![radiobuttonon](../../../../images/radiobuttonon.png) _Estimate (faster)_ the _Min_ and _Max_ values of the bands or use the ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Actual (slower)_ _Accuracy_.

![](../../../../images/rasterSingleBandGray.png)

With the _Load min/max values_ section, scaling of the color table is possible. Outliers can be eliminated using the ![radiobuttonon](../../../../images/radiobuttonon.png) _Cumulative count cut_ setting. The standard data range is set from 2% to 98% of the data values and can be adapted manually. With this setting, the gray character of the image can disappear. Further settings can be made with ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min/max_ and ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Mean +/- standard deviation x_ ![](../../../../images/selectnumber.png). While the first one creates a color table with all of the data included in the original image, the second creates a color table that only considers values within the standard deviation or within multiple standard deviations. This is useful when you have one or two cells with abnormally high values in a raster grid that are having a negative impact on the rendering of the raster.

**Singleband pseudocolor**

This is a render option for single-band files, including a continous palette. You can also create individual color maps for the single bands here.

![](../../../../images/rasterSingleBandPseudocolor.png)

Three types of color interpolation are available:

1. Discrete
2. Linear
3. Exact

In the left block, the button ![](../../../../images/mActionSignPlus.png) _Add values manually_ adds a value to the individual color table. The button ![](../../../../images/mActionSignMinus.png) _Remove selected row_ deletes a value from the individual color table, and the ![](../../../../images/mActionArrowDown.png) _Sort colormap items_ button sorts the color table according to the pixel values in the value column. Double clicking on the value column lets you insert a specific value. Double clicking on the color column opens the dialog _Change color_, where you can select a color to apply on that value. Further, you can also add labels for each color, but this value won’t be displayed when you use the identify feature tool. You can also click on the button ![](../../../../images/mActionDraw.png) _Load color map from band_, which tries to load the table from the band (if it has any). And you can use the buttons ![](../../../../images/mActionFileOpen.png) _Load color map from file_ or ![](../../../../images/mActionFileSaveAs.png) _Export color map to file_ to load an existing color table or to save the defined color table for other sessions.

In the right block, _Generate new color map_ allows you to create newly categorized color maps. For the _Classification mode_ ![](../../../../images/selectstring.png) ‘Equal interval’, you only need to select the _number of classes_ ![](../../../../images/selectnumber.png) and press the button _Classify_. You can invert the colors of the color map by clicking the ![](../../../../images/checkbox.png) _Invert_ checkbox. In the case of the _Mode_ ![](../../../../images/selectstring.png) ‘Continous’, KADAS creates classes automatically depending on the _Min_ and _Max_. Defining _Min/Max_ values can be done with the help of the _Load min/max values_ section. A lot of images have a few very low and high data. These outliers can be eliminated using the ![radiobuttonon](../../../../images/radiobuttonon.png) _Cumulative count cut_ setting. The standard data range is set from 2% to 98% of the data values and can be adapted manually. With this setting, the gray character of the image can disappear. With the scaling option ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Min/max_, KADAS creates a color table with all of the data included in the original image (e.g., KADAS creates a color table with 256 values, given the fact that you have 8 bit bands). You can also calculate your color table using the ![radiobuttonoff](../../../../images/radiobuttonoff.png) _Mean +/- standard deviation x_ ![](../../../../images/selectnumber.png). Then, only the values within the standard deviation or within multiple standard deviations are considered for the color table.

### Color rendering

For every _Band rendering_, a _Color rendering_ is possible.

You can also achieve special rendering effects for your raster file(s) using one of the blending modes (see [_The Vector Properties Dialog_](../working_with_vector/vector_properties.html#vector-properties-dialog)).

Further settings can be made in modifiying the _Brightness_, the _Saturation_ and the _Contrast_. You can also use a _Grayscale_ option, where you can choose between ‘By lightness’, ‘By luminosity’ and ‘By average’. For one hue in the color table, you can modify the ‘Strength’.

### Resampling

The _Resampling_ option makes its appearance when you zoom in and out of an image. Resampling modes can optimize the appearance of the map. They calculate a new gray value matrix through a geometric transformation.

![](../../../../images/rasterRenderAndRessampling.png)

When applying the ‘Nearest neighbour’ method, the map can have a pixelated structure when zooming in. This appearance can be improved by using the ‘Bilinear’ or ‘Cubic’ method, which cause sharp features to be blurred. The effect is a smoother image. This method can be applied, for instance, to digital topographic raster maps.

## Transparency Menu

KADAS has the ability to display each raster layer at a different transparency level. Use the transparency slider ![slider](../../../../images/slider.png) to indicate to what extent the underlying layers (if any) should be visible though the current raster layer. This is very useful if you like to overlay more than one raster layer (e.g., a shaded relief map overlayed by a classified raster map). This will make the look of the map more three dimensional.

Additionally, you can enter a raster value that should be treated as _NODATA_ in the _Additional no data value_ menu.

An even more flexible way to customize the transparency can be done in the _Custom transparency options_ section. The transparency of every pixel can be set here.

As an example, we want to set the water of our example raster file `landcover.tif` to a transparency of 20%. The following steps are neccessary:

1. Load the raster file `landcover.tif`.
2. Open the _Properties_ dialog by double-clicking on the raster name in the legend, or by right-clicking and choosing _Properties_ from the pop-up menu.
3. Select the _Transparency_ menu.
4. From the _Transparency band_ menu, choose ‘None’.
5. Click the ![](../../../../images/mActionSignPlus.png) _Add values manually_ button. A new row will appear in the pixel list.
6. Enter the raster value in the ‘From’ and ‘To’ column (we use 0 here), and adjust the transparency to 20%.
7. Press the **[Apply]** button and have a look at the map.

You can repeat steps 5 and 6 to adjust more values with custom transparency.

As you can see, it is quite easy to set custom transparency, but it can be quite a lot of work. Therefore, you can use the button ![](../../../../images/mActionFileSave.png) _Export to file_ to save your transparency list to a file. The button ![](../../../../images/mActionFileOpen.png) _Import from file_ loads your transparency settings and applies them to the current raster layer.

## Metadata Menu

The _Metadata_ menu displays a wealth of information about the raster layer, including statistics about each band in the current raster layer. From this menu, entries may be made for the _Description_, _Attribution_, _MetadataUrl_ and _Properties_. In _Properties_, statistics are gathered on a ‘need to know’ basis, so it may well be that a given layer’s statistics have not yet been collected.

![](../../../../images/rasterMetadata.png)
