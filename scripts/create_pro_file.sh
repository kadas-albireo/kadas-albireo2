#!/bin/bash

PYFILES=`find kadasrouting -name "**.py**" | grep -v "pyc$" | grep -v test`
UI_FILES=`find kadasrouting -name "**.ui**"`

PRO_FILE=kadasrouting.pro

echo "SOURCES = \\" > ${PRO_FILE}

# First add the SAFE files to the pro file
for FILE in ${PYFILES}
do
  echo "    ${FILE} \\"  >> ${PRO_FILE}
done

echo "
FORMS = \\" >> ${PRO_FILE}

LAST_FILE=""
for FILE in ${UI_FILES}
do
        if [ ! -z ${LAST_FILE} ]
        then
                echo "    ${LAST_FILE} \\" >> ${PRO_FILE}
        fi
        LAST_FILE=${FILE}
done
if [ ! -z ${LAST_FILE} ]
then
        echo "    ${LAST_FILE}" >> ${PRO_FILE}
fi

# Finally define which languages we are translating for

echo "
TRANSLATIONS = kadasrouting/i18n/kadasrouting_id.ts \\
               kadasrouting/i18n/kadasrouting_fr.ts \\
               kadasrouting/i18n/kadasrouting_de.ts \\
               kadasrouting/i18n/kadasrouting_it.ts \\
               kadasrouting/i18n/kadasrouting_id.ts" >> ${PRO_FILE}
