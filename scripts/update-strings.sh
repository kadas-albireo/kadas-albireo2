#!/bin/bash
LOCALES=$*

python_file=`find kadasrouting/ -regex ".*\(ui\|py\)$" -type f`

# update .ts
echo "Please provide translations by editing the translation files below:"
for LOCALE in ${LOCALES}
do
echo "kadasrouting/i18n/kadasrouting_"${LOCALE}".ts"
# Note we don't use pylupdate with qt .pro file approach as it is flakey
# about what is made available.
set -x
pylupdate4 -noobsolete ${python_file} -ts kadasrouting/i18n/kadasrouting_${LOCALE}.ts
done