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

#ifndef VCPU_INTEL_X64_H
#define VCPU_INTEL_X64_H

#include <vcpu/vcpu.h>
#include <vmxon/vmxon_intel_x64.h>
#include <vmcs/vmcs_intel_x64.h>
#include <vmcs/vmcs_intel_x64_vmm_state.h>
#include <vmcs/vmcs_intel_x64_host_vm_state.h>
#include <exit_handler/exit_handler_intel_x64.h>
#include <intrinsics/intrinsics_intel_x64.h>

/// Virtual CPU (Intel x86_64)
///
/// The Virtual CPU represents a "CPU" to the hypervisor that is specific to
/// Intel x86_64.
///
/// This Intel specific vCPU class provides all of the functionality of the
/// base vCPU, but also adds classes specific to Intel's VT-x including the
/// vmxon_intel_x64, vmcs_intel_x64, exit_handler_intel_x64 and
/// intrinsics_intel_x64 classes.
///
/// Note that these should not be created directly, but instead should be
/// created by the vcpu_manager, which uses the vcpu_factory to actually
/// create a vcpu.
///
class vcpu_intel_x64 : public vcpu
{
public:

    /// Constructor
    ///
    /// Creates a vCPU with the provided resources. This constructor
    /// provides a means to override and repalce the internal resources of the
    /// vCPU. Note that if one of the resources is set to NULL, a default
    /// will be constructed in it's place, providing a means to select which
    /// internal components to override.
    ///
    /// @param id the id of the vcpu
    /// @param debug_ring the debug ring the vcpu should use. If you
    ///     provide nullptr, a default debug ring will be created.
    /// @param vmxon the vmxon the vcpu should use. If you
    ///     provide nullptr, a default vmxon will be created.
    /// @param vmcs the vmcs the vcpu should use. If you
    ///     provide nullptr, a default vmcs will be created.
    /// @param exit_handler the exit handler the vcpu should use. If you
    ///     provide nullptr, a default exit handler will be created.
    /// @param intrinsics the intrinsics the vcpu should use. If you
    ///     provide nullptr, a default intrinsics will be created.
    /// @param vmm_state the vmm state the vcpu should use. If you
    ///     provide nullptr, a default vmm state will be created.
    /// @param guest_state the guest state the vcpu should use. If you
    ///     provide nullptr, a default guest state will be created.
    ///
    vcpu_intel_x64(uint64_t id,
                   std::shared_ptr<debug_ring> debug_ring = nullptr,
                   std::shared_ptr<intrinsics_intel_x64> intrinsics = nullptr,
                   std::shared_ptr<vmxon_intel_x64> vmxon = nullptr,
                   std::shared_ptr<vmcs_intel_x64> vmcs = nullptr,
                   std::shared_ptr<exit_handler_intel_x64> exit_handler = nullptr,
                   std::shared_ptr<vmcs_intel_x64_vmm_state> vmm_state = nullptr,
                   std::shared_ptr<vmcs_intel_x64_vmm_state> guest_state = nullptr);

    /// Destructor
    ///
    ~vcpu_intel_x64() override = default;

    /// Init vCPU
    ///
    /// @see vcpu::init
    ///
    void init(void *attr = nullptr) override;

    /// Fini vCPU
    ///
    /// @see vcpu::fini
    ///
    void fini(void *attr = nullptr) override;

    /// Run vCPU
    ///
    /// @see vcpu::run
    ///
    void run(void *attr = nullptr) override;

    /// Halt vCPU
    ///
    /// @see vcpu::hlt
    ///
    void hlt(void *attr = nullptr) override;

private:

    bool m_vmcs_launched;
    bool m_vmxon_started;

    std::shared_ptr<intrinsics_intel_x64> m_intrinsics;
    std::shared_ptr<vmxon_intel_x64> m_vmxon;
    std::shared_ptr<vmcs_intel_x64> m_vmcs;
    std::shared_ptr<exit_handler_intel_x64> m_exit_handler;

    std::shared_ptr<state_save_intel_x64> m_state_save;

    std::shared_ptr<vmcs_intel_x64_state> m_vmm_state;
    std::shared_ptr<vmcs_intel_x64_state> m_guest_state;
};

#endif
