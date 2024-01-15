# Makefile
#
# Copyright (C) 2024 Brian Davis
# All Rights Reserved
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author: Brian Davis <brian8702@sbcglobal.net>
#

SHELL := /bin/bash
CXX := g++

TESTS := $(notdir $(basename $(wildcard test/*.cpp)))

.PHONY : tests

# TODO extra system paths should be configurable via environment
# variable
CXXFLAGS := -g -std=c++20 -isystem /home/bd/install/include -Iinclude -Wall -Werror -fconcepts-diagnostics-depth=10

LIBS := \
-L/home/bd/install/lib \
-lyaml-cpp

TEST_LIBS := -lgmock -lgtest -lgtest_main

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
$(JINC)/util.h \
$(JINC)/yaml/yaml.h

jmgc: jmgc.cpp $(JMG_HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

test_%: test/test_%.cpp $(JMG_HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS) $(TEST_LIBS)

tests: $(TESTS)
