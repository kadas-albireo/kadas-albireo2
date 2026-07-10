set(TESTS testkadassidepanelsize.cpp testkadassidepanelreflow.cpp)

foreach(TESTSRC ${TESTS})
  add_kadas_test(${TESTSRC} MODULE gui LINKEDLIBRARIES kadas_gui)
endforeach(TESTSRC)
