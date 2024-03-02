find_package(Qca)
if(NOT Qca_FOUND)
  find_package(Qca-qt5 REQUIRED)
endif()
