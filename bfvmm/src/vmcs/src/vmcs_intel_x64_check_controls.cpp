//
// Bareflank Hypervisor
//
// Copyright (C) 2015 Assured Information Security, Inc.
// Author: Rian Quinn        <quinnr@ainfosec.com>
// Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include <vmcs/vmcs_intel_x64.h>
#include <vmcs/vmcs_intel_x64_checks.h>
#include <vmcs/vmcs_intel_x64_exceptions.h>
#include <memory_manager/memory_manager.h>

// -----------------------------------------------------------------------------
//  Helpers
// -----------------------------------------------------------------------------

uint64_t
vmcs_intel_x64::get_pin_ctls() const
{ return vmread(VMCS_PIN_BASED_VM_EXECUTION_CONTROLS); }

uint64_t
vmcs_intel_x64::get_proc_ctls() const
{ return vmread(VMCS_PRIMARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS); }

uint64_t
vmcs_intel_x64::get_proc2_ctls() const
{
    auto ctls = get_proc_ctls();

    if proc_disabled(ctls, VM_EXEC_P_PROC_BASED_ACTIVATE_SECONDARY_CONTROLS)
        return 0;

    return vmread(VMCS_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS);
}

uint64_t
vmcs_intel_x64::get_exit_ctls() const
{ return vmread(VMCS_VM_EXIT_CONTROLS); }

uint64_t
vmcs_intel_x64::get_entry_ctls() const
{ return vmread(VMCS_VM_ENTRY_CONTROLS); }

// -----------------------------------------------------------------------------
//  Implementation
// -----------------------------------------------------------------------------

void
vmcs_intel_x64::check_vmcs_control_state()
{
    checks_on_vm_execution_control_fields();
    checks_on_vm_exit_control_fields();
    checks_on_vm_entry_control_fields();
}

void
vmcs_intel_x64::checks_on_vm_execution_control_fields()
{
    check_control_pin_based_ctls_reserved_properly_set();
    check_control_proc_based_ctls_reserved_properly_set();
    check_control_proc_based_ctls2_reserved_properly_set();
    check_control_cr3_count_less_then_4();
    check_control_io_bitmap_address_bits();
    check_control_msr_bitmap_address_bits();
    check_control_tpr_shadow_and_virtual_apic();
    check_control_nmi_exiting_and_virtual_nmi();
    check_control_virtual_nmi_and_nmi_window();
    check_control_virtual_apic_address_bits();
    check_control_virtual_x2apic_and_tpr();
    check_control_register_apic_mode_and_tpr();
    check_control_virtual_interrupt_delivery_and_tpr();
    check_control_x2apic_mode_and_virtual_apic_access();
    check_control_virtual_interrupt_and_external_interrupt();
    check_control_process_posted_interrupt_checks();
    check_control_vpid_checks();
    check_control_enable_ept_checks();
    check_control_unrestricted_guests();
    check_control_enable_vm_functions();
    check_control_enable_vmcs_shadowing();
    check_control_enable_ept_violation_checks();
}

void
vmcs_intel_x64::check_control_pin_based_ctls_reserved_properly_set()
{
    auto ia32_vmx_pinbased_ctls_msr =
        m_intrinsics->read_msr(IA32_VMX_PINBASED_CTLS_MSR);

    auto lower = ((ia32_vmx_pinbased_ctls_msr >> 00) & 0x00000000FFFFFFFF);
    auto upper = ((ia32_vmx_pinbased_ctls_msr >> 32) & 0x00000000FFFFFFFF);

    auto ctls = get_pin_ctls();
    auto ctls_lower = ((ctls >> 00) & 0x00000000FFFFFFFF);
    auto ctls_upper = ((ctls >> 32) & 0x00000000FFFFFFFF);

    if ((lower & ctls_lower) != lower || (upper & ~ctls_upper) != upper)
        throw vmcs_invalid_ctls("pin based", lower, upper,
                                ctls_lower, ctls_upper);
}

void
vmcs_intel_x64::check_control_proc_based_ctls_reserved_properly_set()
{
    auto ia32_vmx_procbased_ctls_msr =
        m_intrinsics->read_msr(IA32_VMX_PROCBASED_CTLS_MSR);

    auto lower = ((ia32_vmx_procbased_ctls_msr >> 00) & 0x00000000FFFFFFFF);
    auto upper = ((ia32_vmx_procbased_ctls_msr >> 32) & 0x00000000FFFFFFFF);

    auto ctls = get_proc_ctls();
    auto ctls_lower = ((ctls >> 00) & 0x00000000FFFFFFFF);
    auto ctls_upper = ((ctls >> 32) & 0x00000000FFFFFFFF);

    if ((lower & ctls_lower) != lower || (upper & ~ctls_upper) != upper)
        throw vmcs_invalid_ctls("proc based", lower, upper,
                                ctls_lower, ctls_upper);
}

void
vmcs_intel_x64::check_control_proc_based_ctls2_reserved_properly_set()
{
    auto ia32_vmx_procbased_ctls2_msr =
        m_intrinsics->read_msr(IA32_VMX_PROCBASED_CTLS2_MSR);

    auto lower = ((ia32_vmx_procbased_ctls2_msr >> 00) & 0x00000000FFFFFFFF);
    auto upper = ((ia32_vmx_procbased_ctls2_msr >> 32) & 0x00000000FFFFFFFF);

    auto ctls2 = get_proc2_ctls();
    auto ctls2_lower = ((ctls2 >> 00) & 0x00000000FFFFFFFF);
    auto ctls2_upper = ((ctls2 >> 32) & 0x00000000FFFFFFFF);

    if ((lower & ctls2_lower) != lower || (upper & ~ctls2_upper) != upper)
        throw vmcs_invalid_ctls("secondary proc based", lower, upper,
                                ctls2_lower, ctls2_upper);
}

void
vmcs_intel_x64::check_control_cr3_count_less_then_4()
{
    auto cr3_target_count = vmread(VMCS_CR3_TARGET_COUNT);

    if (cr3_target_count > 4)
        throw vmcs_invalid_field("cr3 count must be < 4", cr3_target_count);
}

void
vmcs_intel_x64::check_control_io_bitmap_address_bits()
{
    auto ctls = get_proc_ctls();

    if proc_disabled(ctls, VM_EXEC_P_PROC_BASED_USE_IO_BITMAPS)
        return;

    auto addr_a = vmread(VMCS_ADDRESS_OF_IO_BITMAP_A_FULL);
    auto addr_b = vmread(VMCS_ADDRESS_OF_IO_BITMAP_B_FULL);

    if ((addr_a & 0x0000000000000FFF) != 0)
        throw invalid_alignmnet("io bitmap a addr not page aligned", addr_a);

    if ((addr_b & 0x0000000000000FFF) != 0)
        throw invalid_alignmnet("io bitmap b addr not page aligned", addr_b);

    if (check_has_valid_address_width(addr_a) == false)
        throw invalid_address("io bitmap a addr too large", addr_a);

    if (check_has_valid_address_width(addr_b) == false)
        throw invalid_address("io bitmap b addr too large", addr_b);
}

void
vmcs_intel_x64::check_control_msr_bitmap_address_bits()
{
    auto ctls = get_proc_ctls();

    if proc_disabled(ctls, VM_EXEC_P_PROC_BASED_USE_MSR_BITMAPS)
        return;

    auto addr = vmread(VMCS_ADDRESS_OF_MSR_BITMAPS_FULL);

    if ((addr & 0x0000000000000FFF) != 0)
        throw invalid_alignmnet("msr bitmap addr not page aligned", addr);

    if (check_has_valid_address_width(addr) == false)
        throw invalid_address("msr bitmap addr too large", addr);
}

void
vmcs_intel_x64::check_control_tpr_shadow_and_virtual_apic()
{
    auto ctls1 = get_proc_ctls();

    if proc_disabled(ctls1, VM_EXEC_P_PROC_BASED_USE_TPR_SHADOW)
        return;

    auto phys_addr = vmread(VMCS_VIRTUAL_APIC_ADDRESS_FULL);

    if (phys_addr == 0)
        throw invalid_address("vitual apic physical addr is NULL", phys_addr);

    if ((phys_addr & 0x0000000000000FFF) != 0)
        throw invalid_alignmnet("virtual apic addr not 4k aligned", phys_addr);

    if (check_has_valid_address_width(phys_addr) == false)
        throw invalid_address("vitual apic addr too large", phys_addr);

    auto ctls2 = get_proc2_ctls();

    if proc2_disabled(ctls2, VM_EXEC_S_PROC_BASED_VIRTUAL_INTERRUPT_DELIVERY)
        return;

    auto tpr_threshold = vmread(VMCS_TPR_THRESHOLD);

    if ((tpr_threshold & 0x00000000FFFFFFF0) != 0)
        throw vmcs_invalid_field("bits 31:4 must be 0", tpr_threshold);

    auto ctls3 = get_proc2_ctls();

    if proc2_disabled(ctls3, VM_EXEC_S_PROC_BASED_VIRTUALIZE_APIC_ACCESSES)
        return;

    auto virt_addr = (uint64_t)g_mm->phys_to_virt((void *)phys_addr);

    if (virt_addr == 0)
        throw invalid_address("vitual apic virtual addr is NULL", virt_addr);

    auto vtpr = (((uint32_t *)((char *)virt_addr + 0x80))[0]);
    auto vtpr_74 = vtpr & 0x00000000000000F0;
    auto tpr_threshold_30 = vtpr & 0x000000000000000F;

    if (tpr_threshold_30 > (vtpr_74 >> 4))
        throw vmcs_invalid_field("invalid TPR threshold", tpr_threshold);
}

void
vmcs_intel_x64::check_control_nmi_exiting_and_virtual_nmi()
{
    auto ctls = get_pin_ctls();

    if pin_enabled(ctls, VM_EXEC_PIN_BASED_NMI_EXITING)
        return;

    if pin_enabled(ctls, VM_EXEC_PIN_BASED_VIRTUAL_NMIS)
        throw vmcs_invalid_field(
            "virtual NMI must be 0 if NMI exiting is 0", ctls);
}

void
vmcs_intel_x64::check_control_virtual_nmi_and_nmi_window()
{
    auto ctls1 = get_pin_ctls();
    auto ctls2 = get_proc_ctls();

    if pin_enabled(ctls1, VM_EXEC_PIN_BASED_VIRTUAL_NMIS)
        return;

    if proc_enabled(ctls2, VM_EXEC_P_PROC_BASED_NMI_WINDOW_EXITING)
        throw vmcs_invalid_field(
            "NMI window exiting must be 0 if virtual NMI is 0", ctls2);
}

void
vmcs_intel_x64::check_control_virtual_apic_address_bits()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_VIRTUALIZE_APIC_ACCESSES)
        return;

    auto phys_addr = vmread(VMCS_APIC_ACCESS_ADDRESS_FULL);

    if (phys_addr == 0)
        throw invalid_address("apic access physical addr is NULL", phys_addr);

    if ((phys_addr & 0x0000000000000FFF) != 0)
        throw invalid_alignmnet("apic access addr not 4k aligned", phys_addr);

    if (check_has_valid_address_width(phys_addr) == false)
        throw invalid_address("apic access addr too large", phys_addr);
}

void
vmcs_intel_x64::check_control_virtual_x2apic_and_tpr()
{
    auto ctls1 = get_proc_ctls();
    auto ctls2 = get_proc2_ctls();

    if proc_enabled(ctls1, VM_EXEC_P_PROC_BASED_USE_TPR_SHADOW)
        return;

    if proc2_enabled(ctls2, VM_EXEC_S_PROC_BASED_VIRTUALIZE_X2APIC_MODE)
        throw vmcs_invalid_field(
            "x2 apic mode must be 0 if use tpr shadow is 0", ctls2);
}

void
vmcs_intel_x64::check_control_register_apic_mode_and_tpr()
{
    auto ctls1 = get_proc_ctls();
    auto ctls2 = get_proc2_ctls();

    if proc_enabled(ctls1, VM_EXEC_P_PROC_BASED_USE_TPR_SHADOW)
        return;

    if proc2_enabled(ctls2, VM_EXEC_S_PROC_BASED_APIC_REGISTER_VIRTUALIZATION)
        throw vmcs_invalid_field(
            "apic register virt must be 0 if use tpr shadow is 0", ctls2);
}

void
vmcs_intel_x64::check_control_virtual_interrupt_delivery_and_tpr()
{
    auto ctls1 = get_proc_ctls();
    auto ctls2 = get_proc2_ctls();

    if proc_enabled(ctls1, VM_EXEC_P_PROC_BASED_USE_TPR_SHADOW)
        return;

    if proc2_enabled(ctls2, VM_EXEC_S_PROC_BASED_VIRTUAL_INTERRUPT_DELIVERY)
        throw vmcs_invalid_field(
            "virt interrupt delivery must be 0 if use tpr shadow is 0", ctls2);
}

void
vmcs_intel_x64::check_control_x2apic_mode_and_virtual_apic_access()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_VIRTUALIZE_X2APIC_MODE)
        return;

    if proc2_enabled(ctls, VM_EXEC_S_PROC_BASED_VIRTUALIZE_APIC_ACCESSES)
        throw vmcs_invalid_field(
            "apic accesses must be 0 if x2 apic mode is 1", ctls);
}

void
vmcs_intel_x64::check_control_virtual_interrupt_and_external_interrupt()
{
    auto ctls1 = get_pin_ctls();
    auto ctls2 = get_proc2_ctls();

    if proc2_disabled(ctls2, VM_EXEC_S_PROC_BASED_VIRTUAL_INTERRUPT_DELIVERY)
        return;

    if pin_disabled(ctls1, VM_EXEC_PIN_BASED_EXTERNAL_INTERRUPT_EXITING)
        throw vmcs_invalid_field("external interrupt exiting must be 1 "
                                 "if virtual interrupt delivery is 1", ctls1);
}

void
vmcs_intel_x64::check_control_process_posted_interrupt_checks()
{
    auto ctls1 = get_pin_ctls();
    auto ctls2 = get_proc2_ctls();

    if pin_disabled(ctls1, VM_EXEC_PIN_BASED_PROCESS_POSTED_INTERRUPTS)
        return;

    if proc2_disabled(ctls2, VM_EXEC_S_PROC_BASED_VIRTUAL_INTERRUPT_DELIVERY)
        throw vmcs_invalid_field("virtual interrupt delivery must be 1 "
                                 "if posted interrupts is 1", ctls2);

    auto ctls3 = get_exit_ctls();

    if exit_disabled(ctls3, VM_EXIT_CONTROL_ACKNOWLEDGE_INTERRUPT_ON_EXIT)
        throw vmcs_invalid_field("ack interrupt on exit must be 1 "
                                 "if posted interrupts is 1", ctls3);

    auto vector = vmread(VMCS_POSTED_INTERRUPT_NOTIFICATION_VECTOR);

    if ((vector & 0xFFFFFFFFFFFFFF00) != 0)
        throw vmcs_invalid_field("bits 15:8 of the notification vector must "
                                 "be 0 if posted interrupts is 1", vector);

    auto addr = vmread(VMCS_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS_FULL);

    if ((addr & 0x000000000000003F) != 0)
        throw vmcs_invalid_field("bits 5:0 of the interrupt descriptor addr "
                                 "must be 0 if posted interrupts is 1", vector);

    if (check_has_valid_address_width(addr) == false)
        throw invalid_address("interrupt descriptor addr too large", addr);
}

void
vmcs_intel_x64::check_control_vpid_checks()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_ENABLE_VPID)
        return;

    if (vmread(VMCS_VIRTUAL_PROCESSOR_IDENTIFIER) == 0)
        throw vmcs_invalid_field("vpid cannot equal 0", 0);
}

void
vmcs_intel_x64::check_control_enable_ept_checks()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_ENABLE_EPT)
        return;

    auto eptp = vmread(VMCS_EPT_POINTER_FULL);

    auto ia32_vmx_ept_vpid_cap_msr =
        m_intrinsics->read_msr(IA32_VMX_EPT_VPID_CAP_MSR);

    auto uncacheable = (ia32_vmx_ept_vpid_cap_msr & 0x0000000000000100);
    auto write_back = (ia32_vmx_ept_vpid_cap_msr & 0x4000000000000000);

    if ((eptp & 0x0000000000000007) == 0 && uncacheable == 0)
        throw vmcs_invalid_field("hardware does not support ept memory type: "
                                 "uncachable", ia32_vmx_ept_vpid_cap_msr);

    if ((eptp & 0x0000000000000007) == 6 && write_back == 0)
        throw vmcs_invalid_field("hardware does not support ept memory type: "
                                 "write-back", ia32_vmx_ept_vpid_cap_msr);

    if ((eptp & 0x0000000000000007) != 0 && (eptp & 0x0000000000000007) != 6)
        throw vmcs_invalid_field("unknown eptp memory type", eptp);

    if ((eptp & 0x0000000000000038) >> 3 != 3)
        throw vmcs_invalid_field("the ept walk-through length must 1 less "
                                 "than 4, i.e. 3", eptp);

    auto dirty_accessed = (ia32_vmx_ept_vpid_cap_msr & 0x0000000000200000);

    if ((eptp & 0x0000000000000040) != 0 && dirty_accessed == 0)
        throw vmcs_invalid_field("hardware does not support dirty / "
                                 "accessed flags for ept",
                                 ia32_vmx_ept_vpid_cap_msr);

    if ((eptp & 0xFFFF000000000000) != 0 || (eptp & 0x0000000000000F80) != 0)
        throw vmcs_invalid_field("bits 11:7 and 63:48 must be 0", eptp);
}

void
vmcs_intel_x64::check_control_unrestricted_guests()
{
    auto ctls = get_proc2_ctls();

    if (check_is_unrestricted_enabled() == false)
        return;

    if pin_disabled(ctls, VM_EXEC_S_PROC_BASED_ENABLE_EPT)
        throw vmcs_invalid_field("enable ept must be 1 "
                                 "if unrestricted guest is 1", ctls);
}

void
vmcs_intel_x64::check_control_enable_vm_functions()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_ENABLE_VM_FUNCTIONS)
        return;

    auto vmcs_vm_function_controls =
        vmread(VMCS_VM_FUNCTION_CONTROLS_FULL);

    auto ia32_vmx_vmfunc_msr =
        m_intrinsics->read_msr(IA32_VMX_VMFUNC_MSR);

    if ((~ia32_vmx_vmfunc_msr & vmcs_vm_function_controls) != 0)
        throw vmcs_invalid_field("unsupported vm function control bit "
                                 "set", ia32_vmx_vmfunc_msr);

    if ((VM_FUNCTION_CONTROL_EPTP_SWITCHING & vmcs_vm_function_controls) == 0)
        return;

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_ENABLE_EPT)
        throw vmcs_invalid_field(
            "enable ept must be 1 if eptp switching is 1", ctls);

    auto eptp_list = vmread(VMCS_EPTP_LIST_ADDRESS_FULL);

    if ((eptp_list & 0x0000000000000FFF) != 0)
        throw vmcs_invalid_field(
            "bits 11:0 must be 0 for eptp list address", eptp_list);

    if (check_has_valid_address_width(eptp_list) == false)
        throw invalid_address("eptp list address addr too large", eptp_list);
}

void
vmcs_intel_x64::check_control_enable_vmcs_shadowing()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_VMCS_SHADOWING)
        return;

    auto vmcs_vmread_bitmap_address =
        vmread(VMCS_VMREAD_BITMAP_ADDRESS_FULL);

    auto vmcs_vmwrite_bitmap_address =
        vmread(VMCS_VMWRITE_BITMAP_ADDRESS_FULL);

    if ((vmcs_vmread_bitmap_address & 0x0000000000000FFF) != 0)
        throw vmcs_invalid_field("bits 11:0 must be 0 for the vmcs "
                                 "read bitmap address",
                                 vmcs_vmread_bitmap_address);

    if ((vmcs_vmwrite_bitmap_address & 0x0000000000000FFF) != 0)
        throw vmcs_invalid_field("bits 11:0 must be 0 for the vmcs "
                                 "write bitmap address",
                                 vmcs_vmwrite_bitmap_address);

    if (check_has_valid_address_width(vmcs_vmread_bitmap_address) == false)
        throw invalid_address("vmcs read bitmap address addr too "
                              "large", vmcs_vmread_bitmap_address);

    if (check_has_valid_address_width(vmcs_vmwrite_bitmap_address) == false)
        throw invalid_address("vmcs write bitmap address addr too "
                              "large", vmcs_vmwrite_bitmap_address);
}

void
vmcs_intel_x64::check_control_enable_ept_violation_checks()
{
    auto ctls = get_proc2_ctls();

    if proc2_disabled(ctls, VM_EXEC_S_PROC_BASED_EPT_VIOLATION_VE)
        return;

    auto vmcs_virt_except_info_address =
        vmread(VMCS_VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS_FULL);

    if ((vmcs_virt_except_info_address & 0x0000000000000FFF) != 0)
        throw vmcs_invalid_field("bits 11:0 must be 0 for the vmcs "
                                 "virt except info address",
                                 vmcs_virt_except_info_address);

    if (check_has_valid_address_width(vmcs_virt_except_info_address) == false)
        throw invalid_address("vmcs virt except info address addr too "
                              "large", vmcs_virt_except_info_address);
}

void
vmcs_intel_x64::checks_on_vm_exit_control_fields()
{
    check_control_vm_exit_ctls_reserved_properly_set();
    check_control_activate_and_save_premeption_timer_must_be_0();
    check_control_exit_msr_store_address();
    check_control_exit_msr_load_address();
}

void
vmcs_intel_x64::check_control_vm_exit_ctls_reserved_properly_set()
{
    auto ia32_vmx_exit_ctls_msr =
        m_intrinsics->read_msr(IA32_VMX_EXIT_CTLS_MSR);

    auto lower = ((ia32_vmx_exit_ctls_msr >> 00) & 0x00000000FFFFFFFF);
    auto upper = ((ia32_vmx_exit_ctls_msr >> 32) & 0x00000000FFFFFFFF);

    auto ctls = get_exit_ctls();
    auto ctls_lower = ((ctls >> 00) & 0x00000000FFFFFFFF);
    auto ctls_upper = ((ctls >> 32) & 0x00000000FFFFFFFF);

    if ((lower & ctls_lower) != lower || (upper & ~ctls_upper) != upper)
        throw vmcs_invalid_ctls("exit", lower, upper,
                                ctls_lower, ctls_upper);
}

void
vmcs_intel_x64::check_control_activate_and_save_premeption_timer_must_be_0()
{
    auto ctls1 = get_pin_ctls();
    auto ctls2 = get_exit_ctls();

    if pin_enabled(ctls1, VM_EXEC_PIN_BASED_ACTIVATE_VMX_PREEMPTION_TIMER)
        return;

    if pin_enabled(ctls2, VM_EXIT_CONTROL_SAVE_VMX_PREEMPTION_TIMER_VALUE)
        throw vmcs_invalid_field("save vmx preemption timer must be 0 "
                                 "if the activate vmx preemption timer is 0",
                                 ctls1);
}

void
vmcs_intel_x64::check_control_exit_msr_store_address()
{
    auto msr_store_count = vmread(VMCS_VM_EXIT_MSR_STORE_COUNT);

    if (msr_store_count == 0)
        return;

    auto msr_store_addr = vmread(VMCS_VM_EXIT_MSR_STORE_ADDRESS_FULL);

    if ((msr_store_addr & 0x000000000000000F) != 0)
        throw vmcs_invalid_field("bits 3:0 must be 0 for the "
                                 "exit msr store address", msr_store_addr);

    if (check_has_valid_address_width(msr_store_addr) == false)
        throw invalid_address("exit msr store addr too "
                              "large", msr_store_addr);

    auto msr_store_addr_end = msr_store_addr + (msr_store_count * 16) - 1;

    if (check_has_valid_address_width(msr_store_addr_end) == false)
        throw invalid_address("end of exit msr store area too "
                              "large", msr_store_addr_end);
}

void
vmcs_intel_x64::check_control_exit_msr_load_address()
{
    auto msr_load_count = vmread(VMCS_VM_EXIT_MSR_LOAD_COUNT);

    if (msr_load_count == 0)
        return;

    auto msr_load_addr = vmread(VMCS_VM_EXIT_MSR_LOAD_ADDRESS_FULL);

    if ((msr_load_addr & 0x000000000000000F) != 0)
        throw vmcs_invalid_field("bits 3:0 must be 0 for the "
                                 "exit msr load address", msr_load_addr);

    if (check_has_valid_address_width(msr_load_addr) == false)
        throw invalid_address("exit msr load addr too "
                              "large", msr_load_addr);

    auto msr_load_addr_end = msr_load_addr + (msr_load_count * 16) - 1;

    if (check_has_valid_address_width(msr_load_addr_end) == false)
        throw invalid_address("end of exit msr load area too "
                              "large", msr_load_addr_end);
}

void
vmcs_intel_x64::checks_on_vm_entry_control_fields()
{
    check_control_vm_entry_ctls_reserved_properly_set();
    check_control_event_injection_type_vector_checks();
    check_control_event_injection_delivery_ec_checks();
    check_control_event_injection_reserved_bits_checks();
    check_control_event_injection_ec_checks();
    check_control_event_injection_instr_length_checks();
    check_control_entry_msr_load_address();
}

void
vmcs_intel_x64::check_control_vm_entry_ctls_reserved_properly_set()
{
    auto ia32_vmx_entry_ctls_msr =
        m_intrinsics->read_msr(IA32_VMX_ENTRY_CTLS_MSR);

    auto lower = ((ia32_vmx_entry_ctls_msr >> 00) & 0x00000000FFFFFFFF);
    auto upper = ((ia32_vmx_entry_ctls_msr >> 32) & 0x00000000FFFFFFFF);

    auto ctls = get_entry_ctls();
    auto ctls_lower = ((ctls >> 00) & 0x00000000FFFFFFFF);
    auto ctls_upper = ((ctls >> 32) & 0x00000000FFFFFFFF);

    if ((lower & ctls_lower) != lower || (upper & ~ctls_upper) != upper)
        throw vmcs_invalid_ctls("entry", lower, upper,
                                ctls_lower, ctls_upper);
}

void
vmcs_intel_x64::check_control_event_injection_type_vector_checks()
{
    auto interrupt_info_field =
        vmread(VMCS_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD);

    if ((interrupt_info_field & 0x0000000080000000) == 0)
        return;

    auto type = (interrupt_info_field & 0x0000000000000700) >> 8;

    if (type == 1)
        throw vmcs_invalid_field("interrupt information field type of 1 "
                                 "is reserved", interrupt_info_field);

    if (supports_monitor_trap_flag() == false && type == 7)
        throw vmcs_invalid_field("interrupt information field type of 7 "
                                 "is reserved on this hardware",
                                 interrupt_info_field);

    auto vector = interrupt_info_field & 0x00000000000000FF;

    if (type == 2 && vector != 2)
        throw vmcs_invalid_field("interrupt information field vector must be "
                                 "2 if the type field is 2 (NMI)",
                                 interrupt_info_field);

    if (type == 3 && vector > 31)
        throw vmcs_invalid_field("interrupt information field vector must be "
                                 "0->31 if the type field is 3 (HE)",
                                 interrupt_info_field);

    if (type == 7 && vector != 0)
        throw vmcs_invalid_field("interrupt information field vector must be "
                                 "0 if the type field is 7 (other)",
                                 interrupt_info_field);
}

void
vmcs_intel_x64::check_control_event_injection_delivery_ec_checks()
{
    auto interrupt_info_field =
        vmread(VMCS_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD);

    if ((interrupt_info_field & 0x0000000080000000) == 0)
        return;

    if ((interrupt_info_field & 0x0000000000000800) == 0)
        return;

    auto cr0 = vmread(VMCS_GUEST_CR0);

    auto type = (interrupt_info_field & 0x0000000000000700) >> 8;
    auto vector = (interrupt_info_field & 0x00000000000000FF) >> 0;

    if (check_is_unrestricted_enabled() == true)
    {
        if ((cr0 & CRO_PE_PROTECTION_ENABLE) == 0)
            throw vmcs_invalid_field("unrestricted guest must be 0 or PE must "
                                     "be enabled in cr0 if deliver error code "
                                     "bit is set", interrupt_info_field);
    }

    if (type != 3)
        throw vmcs_invalid_field("interrupt information field type must be "
                                 "3 if deliver error code bit is set",
                                 interrupt_info_field);

    switch (vector)
    {
        case 8:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 17:
            break;

        default:
            throw vmcs_invalid_field("vector must indicate exception that "
                                     "would nomrally deliver an error code if "
                                     "deliver error code bit is set",
                                     interrupt_info_field);
    }
}

void
vmcs_intel_x64::check_control_event_injection_reserved_bits_checks()
{
    auto interrupt_info_field =
        vmread(VMCS_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD);

    if ((interrupt_info_field & 0x0000000080000000) == 0)
        return;

    if ((interrupt_info_field & 0x000000007FFFF000) != 0)
        throw vmcs_invalid_field("reserved bits of the interrupt info "
                                 "field must be 0", interrupt_info_field);
}

void
vmcs_intel_x64::check_control_event_injection_ec_checks()
{
    auto interrupt_info_field =
        vmread(VMCS_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD);

    auto exception_error_code =
        vmread(VMCS_VM_ENTRY_EXCEPTION_ERROR_CODE);

    if ((interrupt_info_field & 0x0000000080000000) == 0)
        return;

    if ((interrupt_info_field & 0x0000000000000800) == 0)
        return;

    if ((exception_error_code & 0x00000000FFFF8000) == 0)
        throw vmcs_invalid_field("bits 31:15 of the exception error code "
                                 "field must be 0 if deliver error code bit "
                                 "is set in the interrupt info field",
                                 exception_error_code);
}

void
vmcs_intel_x64::check_control_event_injection_instr_length_checks()
{
    auto interrupt_info_field =
        vmread(VMCS_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD);

    auto instruction_length =
        vmread(VMCS_VM_ENTRY_INSTRUCTION_LENGTH);

    if ((interrupt_info_field & 0x0000000080000000) == 0)
        return;

    auto type = (interrupt_info_field & 0x0000000000000700) >> 8;

    switch (type)
    {
        case 4:
        case 5:
        case 6:
            break;

        default:
            return;
    }

    if ((instruction_length < 1) || (instruction_length > 15))
        throw vmcs_invalid_field("instruction length must be in the range "
                                 "of 1-15 if type is 4, 5, 6",
                                 instruction_length);
}

void
vmcs_intel_x64::check_control_entry_msr_load_address()
{
    auto msr_load_count = vmread(VMCS_VM_ENTRY_MSR_LOAD_COUNT);

    if (msr_load_count == 0)
        return;

    auto msr_load_addr = vmread(VMCS_VM_ENTRY_MSR_LOAD_ADDRESS_FULL);

    if ((msr_load_addr & 0x000000000000000F) != 0)
        throw vmcs_invalid_field("bits 3:0 must be 0 for the "
                                 "entry msr load address", msr_load_addr);

    if (check_has_valid_address_width(msr_load_addr) == false)
        throw invalid_address("entry msr load addr too "
                              "large", msr_load_addr);

    auto msr_load_addr_end = msr_load_addr + (msr_load_count * 16) - 1;

    if (check_has_valid_address_width(msr_load_addr_end) == false)
        throw invalid_address("end of entry msr load area too "
                              "large", msr_load_addr_end);
}
