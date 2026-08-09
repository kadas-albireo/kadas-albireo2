set(TESTS testkadasjsonutils.cpp)

foreach(TESTSRC ${TESTS})
  add_kadas_test(${TESTSRC} MODULE core LINKEDLIBRARIES kadas_core)
endforeach(TESTSRC)
