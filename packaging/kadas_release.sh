#!/bin/bash
set -e
cd "$(dirname "$(readlink -f $0)")"

basedir=${1:-$PWD/..}
distroot=$basedir/dist
debugroot=$basedir/dist_debug
filesdir=$basedir/packaging/files

# Manually merge origins.txt
cat $distroot/origins.txt $basedir/build_mingw64/dist/usr/x86_64-w64-mingw32/sys-root/mingw/origins.txt | sort | uniq > origins.txt

mkdir -p $distroot
cp -a $basedir/build_mingw64/dist/usr/x86_64-w64-mingw32/sys-root/mingw/* $distroot/
cp -a origins.txt $distroot/

# Move debug symbols to separate folder
rm -rf $debugroot
for file in $(find $distroot -name '*.debug' \( -type l -or -type f \)); do
    dest=${file/$distroot/$debugroot}
    mkdir -p "$(dirname $dest)"
    mv "$file" "$dest"
done

# Remove devel files
find $distroot -name '*.dll.a' -delete
rm -rf $distroot/include
rm -f $distroot/share/qgis/FindQGIS.cmake

# Remove unused translations
find $distroot/share/qgis/i18n/ -type f -not -regex '^.*/qgis_\(de\|it\|fr\)\.qm$' -delete

cp -a $filesdir/* $distroot/share/kadas/

# Install python plugins
mkdir -p $distroot/share/kadas/python/plugins
for plugin in kadas-print-plugin kadas-about-plugin kadas-help-plugin kadas-gpkg-plugin; do
  version=master
  wget -O $plugin.zip https://github.com/kadas-albireo/$plugin/archive/$version.zip
  unzip $plugin.zip
  mv $plugin-$version/* $distroot/share/kadas/python/plugins/
  rm -rf $plugin-$version $plugin.zip
done
rm -f $distroot/share/kadas/python/plugins/README.md

# Remove empty directories
find $distroot -type d -empty -delete
