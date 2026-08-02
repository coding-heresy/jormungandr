/** -*- mode: c++ -*-
 *
 * Copyright (C) 2026 Brian Davis
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Brian Davis <brian8702@sbcglobal.net>
 *
 */

/**
 * Library that converts the test_features library into a python
 * module using the reflexive library.
 */

#include "reflexive.h"
#include "test_features.h"

using namespace std;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace jmg
{

/**
 * python docstring for the ReflexiveFeatures module
 */
constexpr auto kReflexiveFeaturesModuleDocStr =
  "module for testing the python reflex library"sv;

/**
 * python docstring for the TestLifetime class
 */
constexpr auto kTestLifetimeDocStr =
  "class that logs lifetime events for testing with PythonReflex"sv;

/**
 * python docstring for the TestClass class
 */
constexpr auto kTestClassDocStr =
  "class that exhibits various behaviors for testing with PythonReflex"sv;

/**
 * python docstring for the TestContainer class
 */
constexpr auto kTestContainerDocStr =
  "class that exhibits container behavior for testing with PythonReflex"sv;

struct ReflexiveFeatures
  : PythonModule<ReflexiveFeatures,
                 kReflexiveFeaturesModuleDocStr,
                 PythonReflex<TestLifetime, kTestLifetimeDocStr>,
                 PythonReflex<TestClass, kTestClassDocStr>,
                 PythonReflex<TestContainer, kTestContainerDocStr>> {};

} // namespace jmg

JMG_DECLARE_MODULE(reflexive_features)
