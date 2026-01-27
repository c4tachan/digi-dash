#include "digidash/pid_binding_system.h"

namespace digidash {

PIDBindingSystem::PIDBindingSystem() {}

PIDBindingSystem::~PIDBindingSystem() {}

void PIDBindingSystem::register_binding(const PIDBinding& binding) {
    bindings_[binding.pid_id] = binding;
    current_values_[binding.pid_id] = binding.min_value;
}

void PIDBindingSystem::set_pid_value(uint32_t pid_id, float raw_value) {
    auto it = bindings_.find(pid_id);
    if (it == bindings_.end()) {
        return;
    }
    
    const auto& binding = it->second;
    float scaled = raw_value * binding.scale + binding.offset;
    
    // Clamp to valid range
    if (scaled < binding.min_value) scaled = binding.min_value;
    if (scaled > binding.max_value) scaled = binding.max_value;
    
    current_values_[pid_id] = scaled;
}

float PIDBindingSystem::get_pid_value(uint32_t pid_id) const {
    auto it = current_values_.find(pid_id);
    if (it == current_values_.end()) {
        return 0.0f;
    }
    return it->second;
}

std::string PIDBindingSystem::format_value(uint32_t pid_id) const {
    // TODO: Implement value formatting with units
    return std::to_string(get_pid_value(pid_id));
}

} // namespace digidash
