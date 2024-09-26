#!/bin/bash

# Helper script to copy 3D classes which live in QGIS app to Kadas.
# Run this from within [QGIS]/src/3d

# List of class names and file names
class_names=(
#  "Qgs3DMapCanvasWidget"
  "Qgs3DMapConfigWidget"
  "QgsMesh3DSymbolWidget"
  "QgsLightsWidget"
  "QgsAmbientOcclusionSettingsWidget"
  "QgsPhongMaterialWidget"
  "QgsSkyboxRenderingSettingsWidget"
  "QgsShadowRenderingSettingsWidget"
  "Qgs3DNavigationWidget")
file_names=(
#  "qgs3dmapcanvaswidget" 
	"qgs3dmapconfigwidget"
	"qgsmesh3dsymbolwidget"
	"qgslightswidget" 
	"qgsambientocclusionsettingswidget"
	"qgsphongmaterialwidget" 
	"qgsskyboxrenderingsettingswidget"
	"qgsshadowrenderingsettingswidget"
	"qgs3dnavigationwidget")

# Define new prefix and the output directory
prefix="Kadas"
output_dir="kadas"

# Create output directory if it doesn't exist
mkdir -p "$output_dir"

# Outer loop: iterate over all the files (both .h and .cpp)
for old_file in "${file_names[@]}"; do
  for ext in "h" "cpp"; do
    old_filename="${old_file}.${ext}"
    new_filename="${prefix,,}${old_file#qgs}.${ext}"

    # Check if the file exists before proceeding
    if [[ -f "$old_filename" ]]; then
      # Copy the file to the new folder with the new name
      cp "$old_filename" "$output_dir/$new_filename"
      
      # Inner loop: iterate over all class names and patterns
      for i in "${!class_names[@]}"; do
        old_class="${class_names[i]}"
        new_class="${prefix}${old_class#Qgs}"
        old_file_name="${file_names[i]}"
        new_file_name="${prefix,,}${old_file_name#qgs}"

        # Perform the class name replacement (QgsAbcDef -> KadasAbcDef)
        sed -i "s/${old_class}/${new_class}/g" "$output_dir/$new_filename"

        # Replace all references to the header file (qgsabcdef.h -> kadasabcdef.h)
        sed -i "s/${old_file_name}.h/${new_file_name}.h/g" "$output_dir/$new_filename"

        # A couple of additional replacements
        sed -i "s/Ui::${new_class}/Ui_${new_class}/g" "$output_dir/$new_filename"
        sed -i "s/APP_EXPORT //g" "$output_dir/$new_filename"
        sed -i "s/#include \"qgis_app.h\"//g" "$output_dir/$new_filename"
        sed -i "s/#include \"qgisapp.h\"//g" "$output_dir/$new_filename"

        # Handle replacement of all-caps pattern QGSABCDEF_H with KADASABCDEF_H
        old_caps_define="$(echo "$old_class" | tr '[:lower:]' '[:upper:]')_H"
        new_caps_define="$(echo "$new_class" | tr '[:lower:]' '[:upper:]')_H"
        sed -i "s/${old_caps_define}/${new_caps_define}/g" "$output_dir/$new_filename"
      done

      echo "Processed and renamed $old_filename to $new_filename"
    else
      echo "File $old_filename not found, skipping..."
    fi
  done
done

echo "All files processed and saved in '$output_dir' directory."

