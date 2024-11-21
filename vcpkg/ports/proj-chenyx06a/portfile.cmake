vcpkg_from_github(
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  OSGeo/proj-datumgrid
  REF
  ${VERSION}
  SHA512
  9582fd1cb9f32fdbef0de0a01133fbdfc5cb0db167d214abef3329ebf6853ee499485bc5f7a0812f8ab6a109118375757f349bc4a4e2bb5d4b5b413ccfcb57d1
  HEAD_REF
  master)

file(COPY "${SOURCE_PATH}/europe/CHENyx06a.gsb"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/proj/")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/filelist.csv")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
