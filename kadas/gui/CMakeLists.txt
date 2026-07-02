set(kadas_gui_SRC
    kadasattributetabledialog.cpp
    kadasbottombar.cpp
    kadassidepanel.cpp
    kadassidepanelhost.cpp
    kadascatalogbrowser.cpp
    kadascatalogprovider.cpp
    kadasclipboard.cpp
    kadasclipboardutils.h
    kadascoordinatedisplayer.cpp
    kadascoordinateinput.cpp
    kadascrsselection.cpp
    kadasfeaturepicker.cpp
    kadasfloatinginputwidget.cpp
    kadasheightprofiledialog.cpp
    kadasattributetypes.cpp
    kadaslayerpropertiesdialog.cpp
    kadaslayerselectionwidget.cpp
    kadasmapitemtooltip.cpp
    kadasmapwidget.cpp
    kadasprojectmigration.cpp
    kadasprojecttemplateselectiondialog.cpp
    kadasribbonbutton.cpp
    kadasrichtexteditor.cpp
    kadastextbrowser.cpp
    annotationitems/kadasannotationcontrollerregistry.cpp
    annotationitems/kadasannotationitemcontroller.cpp
    annotationitems/kadasannotationitemcontrollers.cpp
    annotationitems/kadasannotationlayerhelpers.cpp
    annotationitems/kadasannotationlayerregistry.cpp
    annotationitems/kadasannotationprojectintegration.cpp
    annotationitems/kadasannotationrotation.cpp
    annotationitems/kadasannotationstyleeditor.cpp
    annotationitems/kadascircleannotationcontroller.cpp
    annotationitems/kadascircleannotationitem.cpp
    annotationitems/kadascoordcrossannotationcontroller.cpp
    annotationitems/kadascoordcrossannotationitem.cpp
    annotationitems/kadasgpxrouteannotationcontroller.cpp
    annotationitems/kadasgpxrouteannotationitem.cpp
    annotationitems/kadasgpxwaypointannotationcontroller.cpp
    annotationitems/kadasgpxwaypointannotationitem.cpp
    annotationitems/kadaslineannotationcontroller.cpp
    annotationitems/kadasmarkerannotationcontroller.cpp
    annotationitems/kadasmilxannotationcontroller.cpp
    annotationitems/kadasmilxannotationitem.cpp
    annotationitems/kadasmilxlayersettings.cpp
    annotationitems/kadaspictureannotationcontroller.cpp
    annotationitems/kadaspinannotationcontroller.cpp
    annotationitems/kadaspinannotationitem.cpp
    annotationitems/kadaspointtextannotationcontroller.cpp
    annotationitems/kadaspolygonannotationcontroller.cpp
    annotationitems/kadasrectangleannotationcontroller.cpp
    annotationitems/kadassvgmarkerannotationcontroller.cpp
    annotationitems/kadassvgmarkerannotationitem.cpp
    annotationitems/kadasrectangleannotationitem.cpp
    maptools/kadasmaptooldeleteitems.cpp
    maptools/kadasmaptooleditannotationitem.cpp
    maptools/kadasmaptoolheightprofile.cpp
    maptools/kadasmaptoolhillshade.cpp
    maptools/kadasmaptoolmeasure.cpp
    maptools/kadasmaptoolminmax.cpp
    maptools/kadasmaptoolpan.cpp
    maptools/kadasmaptoolselectrect.cpp
    maptools/kadasmaptoolslope.cpp
    maptools/kadasmaptoolviewshed.cpp
    maptools/kadasshapecapturemaptool.cpp
    milx/kadasmilxclient.cpp
    milx/kadasmilxlayerpropertiespage.cpp
    milx/kadasmilxlibrary.cpp
    catalog/kadasarcgisportalcatalogprovider.cpp
    catalog/kadasarcgisrestcatalogprovider.cpp
    catalog/kadasgeoadminrestcatalogprovider.cpp
    catalog/kadasvbscatalogprovider.cpp
    search/kadasalternategotolocatorfilter.cpp
    search/kadaslocaldatasearchprovider.cpp
    search/kadaslocationsearchprovider.cpp
    search/kadasmapserverfindsearchprovider.cpp
    search/kadaspinsearchprovider.cpp
    search/kadasremotedatasearchprovider.cpp
    search/kadasworldlocationsearchprovider.cpp
)

set(kadas_gui_HDR
    kadasattributetabledialog.h
    kadasbottombar.h
    kadassidepanel.h
    kadassidepanelhost.h
    kadascatalogbrowser.h
    kadascatalogprovider.h
    kadasclipboard.h
    kadascoordinatedisplayer.h
    kadascoordinateinput.h
    kadascrsselection.h
    kadasfeaturepicker.h
    kadasfloatinginputwidget.h
    kadasheightprofiledialog.h
    kadasattributetypes.h
    kadaslayerpropertiesdialog.h
    kadaslayerselectionwidget.h
    kadasmapitemtooltip.h
    kadasplugininterface.h
    kadasprojectmigration.h
    kadasprojecttemplateselectiondialog.h
    kadasribbonbutton.h
    kadasrichtexteditor.h
    kadassearchprovider.h
    kadastextbrowser.h
    annotationitems/kadasannotationcontrollerregistry.h
    annotationitems/kadasannotationitemcontext.h
    annotationitems/kadasannotationitemcontroller.h
    annotationitems/kadasannotationitemcontrollers.h
    annotationitems/kadasannotationlayerhelpers.h
    annotationitems/kadasannotationlayerregistry.h
    annotationitems/kadasannotationprojectintegration.h
    annotationitems/kadasannotationrotation.h
    annotationitems/kadasannotationshadow.h
    annotationitems/kadasannotationstyleeditor.h
    annotationitems/kadasannotationzindex.h
    annotationitems/kadascircleannotationcontroller.h
    annotationitems/kadascircleannotationitem.h
    annotationitems/kadascoordcrossannotationcontroller.h
    annotationitems/kadascoordcrossannotationitem.h
    annotationitems/kadasgpxrouteannotationcontroller.h
    annotationitems/kadasgpxrouteannotationitem.h
    annotationitems/kadasgpxwaypointannotationcontroller.h
    annotationitems/kadasgpxwaypointannotationitem.h
    annotationitems/kadaslineannotationcontroller.h
    annotationitems/kadasmarkerannotationcontroller.h
    annotationitems/kadasmilxannotationcontroller.h
    annotationitems/kadasmilxannotationitem.h
    annotationitems/kadasmilxlayersettings.h
    annotationitems/kadaspictureannotationcontroller.h
    annotationitems/kadaspinannotationcontroller.h
    annotationitems/kadaspinannotationitem.h
    annotationitems/kadaspointtextannotationcontroller.h
    annotationitems/kadaspolygonannotationcontroller.h
    annotationitems/kadasrectangleannotationcontroller.h
    annotationitems/kadasrectangleannotationitem.h
    annotationitems/kadassvgmarkerannotationcontroller.h
    annotationitems/kadassvgmarkerannotationitem.h
    maptools/kadasmaptooldeleteitems.h
    maptools/kadasmaptooleditannotationitem.h
    maptools/kadasmaptoolheightprofile.h
    maptools/kadasmaptoolhillshade.h
    maptools/kadasmaptoolmeasure.h
    maptools/kadasmaptoolminmax.h
    maptools/kadasmaptoolpan.h
    maptools/kadasmaptoolselectrect.h
    maptools/kadasmaptoolslope.h
    maptools/kadasmaptoolviewshed.h
    maptools/kadasshapecapturemaptool.h
    milx/kadasmilxclient.h
    milx/kadasmilxinterface.h
    milx/kadasmilxlayerpropertiespage.h
    milx/kadasmilxlibrary.h
    catalog/kadasarcgisportalcatalogprovider.h
    catalog/kadasarcgisrestcatalogprovider.h
    catalog/kadasgeoadminrestcatalogprovider.h
    catalog/kadasvbscatalogprovider.h
    search/kadasalternategotolocatorfilter.h
    search/kadaslocaldatasearchprovider.h
    search/kadaslocationsearchprovider.h
    search/kadasmapserverfindsearchprovider.h
    search/kadaspinsearchprovider.h
    search/kadasremotedatasearchprovider.h
    search/kadasworldlocationsearchprovider.h
    kadasmapwidget.h
)

file(GLOB kadas_gui_UI ui/*.ui)
list(SORT kadas_gui_UI)

qt_wrap_ui(kadas_gui_UI_HDR ${kadas_gui_UI})

set_property(SOURCE ${kadas_gui_UI} PROPERTY SKIP_AUTOMOC ON)

add_library(kadas_gui ${kadas_gui_SRC} ${kadas_gui_HDR} ${kadas_gui_UI})

if(APPLE)
  target_sources(kadas_gui PRIVATE kadasclipboardutils.mm)
  target_link_libraries(kadas_gui "-framework AppKit")
endif()

pkg_search_module(CAIRO IMPORTED_TARGET cairo REQUIRED)
pkg_search_module(LIBRSVG IMPORTED_TARGET librsvg-2.0 REQUIRED)

find_package(Qwt REQUIRED)
find_package(GeographicLib REQUIRED)

target_link_libraries(
  kadas_gui
  Qt6::Widgets
  Qt6::Network
  Qt6::Svg
  Qt6::Xml
  Qt6::Concurrent
  QuaZip::QuaZip
  QGIS::Gui
  QGIS::3D
  ${GeographicLib_LIBRARIES}
  PkgConfig::LIBRSVG
  PkgConfig::CAIRO
  Exiv2::exiv2lib
  Qwt::Qwt
  kadas_core
  kadas_analysis
)

target_include_directories(kadas_gui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

generate_export_header(
  kadas_gui BASE_NAME KADAS_GUI EXPORT_FILE_NAME kadas_gui.h
)
set(kadas_gui_HDR ${kadas_gui_HDR} ${CMAKE_CURRENT_BINARY_DIR}/kadas_gui.h)

install(
  TARGETS kadas_gui
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
  LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(FILES ${kadas_gui_HDR} DESTINATION include/kadas/gui)
