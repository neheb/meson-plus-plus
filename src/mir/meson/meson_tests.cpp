
// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Intel Corporation

#include <gtest/gtest.h>

#include "version.hpp"

namespace V = MIR::Version;
using V::to_string;

class VersionTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string, V::Operator>> {};

TEST_P(VersionTest, compare) {
    const auto & [input1, input2, op] = GetParam();
    bool result = V::compare(input1, op, input2);
    ASSERT_TRUE(result) << "Case: " << input1 << " " << to_string(op) << " " << input2 << std::endl;
}

INSTANTIATE_TEST_CASE_P(
    VersionComparisons, VersionTest,
    ::testing::Values(
        // examples from https://fedoraproject.org/wiki/Archive:Tools/RPM/VersionComparison
        std::make_tuple("1.0010", "1.9", V::Operator::GT),
        std::make_tuple("1.05", "1.5", V::Operator::EQ),
        std::make_tuple("1.0", "1", V::Operator::GT),
        std::make_tuple("2.50", "2.5", V::Operator::GT),
        std::make_tuple("fc4", "fc.4", V::Operator::EQ),
        std::make_tuple("FC5", "fc4", V::Operator::LT),
        std::make_tuple("2a", "2.0", V::Operator::LT),
        std::make_tuple("1.0", "1.fc4", V::Operator::GT),
        std::make_tuple("3.0.0_fc", "3.0.0.fc", V::Operator::EQ),
        // from RPM tests
        std::make_tuple("1.0", "1.0", V::Operator::EQ),
        std::make_tuple("1.0", "2.0", V::Operator::LT),
        std::make_tuple("2.0", "1.0", V::Operator::GT),
        std::make_tuple("2.0.1", "2.0.1", V::Operator::EQ),
        std::make_tuple("2.0", "2.0.1", V::Operator::LT),
        std::make_tuple("2.0.1", "2.0", V::Operator::GT),
        std::make_tuple("2.0.1a", "2.0.1a", V::Operator::EQ),
        std::make_tuple("2.0.1a", "2.0.1", V::Operator::GT),
        std::make_tuple("2.0.1", "2.0.1a", V::Operator::LT),
        std::make_tuple("5.5p1", "5.5p1", V::Operator::EQ),
        std::make_tuple("5.5p1", "5.5p2", V::Operator::LT),
        std::make_tuple("5.5p2", "5.5p1", V::Operator::GT),
        std::make_tuple("5.5p10", "5.5p10", V::Operator::EQ),
        std::make_tuple("5.5p1", "5.5p10", V::Operator::LT),
        std::make_tuple("5.5p10", "5.5p1", V::Operator::GT),
        std::make_tuple("10xyz", "10.1xyz", V::Operator::LT),
        std::make_tuple("10.1xyz", "10xyz", V::Operator::GT),
        std::make_tuple("xyz10", "xyz10", V::Operator::EQ),
        std::make_tuple("xyz10", "xyz10.1", V::Operator::LT),
        std::make_tuple("xyz10.1", "xyz10", V::Operator::GT),
        std::make_tuple("xyz.4", "xyz.4", V::Operator::EQ),
        std::make_tuple("xyz.4", "8", V::Operator::LT),
        std::make_tuple("8", "xyz.4", V::Operator::GT),
        std::make_tuple("xyz.4", "2", V::Operator::LT),
        std::make_tuple("2", "xyz.4", V::Operator::GT),
        std::make_tuple("5.5p2", "5.6p1", V::Operator::LT),
        std::make_tuple("5.6p1", "5.5p2", V::Operator::GT),
        std::make_tuple("5.6p1", "6.5p1", V::Operator::LT),
        std::make_tuple("6.5p1", "5.6p1", V::Operator::GT),
        std::make_tuple("6.0.rc1", "6.0", V::Operator::GT),
        std::make_tuple("6.0", "6.0.rc1", V::Operator::LT),
        std::make_tuple("10b2", "10a1", V::Operator::GT),
        std::make_tuple("10a2", "10b2", V::Operator::LT),
        std::make_tuple("1.0aa", "1.0aa", V::Operator::EQ),
        std::make_tuple("1.0a", "1.0aa", V::Operator::LT),
        std::make_tuple("1.0aa", "1.0a", V::Operator::GT),
        std::make_tuple("10.0001", "10.0001", V::Operator::EQ),
        std::make_tuple("10.0001", "10.1", V::Operator::EQ),
        std::make_tuple("10.1", "10.0001", V::Operator::EQ),
        std::make_tuple("10.0001", "10.0039", V::Operator::LT),
        std::make_tuple("10.0039", "10.0001", V::Operator::GT),
        std::make_tuple("4.999.9", "5.0", V::Operator::LT),
        std::make_tuple("5.0", "4.999.9", V::Operator::GT),
        std::make_tuple("20101121", "20101121", V::Operator::EQ),
        std::make_tuple("20101121", "20101122", V::Operator::LT),
        std::make_tuple("20101122", "20101121", V::Operator::GT),
        std::make_tuple("2_0", "2_0", V::Operator::EQ),
        std::make_tuple("2.0", "2_0", V::Operator::EQ),
        std::make_tuple("2_0", "2.0", V::Operator::EQ), std::make_tuple("a", "a", V::Operator::EQ),
        std::make_tuple("a+", "a+", V::Operator::EQ), std::make_tuple("a+", "a_", V::Operator::EQ),
        std::make_tuple("a_", "a+", V::Operator::EQ), std::make_tuple("+a", "+a", V::Operator::EQ),
        std::make_tuple("+a", "_a", V::Operator::EQ), std::make_tuple("_a", "+a", V::Operator::EQ),
        std::make_tuple("+_", "+_", V::Operator::EQ), std::make_tuple("_+", "+_", V::Operator::EQ),
        std::make_tuple("_+", "_+", V::Operator::EQ), std::make_tuple("+", "_", V::Operator::EQ),
        std::make_tuple("_", "+", V::Operator::EQ),
        // other tests
        std::make_tuple("0.99.beta19", "0.99.beta14", V::Operator::GT),
        std::make_tuple("1.0.0", "2.0.0", V::Operator::LT),
        std::make_tuple(".0.0", "2.0.0", V::Operator::LT),
        std::make_tuple("alpha", "beta", V::Operator::LT),
        std::make_tuple("1.0", "1.0.0", V::Operator::LT),
        std::make_tuple("2.456", "2.1000", V::Operator::LT),
        std::make_tuple("2.1000", "3.111", V::Operator::LT),
        std::make_tuple("2.001", "2.1", V::Operator::EQ),
        std::make_tuple("2.34", "2.34", V::Operator::EQ),
        std::make_tuple("6.1.2", "6.3.8", V::Operator::LT),
        std::make_tuple("1.7.3.0", "2.0.0", V::Operator::LT),
        std::make_tuple("2.24.51", "2.25", V::Operator::LT),
        std::make_tuple("2.1.5+20120813+gitdcbe778", "2.1.5", V::Operator::GT),
        std::make_tuple("3.4.1", "3.4b1", V::Operator::GT),
        std::make_tuple("041206", "200090325", V::Operator::LT),
        std::make_tuple("0.6.2+git20130413", "0.6.2", V::Operator::GT),
        std::make_tuple("2.6.0+bzr6602", "2.6.0", V::Operator::GT),
        std::make_tuple("2.6.0", "2.6b2", V::Operator::GT),
        std::make_tuple("2.6.0+bzr6602", "2.6b2x", V::Operator::GT),
        std::make_tuple("0.6.7+20150214+git3a710f9", "0.6.7", V::Operator::GT),
        std::make_tuple("15.8b", "15.8.0.1", V::Operator::LT),
        std::make_tuple("1.2rc1", "1.2.0", V::Operator::LT)));
