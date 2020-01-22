#!/bin/sh

dir=$(readlink -f "$(dirname "$(readlink -f "$0")")/..")/locale
(
cd $dir

echo "include(translations.pri)" > translations.pro
echo "" >> translations.pro
echo "SOURCES += \\" >> translations.pro
find ../kadas -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.ui' \)  -printf '%p \\\n' >> translations.pro
echo "" >> translations.pro

echo "TRANSLATIONS += \\" >> translations.pro
while read locale; do
    echo "Kadas_$locale.ts \\" >> translations.pro
done < locales.txt

lupdate-qt5 translations.pro
)
