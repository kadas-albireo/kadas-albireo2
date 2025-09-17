set(TESTS testkadascatalogbrowser.cpp)

foreach(TESTSRC ${TESTS})
  add_kadas_test(${TESTSRC}
    MODULE gui
    LINKEDLIBRARIES kadas_app
    LABELS "NO_CI")
endforeach(TESTSRC)
