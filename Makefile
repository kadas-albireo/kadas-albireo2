# Makefile for Kadas Routing Plugin
SHELL := /bin/bash
DIR := ${CURDIR}

# LOCALES = space delimited list of iso codes to generate po files for
# Please dont remove en here
LOCALES = en fr de it

#Qt .ts file updates - run to register new strings for translation in safe_qgis
update-translation-strings:
	# update application strings
	@echo "Checking current translation."
	@scripts/update-strings.sh $(LOCALES)

#Qt .qm file updates - run to create binary representation of translated strings for translation in safe_qgis
compile-translation-strings:
	@#Compile qt messages binary
	@scripts/create_pro_file.sh
	@lrelease-qt4 inasafe.pro
	@rm inasafe.pro

test-translations:
	@echo
	@echo "----------------------------------------------------------------"
	@echo "Missing translations - for more info run: make translation-stats"
	@echo "----------------------------------------------------------------"
	@python scripts/missing_translations.py `pwd` id
	@python scripts/missing_translations.py `pwd` fr
	@python scripts/missing_translations.py `pwd` af
	@python scripts/missing_translations.py `pwd` es_ES
	@python scripts/missing_translations.py `pwd` vi
	@python scripts/missing_translations.py `pwd` pt

# Run flake8 style checking
flake8:
	@echo
	@echo "-----------"
	@echo "Flake8 issues"
	@echo "-----------"
	@python3 -m flake8 --version
	@python3 -m flake8

pylint:
	@echo
	@echo "-----------------"
	@echo "Pylint violations"
	@echo "-----------------"
	@pylint --version
	@pylint --reports=n --rcfile=pylintrc safe realtime || true

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

qgis-docker-ci-update-string:
	@echo "Update string for translation"
	@docker run -t -i -v $(DIR):/home -w /home etrimaille/qgis-plugin-ci push-translation ${TX_TOKEN}

shell:
	@echo "Shell"
	@docker run -t -i -v $(DIR):/home -w /home -e TX_TOKEN --entrypoint /bin/bash etrimaille/qgis-plugin-ci
