#!/usr/bin/env bash
###########################################################################
#    sipify_all.sh
#    ---------------------
#    Date                 : 25.03.2017
#    Copyright            : (C) 2017 by Denis Rouzaud
#    Email                : denis.rouzaud@gmail.com
###########################################################################
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
###########################################################################
set -e

# TEMPLATE_DOC=""
# while :; do
#     case $1 in
#         -t|--template-doc) TEMPLATE_DOC="-template-doc"
#         ;;
#         *) break
#     esac
#     shift
# done

DIR=$(git rev-parse --show-toplevel)

pushd ${DIR} > /dev/null

count=0

modules=(core gui analysis)
for module in "${modules[@]}"; do

  # clean auto_additions and auto_generated folders
  rm -rf python/kadas${module}/auto_additions/*.py
  rm -rf python/kadas${module}/auto_generated/*.py
  mkdir -p python/kadas${module}/auto_additions/
  # put back __init__.py
  echo '"""
This folder is completed using sipify.pl script
It is not aimed to be manually edited
"""' > python/kadas${module}/auto_additions/__init__.py

  # Generate {module}_auto.sip
  find kadas/${module}/ -name '*.h' | sed -E 's|^kadas/'${module}'/(.*)\.h$|%Include auto_generated/\1.sip|g' > python/kadas${module}/kadas${module}_auto.sip

  while read -r sipfile; do
      echo "$sipfile.in"
      header=$(sed -E 's@(.*)\.sip@kadas/\1.h@; s@^.*/auto_generated/@kadas/'${module}'/@' <<< $sipfile)
      pyfile=$(sed -E 's@([^\/]+\/)*([^\/]+)\.sip@\2.py@;' <<< $sipfile)
      if [ ! -f $header ]; then
        echo "*** Missing header: $header for sipfile $sipfile"
      else
        path=$(sed -r 's@/[^/]+$@@' <<< $sipfile)
        mkdir -p python/$path
        ./scripts/sipify.pl -s python/$sipfile.in -p python/kadas${module}/auto_additions/${pyfile} $header &
      fi
      count=$((count+1))
  done < <( sed -n -r "s@^%Include auto_generated/(.*\.sip)@kadas${module}/auto_generated/\1@p" python/kadas${module}/kadas${module}_auto.sip )
done
wait # wait for sipify processes to finish

echo " => $count files sipified! 🍺"

popd > /dev/null
