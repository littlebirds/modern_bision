#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace eval {

// Lexical scope — a hash map of entries with a parent link.
// Pushed/popped when entering/leaving blocks and functions.
//
// T must be copyable or movable (stored by value in unordered_map).
template <typename T>
class Scope {
public:
    explicit Scope(std::shared_ptr<Scope> parent = nullptr)
        : parent_(std::move(parent)) {}

    // Declare or overwrite a binding in THIS scope (shadows parent).
    void define(const std::string& name, T value) {
        bindings_[name] = std::move(value);
    }

    // Look up a binding by walking up the scope chain.
    // Returns nullptr if not found.
    const T* resolve(const std::string& name) const {
        auto it = bindings_.find(name);
        if (it != bindings_.end()) return &it->second;
        if (parent_) return parent_->resolve(name);
        return nullptr;
    }

    // Mutable lookup — walks up the scope chain.
    // Returns nullptr if not found.
    T* resolveMut(const std::string& name) {
        auto it = bindings_.find(name);
        if (it != bindings_.end()) return &it->second;
        if (parent_) return parent_->resolveMut(name);
        return nullptr;
    }

    // Check if a binding exists in this scope (not parent).
    bool hasLocal(const std::string& name) const {
        return bindings_.find(name) != bindings_.end();
    }

    std::shared_ptr<Scope> parent() const { return parent_; }

private:
    std::unordered_map<std::string, T> bindings_;
    std::shared_ptr<Scope> parent_;
};

} // namespace eval
