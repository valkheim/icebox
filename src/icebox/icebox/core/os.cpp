#include "os.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "core"
#include "core.hpp"
#include "core_private.hpp"
#include "os_private.hpp"
#include "utils/fnview.hpp"

bool os::is_kernel_address(core::Core& core, uint64_t ptr)
{
    return core.os_->is_kernel_address(ptr);
}

bool os::can_inject_fault(core::Core& core, uint64_t ptr)
{
    if(!core.os_)
        return false;

    return core.os_->can_inject_fault(ptr);
}

bool os::reader_setup(core::Core& core, reader::Reader& reader, opt<proc_t> proc)
{
    if(!core.os_)
        return true;

    return core.os_->reader_setup(reader, proc);
}

sym::Symbols& os::kernel_symbols(core::Core& core)
{
    return core.os_->kernel_symbols();
}

bool os::proc_list(core::Core& core, on_proc_fn on_proc)
{
    return core.os_->proc_list(on_proc);
}

opt<proc_t> os::proc_current(core::Core& core)
{
    return core.os_->proc_current();
}

opt<proc_t> os::proc_find(core::Core& core, std::string_view name, flags_e flags)
{
    return core.os_->proc_find(name, flags);
}

opt<proc_t> os::proc_find(core::Core& core, uint64_t pid)
{
    return core.os_->proc_find(pid);
}

opt<std::string> os::proc_name(core::Core& core, proc_t proc)
{
    return core.os_->proc_name(proc);
}

bool os::proc_is_valid(core::Core& core, proc_t proc)
{
    return core.os_->proc_is_valid(proc);
}

uint64_t os::proc_id(core::Core& core, proc_t proc)
{
    return core.os_->proc_id(proc);
}

flags_e os::proc_flags(core::Core& core, proc_t proc)
{
    return core.os_->proc_flags(proc);
}

void os::proc_join(core::Core& core, proc_t proc, join_e join)
{
    return core.os_->proc_join(proc, join);
}

opt<phy_t> os::proc_resolve(core::Core& core, proc_t proc, uint64_t ptr)
{
    return core.os_->proc_resolve(proc, ptr);
}

opt<proc_t> os::proc_select(core::Core& core, proc_t proc, uint64_t ptr)
{
    return core.os_->proc_select(proc, ptr);
}

opt<proc_t> os::proc_parent(core::Core& core, proc_t proc)
{
    return core.os_->proc_parent(proc);
}

bool os::thread_list(core::Core& core, proc_t proc, on_thread_fn on_thread)
{
    return core.os_->thread_list(proc, on_thread);
}

opt<thread_t> os::thread_current(core::Core& core)
{
    return core.os_->thread_current();
}

opt<proc_t> os::thread_proc(core::Core& core, thread_t thread)
{
    return core.os_->thread_proc(thread);
}

opt<uint64_t> os::thread_pc(core::Core& core, proc_t proc, thread_t thread)
{
    return core.os_->thread_pc(proc, thread);
}

uint64_t os::thread_id(core::Core& core, proc_t proc, thread_t thread)
{
    return core.os_->thread_id(proc, thread);
}

bool os::mod_list(core::Core& core, proc_t proc, on_mod_fn on_mod)
{
    return core.os_->mod_list(proc, on_mod);
}

opt<std::string> os::mod_name(core::Core& core, proc_t proc, mod_t mod)
{
    return core.os_->mod_name(proc, mod);
}

opt<span_t> os::mod_span(core::Core& core, proc_t proc, mod_t mod)
{
    return core.os_->mod_span(proc, mod);
}

opt<mod_t> os::mod_find(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.os_->mod_find(proc, addr);
}

bool os::vm_area_list(core::Core& core, proc_t proc, on_vm_area_fn on_vm_area)
{
    return core.os_->vm_area_list(proc, on_vm_area);
}

opt<vm_area_t> os::vm_area_find(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.os_->vm_area_find(proc, addr);
}

opt<span_t> os::vm_area_span(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_span(proc, vm_area);
}

vma_access_e os::vm_area_access(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_access(proc, vm_area);
}

vma_type_e os::vm_area_type(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_type(proc, vm_area);
}

opt<std::string> os::vm_area_name(core::Core& core, proc_t proc, vm_area_t vm_area)
{
    return core.os_->vm_area_name(proc, vm_area);
}

bool os::driver_list(core::Core& core, on_driver_fn on_driver)
{
    return core.os_->driver_list(on_driver);
}

opt<driver_t> os::driver_find(core::Core& core, uint64_t addr)
{
    return core.os_->driver_find(addr);
}

opt<std::string> os::driver_name(core::Core& core, driver_t drv)
{
    return core.os_->driver_name(drv);
}

opt<span_t> os::driver_span(core::Core& core, driver_t drv)
{
    return core.os_->driver_span(drv);
}

opt<os::bpid_t> os::listen_proc_create(core::Core& core, const on_proc_event_fn& on_proc_event)
{
    return core.os_->listen_proc_create(on_proc_event);
}

opt<os::bpid_t> os::listen_proc_delete(core::Core& core, const on_proc_event_fn& on_proc_event)
{
    return core.os_->listen_proc_delete(on_proc_event);
}

opt<os::bpid_t> os::listen_thread_create(core::Core& core, const on_thread_event_fn& on_thread_event)
{
    return core.os_->listen_thread_create(on_thread_event);
}

opt<os::bpid_t> os::listen_thread_delete(core::Core& core, const on_thread_event_fn& on_thread_event)
{
    return core.os_->listen_thread_delete(on_thread_event);
}

opt<os::bpid_t> os::listen_mod_create(core::Core& core, const on_mod_event_fn& on_load)
{
    return core.os_->listen_mod_create(on_load);
}

opt<os::bpid_t> os::listen_drv_create(core::Core& core, const on_drv_event_fn& on_load)
{
    return core.os_->listen_drv_create(on_load);
}

size_t os::unlisten(core::Core& core, bpid_t bpid)
{
    return core.os_->unlisten(bpid);
}

opt<arg_t> os::read_stack(core::Core& core, size_t index)
{
    return core.os_->read_stack(index);
}

opt<arg_t> os::read_arg(core::Core& core, size_t index)
{
    return core.os_->read_arg(index);
}

bool os::write_arg(core::Core& core, size_t index, arg_t arg)
{
    return core.os_->write_arg(index, arg);
}

void os::debug_print(core::Core& core)
{
    return core.os_->debug_print();
}
