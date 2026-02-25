set(kadas_gui_SRC
    kadasattributetabledialog.cpp
    kadasbottombar.cpp
    kadascatalogbrowser.cpp
    kadascatalogprovider.cpp
    kadasclipboard.cpp
    kadascoordinatedisplayer.cpp
    kadascoordinateinput.cpp
    kadascrsselection.cpp
    kadasfeaturepicker.cpp
    kadasfloatinginputwidget.cpp
    kadasheightprofiledialog.cpp
    kadasitemcontextmenuactions.cpp
    kadasitemlayer.cpp
    kadaslayerpropertiesdialog.cpp
    kadaslayerselectionwidget.cpp
    kadasmapcanvasitem.cpp
    kadasmapcanvasitemmanager.cpp
    kadasmapitemtooltip.cpp
    kadasmapwidget.cpp
    kadasprojectmigration.cpp
    kadasprojecttemplateselectiondialog.cpp
    kadasribbonbutton.cpp
    kadasrichtexteditor.cpp
    kadastextbrowser.cpp
    maptools/kadasmaptoolcreateitem.cpp
    maptools/kadasmaptooldeleteitems.cpp
    maptools/kadasmaptooledititem.cpp
    maptools/kadasmaptooledititemgroup.cpp
    maptools/kadasmaptoolheightprofile.cpp
    maptools/kadasmaptoolhillshade.cpp
    maptools/kadasmaptoolmeasure.cpp
    maptools/kadasmaptoolminmax.cpp
    maptools/kadasmaptoolpan.cpp
    maptools/kadasmaptoolselectrect.cpp
    maptools/kadasmaptoolslope.cpp
    maptools/kadasmaptoolviewshed.cpp
    milx/kadasmilxclient.cpp
    milx/kadasmilxeditor.cpp
    milx/kadasmilxitem.cpp
    milx/kadasmilxlayer.cpp
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
    mapitemeditors/kadasgpxrouteeditor.cpp
    mapitemeditors/kadasgpxwaypointeditor.cpp
    mapitemeditors/kadasmapitemeditor.cpp
    mapitemeditors/kadasredliningitemeditor.cpp
    mapitemeditors/kadasredliningtexteditor.cpp
    mapitemeditors/kadassymbolattributeseditor.cpp
    mapitems/kadasanchoreditem.cpp
    mapitems/kadascircleitem.cpp
    mapitems/kadascircularsectoritem.cpp
    mapitems/kadascoordinatecrossitem.cpp
    mapitems/kadasgeometryitem.cpp
    mapitems/kadasgpxrouteitem.cpp
    mapitems/kadasgpxwaypointitem.cpp
    mapitems/kadaslineitem.cpp
    mapitems/kadasmapitem.cpp
    mapitems/kadaspictureitem.cpp
    mapitems/kadaspointitem.cpp
    mapitems/kadaspolygonitem.cpp
    mapitems/kadasrectangleitem.cpp
    mapitems/kadasrectangleitembase.cpp
    mapitems/kadasselectionrectitem.cpp
    mapitems/kadassymbolitem.cpp
    mapitems/kadastextitem.cpp
)

set(kadas_gui_HDR
    kadasattributetabledialog.h
    kadasbottombar.h
    kadascatalogbrowser.h
    kadascatalogprovider.h
    kadasclipboard.h
    kadascoordinatedisplayer.h
    kadascoordinateinput.h
    kadascrsselection.h
    kadasfeaturepicker.h
    kadasfloatinginputwidget.h
    kadasheightprofiledialog.h
    kadasitemcontextmenuactions.h
    kadasitemlayer.h
    kadaslayerpropertiesdialog.h
    kadaslayerselectionwidget.h
    kadasmapcanvasitem.h
    kadasmapcanvasitemmanager.h
    kadasmapiteminterface.h
    kadasmapitemtooltip.h
    kadasplugininterface.h
    kadasprojectmigration.h
    kadasprojecttemplateselectiondialog.h
    kadasribbonbutton.h
    kadasrichtexteditor.h
    kadassearchprovider.h
    kadastextbrowser.h
    maptools/kadasmaptoolcreateitem.h
    maptools/kadasmaptooldeleteitems.h
    maptools/kadasmaptooledititem.h
    maptools/kadasmaptooledititemgroup.h
    maptools/kadasmaptoolheightprofile.h
    maptools/kadasmaptoolhillshade.h
    maptools/kadasmaptoolmeasure.h
    maptools/kadasmaptoolminmax.h
    maptools/kadasmaptoolpan.h
    maptools/kadasmaptoolselectrect.h
    maptools/kadasmaptoolslope.h
    maptools/kadasmaptoolviewshed.h
    milx/kadasmilxclient.h
    milx/kadasmilxeditor.h
    milx/kadasmilxinterface.h
    milx/kadasmilxitem.h
    milx/kadasmilxlayer.h
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
    mapitemeditors/kadasgpxrouteeditor.h
    mapitemeditors/kadasgpxwaypointeditor.h
    mapitemeditors/kadasmapitemeditor.h
    mapitemeditors/kadasredliningitemeditor.h
    mapitemeditors/kadasredliningtexteditor.h
    mapitemeditors/kadassymbolattributeseditor.h
    mapitems/kadasanchoreditem.h
    mapitems/kadascircleitem.h
    mapitems/kadascircularsectoritem.h
    mapitems/kadascoordinatecrossitem.h
    mapitems/kadasgeometryitem.h
    mapitems/kadasgpxrouteitem.h
    mapitems/kadasgpxwaypointitem.h
    mapitems/kadaslineitem.h
    mapitems/kadasmapitem.h
    mapitems/kadaspictureitem.h
    mapitems/kadaspointitem.h
    mapitems/kadaspolygonitem.h
    mapitems/kadasrectangleitem.h
    mapitems/kadasrectangleitembase.h
    mapitems/kadasselectionrectitem.h
    mapitems/kadassymbolitem.h
    mapitems/kadastextitem.h
)

file(GLOB kadas_gui_UI ui/*.ui)
list(SORT kadas_gui_UI)

qt_wrap_ui(kadas_gui_UI_HDR ${kadas_gui_UI})

set_property(SOURCE ${kadas_gui_UI} PROPERTY SKIP_AUTOMOC ON)

add_library(kadas_gui ${kadas_gui_SRC} ${kadas_gui_HDR} ${kadas_gui_UI})

pkg_search_module(CAIRO IMPORTED_TARGET cairo REQUIRED)
pkg_search_module(LIBRSVG IMPORTED_TARGET librsvg-2.0 REQUIRED)
pkg_search_module(SVG2SVGT IMPORTED_TARGET svg2svgt REQUIRED)

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
  ${GeographicLib_LIBRARIES}
  PkgConfig::LIBRSVG
  PkgConfig::SVG2SVGT
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
