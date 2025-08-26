set(TESTS testkadasalternategotolocatorfilter.cpp)

foreach(TESTSRC ${TESTS})
  add_kadas_test(${TESTSRC} MODULE gui LINKEDLIBRARIES kadas_gui)
endforeach(TESTSRC)
