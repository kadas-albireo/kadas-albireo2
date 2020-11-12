# Makefile for Kadas Routing Plugin
SHELL := /bin/bash
DIR := ${CURDIR}

# LOCALES = space delimited list of iso codes to generate po files for
# Please dont remove en here
LOCALES = en fr de it

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
	# Revert the changes to qm files
	@git checkout ./kadasrouting/i18n/*qm

##########################################################
#
# Make targets specific to Docker go below this point
#
##########################################################

docker-update-translation-strings:
	@echo "Update translation using docker"
	@docker run --rm -v $(DIR):/home kartoza/qt-translation make update-translation-strings

docker-compile-translation-strings:
	@echo "Compile translation string using docker"
	@docker run --rm -v $(DIR):/home kartoza/qt-translation make compile-translation-strings

docker-test-translation:
	@echo "Test translation using docker"
	@docker run --rm -v $(DIR):/home kartoza/qt-translation make test-translations

push-translation:
	@echo "Push translation source to Transifex"
	@tx push -s

pull-translation:
	@echo "Pull translated string from Transifex"
	@tx pull -a -f

generate_vehicles_list:
	@echo "Generating list of vehicle for the manual"
	@echo "Copy the list below to the Kadas manual, kadas routing plugin section."
	@python scripts/generate_vehicles_list.py
