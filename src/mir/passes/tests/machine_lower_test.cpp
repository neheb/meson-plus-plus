// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "passes.hpp"
#include "passes/private.hpp"
#include "state/state.hpp"
#include "toolchains/archiver.hpp"
#include "toolchains/common.hpp"
#include "toolchains/compilers/cpp/cpp.hpp"
#include "toolchains/linker.hpp"

#include "test_utils.hpp"

TEST(machine_lower, simple) {
    auto irlist = lower("x = 7\ny = host_machine.cpu_family()");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 2);
    const auto & r = irlist.instructions.back();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::String>>(r));
    ASSERT_EQ(std::get<std::shared_ptr<MIR::String>>(r)->value, "x86_64");
}

TEST(machine_lower, in_array) {
    auto irlist = lower("x = [host_machine.cpu_family()]");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::Array>>(r));
    const auto & a = std::get<std::shared_ptr<MIR::Array>>(r)->value;

    ASSERT_EQ(a.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::String>>(a[0]));
    ASSERT_EQ(std::get<std::shared_ptr<MIR::String>>(a[0])->value, "x86_64");
}

TEST(machine_lower, in_function_args) {
    auto irlist = lower("foo(host_machine.endian())");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 1);
    const auto & r = irlist.instructions.front();

    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(r));
    const auto & f = std::get<std::shared_ptr<MIR::FunctionCall>>(r);

    ASSERT_EQ(f->pos_args.size(), 1);
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::String>>(f->pos_args[0]));
    ASSERT_EQ(std::get<std::shared_ptr<MIR::String>>(f->pos_args[0])->value, "little");
}

TEST(machine_lower, in_condtion) {
    auto irlist = lower("if host_machine.cpu_family()\n x = 2\nendif");
    auto info = MIR::Machines::PerMachine<MIR::Machines::Info>(
        MIR::Machines::Info{MIR::Machines::Machine::BUILD, MIR::Machines::Kernel::LINUX,
                            MIR::Machines::Endian::LITTLE, "x86_64"});
    bool progress = MIR::Passes::machine_lower(&irlist, info);
    ASSERT_TRUE(progress);
    ASSERT_EQ(irlist.instructions.size(), 0);

    ASSERT_TRUE(is_con(irlist.next));
    const auto & con = get_con(irlist.next);
    const auto & obj = con->condition;
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<MIR::String>>(obj));
    ASSERT_EQ(std::get<std::shared_ptr<MIR::String>>(obj)->value, "x86_64");
}
