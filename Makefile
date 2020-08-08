# Makefile for Kadas Routing Plugin
SHELL := /bin/bash
DIR := ${CURDIR}

# LOCALES = space delimited list of iso codes to generate po files for
# Please dont remove en here
LOCALES = en fr de it id

#Qt .ts file updates - run to register new strings for translation 
update-translation-strings:
	# update application strings
	@echo "Checking current translation."
	@scripts/update-strings.sh $(LOCALES)

#Qt .qm file updates - run to create binary representation of translated strings for translation
compile-translation-strings:
	@#Compile qt messages binary
	@scripts/create_pro_file.sh
	@lrelease-qt4 kadasrouting.pro
	@rm kadasrouting.pro

test-translations:
	@echo
	@echo "----------------------------------------------------------------"
	@echo "Check missing translation"
	@echo "----------------------------------------------------------------"
	@python scripts/missing_translations.py `pwd` fr
	@python scripts/missing_translations.py `pwd` de
	@python scripts/missing_translations.py `pwd` it
	@python scripts/missing_translations.py `pwd` id
	@rm ./kadasrouting/i18n/*qm

##########################################################
#
# Make targets specific to Docker go below this point
#
##########################################################

docker-update-translation-strings:
	@echo "Update translation using docker"
	@docker run -t -i -v $(DIR):/home kartoza/qt-translation make update-translation-strings

docker-compile-translation-strings:
	@echo "Update translation using docker"
	@docker run -t -i -v $(DIR):/home kartoza/qt-translation make compile-translation-strings

docker-test-translation:
	@echo "Update translation using docker"
	@docker run -t -i -v $(DIR):/home kartoza/qt-translation make test-translations

push-translation:
	@echo "Push translation source to Transifex"
	@tx push -s

pull-translation:
	@echo "Pull translated string from Transifex"
	@tx pull -a -f

# Make target below are using qgis-plugin-ci for translation, but it's not finished yet

qgis-docker-ci-update-string:
	@echo "Update string for translation"
	@docker run -t -i -v $(DIR):/home -w /home etrimaille/qgis-plugin-ci push-translation ${TX_TOKEN}

shell:
	@echo "Shell"
	@docker run -t -i -v $(DIR):/home -w /home -e TX_TOKEN --entrypoint /bin/bash etrimaille/qgis-plugin-ci
