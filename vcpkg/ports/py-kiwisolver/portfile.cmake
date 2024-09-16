vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO nucleic/kiwi
    REF ${VERSION}
    SHA512 889e106c27915cb773bc13969605812c1ca01a139e992d2b3517eb20989ae41392bfdcdd63184a7777b13eff5109d167869087ea09149b1527e56a3455213b14
    HEAD_REF master
)


file(WRITE "${SOURCE_PATH}/py/src/version.h" 
"\n\
/* ----------------------------------------------------------------------------\n\
| Copyright (c) 2013-2021, Nucleic Development Team.\n\
|\n\
| Distributed under the terms of the Modified BSD License.\n\
|\n\
| The full license is in the file LICENSE, distributed with this software.\n\
| ---------------------------------------------------------------------------*/\n\
\n\
#pragma once\n\
\n\
#define PY_KIWI_VERSION \"${VERSION}\"\n\
\n"
)

vcpkg_python_build_and_install_wheel(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_python_test_import(MODULE "kiwisolver")

