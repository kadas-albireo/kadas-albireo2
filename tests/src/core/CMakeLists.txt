set(TESTS testkadasfileserver.cpp testkadasguidegridutils.cpp
          testkadasjsonutils.cpp
)

foreach(TESTSRC ${TESTS})
  add_kadas_test(${TESTSRC} MODULE core LINKEDLIBRARIES kadas_core)
endforeach(TESTSRC)
