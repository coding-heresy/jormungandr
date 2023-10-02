
CXX := g++
# TODO extra system paths should be configurable via environment
# variable
CXXFLAGS := -std=c++20 -isystem /home/bd/install/include -Iinclude -Wall -Werror

JINC := include/jmg

JMG_HEADERS := \
$(JINC)/array_proxy.h \
$(JINC)/conversion.h \
$(JINC)/field.h \
$(JINC)/file_util.h \
$(JINC)/meta.h \
$(JINC)/object.h \
$(JINC)/preprocessor.h \
$(JINC)/ptree/ptree.h \
$(JINC)/quickfix/quickfix.h \
$(JINC)/safe_types.h \
$(JINC)/union.h \
$(JINC)/util.h

jmgc: jmgc.cpp $(JMG_HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

test_safe_types: test/test_safe_types.cpp $(JMG_HEADERS)
	$(CXX) -o test_safe_types $(CXXFLAGS) $< -o $@ -lgtest -lgtest_main
test_conversion: test/test_conversion.cpp $(JMG_HEADERS)
	$(CXX) -o test_conversion $(CXXFLAGS) $< -o $@ -lgtest -lgtest_main

test_meta: test/test_meta.cpp $(JMG_HEADERS)
	$(CXX) -o test_meta $(CXXFLAGS) $< -o $@ -lgtest -lgtest_main

test_object: test/test_object.cpp $(JMG_HEADERS)
	$(CXX) -o test_object $(CXXFLAGS) $< -o $@ -lgtest -lgtest_main

test_ptree: test/test_ptree.cpp $(JMG_HEADERS)
	$(CXX) -o test_ptree $(CXXFLAGS) $< -o $@ -lgtest -lgtest_main

#experiments: experiments.cpp $(JMG_HEADERS)
#	$(CXX) $(CXXFLAGS) $< -o $@
