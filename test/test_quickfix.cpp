/** -*- mode: c++ -*-
 *
 * Copyright (C) 2024 Brian Davis
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

#include <gtest/gtest.h>

#include "test/quickfix_42.h"

using namespace fix_spec;
using namespace fix_spec::msg_header;
using namespace fix_spec::msg_trailer;
using namespace jmg;
using namespace std;

// test messages shamelessly stolen from
// https://www.fixsim.com/sample-fix-messages

std::string_view kTestNewOrderSingle =
  "8=FIX.4.29=16335=D34=97249=TESTBUY352=20190206-16:25:10.403"
  "56=TESTSELL311=14163685067084226997921=238=10040=154=155=AAPL"
  "60=20190206-16:25:08.968207=TO6000=TEST123410=106";

std::string_view kTestExecRpt =
  "8=FIX.4.29=27135=834=97449=TESTSELL352=20190206-16:26:09.059"
  "56=TESTBUY36=174.5111=14163685067084226997914=100.0000000000"
  "17=363685067168435797920=021=231=174.5132=100.000000000037=1005448"
  "38=10039=240=154=155=AAPL60=20190206-16:26:08.435150=2"
  "151=0.000000000010=194";

std::string_view kTestLogon =
  "8=FIX.4.29=7435=A34=97849=TESTSELL352=20190206-16:29:19.208"
  "56=TESTBUY398=0108=6010=137";

std::string_view kTestLogout =
  "8=FIX.4.29=6235=534=97749=TESTSELL352=20190206-16:28:51.518"
  "56=TESTBUY310=092";

TEST(QuickFixTest, TestNewOrderSingle) {
  using namespace new_order_single;
  const auto nos = NewOrderSingle(kTestNewOrderSingle, kLengthFields);

  // header fields

  EXPECT_EQ("FIX.4.2"s, jmg::get<BeginString>(nos));
  EXPECT_EQ(163U, jmg::get<BodyLength>(nos));
  // TODO check MsgType once conversion to enum is working
  EXPECT_EQ(972U, jmg::get<MsgSeqNum>(nos));
  EXPECT_EQ("TESTBUY3"s, jmg::get<SenderCompID>(nos));
  // TODO check SendingTime once timestamp conversion works
  EXPECT_EQ("TESTSELL3"s, jmg::get<TargetCompID>(nos));
  EXPECT_EQ("141636850670842269979"s, jmg::get<ClOrdID>(nos));
  EXPECT_FALSE(jmg::try_get<OnBehalfOfCompID>(nos).has_value());

  // NewOrderSingle fields

  // TODO check HandlInst once conversion to enum is working
  const auto orderQty = jmg::try_get<OrderQty>(nos);
  EXPECT_TRUE(orderQty.has_value());
  EXPECT_EQ(100, *orderQty);
  // TODO check OrdType once conversion to enum is working
  // TODO check Side once conversion to enum is working
  EXPECT_EQ("AAPL"s, jmg::get<Symbol>(nos));
  // TODO check TransactTime once timestamp conversion works
  const auto exchange = jmg::try_get<SecurityExchange>(nos);
  EXPECT_TRUE(exchange.has_value());
  EXPECT_EQ("TO"s, *exchange);
  EXPECT_FALSE(jmg::try_get<ClientID>(nos).has_value());

  // trailer fields

  EXPECT_FALSE(jmg::try_get<Signature>(nos).has_value());
  EXPECT_EQ("106"s, jmg::get<CheckSum>(nos));
}
