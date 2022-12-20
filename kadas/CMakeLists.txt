ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(analysis)
ADD_SUBDIRECTORY(gui)
ADD_SUBDIRECTORY(app)

INSTALL (FILES resources/timezone21.bin DESTINATION share/kadas/resources)
IF(WITH_GLOBE)
  INSTALL (DIRECTORY resources/globe DESTINATION share/kadas/resources)
ENDIF(WITH_GLOBE)
