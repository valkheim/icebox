#define FDP_MODULE "tests_win10"
#include <icebox/callstack.hpp>
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/reader.hpp>
#include <icebox/sym.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/tracer/syscalls32.gen.hpp>
#include <icebox/tracer/tracer.hpp>
#include <icebox/utils/fnview.hpp>
#include <icebox/waiter.hpp>

#include <fmt/format.h>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#include <map>
#include <unordered_set>

namespace
{
    struct Win10Test
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ptr_core = core::attach("win10");
            ASSERT_TRUE(ptr_core);
            const auto paused = state::pause(*ptr_core);
            ASSERT_TRUE(paused);
        }

        void TearDown() override
        {
            const auto resumed = state::resume(*ptr_core);
            EXPECT_TRUE(resumed);
        }

        std::shared_ptr<core::Core> ptr_core;
    };
}

TEST_F(Win10Test, attach)
{
}

TEST_F(Win10Test, drivers)
{
    using Driver  = std::tuple<uint64_t, uint64_t, size_t>;
    using Drivers = std::map<std::string, Driver>;

    Drivers drivers;
    auto& core = *ptr_core;
    os::driver_list(core, [&](driver_t drv)
    {
        const auto name = os::driver_name(core, drv);
        EXPECT_TRUE(!!name);
        const auto span = os::driver_span(core, drv);
        EXPECT_TRUE(!!span);
        drivers.emplace(*name, Driver{drv.id, span->addr, span->size});
        return WALK_NEXT;
    });
    EXPECT_NE(drivers.size(), 0u);
    const auto it = drivers.find(R"(\SystemRoot\system32\ntoskrnl.exe)");
    EXPECT_NE(it, drivers.end());

    const auto [id, addr, size] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);

    const auto want = addr + (size >> 1);
    const auto drv  = os::driver_find(core, want);
    EXPECT_TRUE(!!drv);
    EXPECT_EQ(id, drv->id);
}

TEST_F(Win10Test, processes)
{
    using Process   = std::tuple<uint64_t, uint64_t, uint64_t, flags_e>;
    using Processes = std::multimap<std::string, Process>;

    Processes processes;
    auto& core = *ptr_core;
    os::proc_list(core, [&](proc_t proc)
    {
        const auto name = os::proc_name(core, proc);
        EXPECT_TRUE(!!name);
        const auto pid = os::proc_id(core, proc);
        EXPECT_NE(pid, 0u);
        const auto flags = os::proc_flags(core, proc);
        processes.emplace(*name, Process{proc.id, proc.dtb.val, pid, flags});
        return WALK_NEXT;
    });
    EXPECT_NE(processes.size(), 0u);
    const auto it = processes.find("explorer.exe");
    EXPECT_NE(it, processes.end());

    const auto [id, dtb, pid, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(dtb, 0u);
    EXPECT_NE(pid, 0u);
    UNUSED(flags);

    const auto proc = os::proc_find(core, pid);
    EXPECT_TRUE(!!proc);
    EXPECT_EQ(id, proc->id);
    EXPECT_EQ(dtb, proc->dtb.val);

    const auto valid = os::proc_is_valid(core, *proc);
    EXPECT_TRUE(valid);

    // check parent
    const auto parent = os::proc_parent(core, *proc);
    EXPECT_TRUE(!!parent);
    const auto parent_name = os::proc_name(core, *parent);
    EXPECT_TRUE(!!parent_name);
    EXPECT_EQ(*parent_name, "userinit.exe");

    // join proc in kernel
    os::proc_join(core, *proc, os::JOIN_ANY_MODE);
    const auto kcur = os::proc_current(core);
    EXPECT_TRUE(!!kcur);
    EXPECT_EQ(id, kcur->id);
    EXPECT_EQ(dtb, kcur->dtb.val);

    // join proc in user-mode
    os::proc_join(core, *proc, os::JOIN_USER_MODE);
    const auto cur = os::proc_current(core);
    EXPECT_TRUE(!!cur);
    EXPECT_EQ(id, cur->id);
    EXPECT_EQ(dtb, cur->dtb.val);
}

TEST_F(Win10Test, threads)
{
    using Threads = std::set<uint64_t>;

    auto& core          = *ptr_core;
    const auto explorer = os::proc_find(core, "explorer.exe", flags_e::FLAGS_NONE);
    EXPECT_TRUE(!!explorer);

    Threads threads;
    os::thread_list(core, *explorer, [&](thread_t thread)
    {
        const auto proc = os::thread_proc(core, thread);
        EXPECT_TRUE(!!proc);
        EXPECT_EQ(proc->id, explorer->id);
        const auto tid = os::thread_id(core, *proc, thread);
        EXPECT_NE(tid, 0u);
        threads.emplace(tid);
        return WALK_NEXT;
    });
    EXPECT_NE(threads.size(), 0u);

    os::proc_join(core, *explorer, os::JOIN_ANY_MODE);
    const auto current = os::thread_current(core);
    EXPECT_TRUE(!!current);

    const auto tid = os::thread_id(core, *explorer, *current);
    const auto it  = threads.find(tid);
    EXPECT_NE(it, threads.end());
}

TEST_F(Win10Test, modules)
{
    using Module  = std::tuple<uint64_t, uint64_t, size_t, flags_e>;
    using Modules = std::multimap<std::string, Module>;

    auto& core      = *ptr_core;
    const auto proc = os::proc_find(core, "explorer.exe", flags_e::FLAGS_NONE);
    EXPECT_TRUE(!!proc);

    Modules modules;
    os::mod_list(core, *proc, [&](mod_t mod)
    {
        const auto name = os::mod_name(core, *proc, mod);
        if(!name)
            return WALK_NEXT; // FIXME

        const auto span = os::mod_span(core, *proc, mod);
        EXPECT_TRUE(!!span);
        modules.emplace(*name, Module{mod.id, span->addr, span->size, mod.flags});
        return WALK_NEXT;
    });
    EXPECT_NE(modules.size(), 0u);

    const auto it = modules.find(R"(C:\Windows\SYSTEM32\ntdll.dll)");
    EXPECT_NE(it, modules.end());

    const auto [id, addr, size, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);
    UNUSED(flags);

    const auto want = addr + (size >> 1);
    const auto mod  = os::mod_find(core, *proc, want);
    EXPECT_TRUE(!!mod);
    EXPECT_EQ(id, mod->id);
}

namespace
{
    template <typename T>
    static void run_until(core::Core& core, T predicate)
    {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto end = now + std::chrono::seconds(8);
        while(!predicate() && std::chrono::high_resolution_clock::now() < end)
        {
            state::resume(core);
            state::wait(core);
        }
        EXPECT_TRUE(predicate());
    }
}

TEST_F(Win10Test, unable_to_single_step_query_information_process)
{
    auto& core      = *ptr_core;
    const auto proc = waiter::proc_wait(core, "ProcessHacker.exe", FLAGS_NONE);
    EXPECT_TRUE(!!proc);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_32BIT);
    EXPECT_TRUE(!!ntdll);

    auto loader   = sym::Loader{core};
    const auto ok = loader.mod_load(*proc, *ntdll);
    EXPECT_TRUE(ok);

    wow64::syscalls32 tracer{core, loader.symbols(), "ntdll"};
    bool found = false;
    // ZwQueryInformationProcess in 32-bit has code reading itself
    // we need to ensure we can break this function & resume properly
    // FDP had a bug where this was not possible
    tracer.register_ZwQueryInformationProcess(*proc, [&](wow64::HANDLE, wow64::PROCESSINFOCLASS, wow64::PVOID, wow64::ULONG, wow64::PULONG)
    {
        found = true;
    });
    run_until(core, [&] { return found; });
}

TEST_F(Win10Test, unset_bp_when_two_bps_share_phy_page)
{
    auto& core      = *ptr_core;
    const auto proc = waiter::proc_wait(core, "ProcessHacker.exe", FLAGS_NONE);
    EXPECT_TRUE(!!proc);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_32BIT);
    EXPECT_TRUE(!!ntdll);

    auto loader   = sym::Loader{core};
    const auto ok = loader.mod_load(*proc, *ntdll);
    EXPECT_TRUE(ok);

    // break on a single function once
    wow64::syscalls32 tracer{core, loader.symbols(), "ntdll"};
    int func_start = 0;
    tracer.register_ZwWaitForSingleObject(*proc, [&](wow64::HANDLE, wow64::BOOLEAN, wow64::PLARGE_INTEGER)
    {
        ++func_start;
    });
    run_until(core, [&] { return func_start > 0; });

    // set a breakpoint on next instruction
    state::single_step(core);
    const auto addr_a = registers::read(core, FDP_RIP_REGISTER);
    int func_a        = 0;
    auto bp_a         = state::set_breakpoint(core, "ZwWaitForSingleObject + $1", addr_a, *proc, [&]
    {
        func_a++;
    });

    // set a breakpoint on next instruction again
    // we are sure the previous bp share a physical page with at least one bp
    state::single_step(core);
    const auto addr_b = registers::read(core, FDP_RIP_REGISTER);
    int func_b        = 0;
    const auto bp_b   = state::set_breakpoint(core, "ZwWaitForSingleObject + $2", addr_b, *proc, [&]
    {
        func_b++;
    });

    // wait to break on third breakpoint
    run_until(core, [&] { return func_b > 0; });

    // remove mid breakpoint
    bp_a.reset();

    // ensure vm is not frozen
    run_until(core, [&] { return func_start > 4; });
}

TEST_F(Win10Test, memory)
{
    auto& core      = *ptr_core;
    const auto proc = os::proc_find(core, "explorer.exe", flags_e::FLAGS_NONE);
    EXPECT_TRUE(!!proc);
    LOG(INFO, "explorer dtb: 0x%" PRIx64, proc->dtb.val);

    os::proc_join(core, *proc, os::JOIN_USER_MODE);

    auto from_reader  = std::vector<uint8_t>{};
    auto from_virtual = std::vector<uint8_t>{};
    const auto reader = reader::make(core, *proc);
    os::mod_list(core, *proc, [&](mod_t mod)
    {
        const auto span = os::mod_span(core, *proc, mod);
        EXPECT_TRUE(!!span);

        from_reader.resize(span->size);
        auto ok = reader.read(&from_reader[0], span->addr, span->size);
        EXPECT_TRUE(ok);

        from_virtual.resize(span->size);
        ok = memory::read_virtual_with_dtb(core, &from_virtual[0], proc->dtb, span->addr, span->size);
        EXPECT_TRUE(ok);

        EXPECT_EQ(0, memcmp(&from_reader[0], &from_virtual[0], span->size));

        const auto phy = memory::virtual_to_physical(core, span->addr, proc->dtb);
        EXPECT_TRUE(!!phy);
        return WALK_NEXT;
    });
}

namespace
{
    static size_t count_symbols(sym::Symbols& symbols)
    {
        size_t count  = 0;
        const auto ok = symbols.list([&](const auto& /*sym*/)
        {
            ++count;
            return WALK_NEXT;
        });
        EXPECT_TRUE(ok);
        return count;
    }
}

TEST_F(Win10Test, loader)
{
    auto& core      = *ptr_core;
    const auto proc = waiter::proc_wait(core, "dwm.exe", FLAGS_NONE);
    ASSERT_TRUE(!!proc);

    os::proc_join(core, *proc, os::JOIN_ANY_MODE);
    auto drivers = sym::Loader{core};
    drivers.drv_listen({});
    EXPECT_GE(count_symbols(drivers.symbols()), 128u);

    os::proc_join(core, *proc, os::JOIN_USER_MODE);
    auto modules = sym::Loader{core};
    modules.mod_listen(*proc, {});
    EXPECT_GE(count_symbols(modules.symbols()), 32u);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
    ASSERT_TRUE(ntdll);
}

TEST_F(Win10Test, tracer)
{
    auto& core      = *ptr_core;
    const auto proc = waiter::proc_wait(core, "dwm.exe", FLAGS_NONE);
    ASSERT_TRUE(!!proc);

    os::proc_join(core, *proc, os::JOIN_USER_MODE);
    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
    ASSERT_TRUE(ntdll);

    auto loader = sym::Loader{core};
    loader.mod_load(*proc, *ntdll);

    using Calls = std::unordered_set<std::string>;
    auto calls  = Calls{};
    auto tracer = nt::syscalls{core, loader.symbols(), "ntdll"};
    auto count  = 0;
    tracer.register_all(*proc, [&](const auto& cfg)
    {
        calls.insert(cfg.name);
        ++count;
    });
    run_until(core, [&] { return count > 32; });
    for(const auto& call : calls)
        LOG(INFO, "call: %s", call.data());
}

namespace
{
    static std::string dump_address(sym::Symbols& symbols, uint64_t addr)
    {
        const auto cur = symbols.find(addr);
        if(!cur)
            return fmt::format("{:#x}", addr);

        return sym::to_string(*cur);
    }
}

TEST_F(Win10Test, callstacks)
{
    auto& core      = *ptr_core;
    const auto proc = waiter::proc_wait(core, "dwm.exe", FLAGS_NONE);
    ASSERT_TRUE(!!proc);

    auto loader = sym::Loader{core};
    loader.mod_listen(*proc, {});
    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
    ASSERT_TRUE(ntdll);

    auto& symbols   = loader.symbols();
    auto tracer     = nt::syscalls{core, symbols, "ntdll"};
    auto callstacks = callstack::make_callstack_nt(core);
    auto count      = size_t{0};
    tracer.register_all(*proc, [&](const auto& /* cfg*/)
    {
        LOG(INFO, " ");
        auto idx = size_t{0};
        callstacks->get_callstack(*proc, [&](callstack::callstep_t step)
        {
            const auto symbol = dump_address(symbols, step.addr);
            LOG(INFO, "0x%" PRIx64 ": %s", idx++, symbol.data());
            return WALK_NEXT;
        });
        count++;
    });
    run_until(core, [&] { return count > 32; });
}
