#pragma once

#include "value.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace eval {

class Context {
public:
    explicit Context(std::shared_ptr<Context> parent = nullptr) : parent_(std::move(parent)) {}

    void set(const std::string& name, const Value& value) { bindings_[name] = value; }

    Value get(const std::string& name) const {
        auto it = bindings_.find(name);
        if (it != bindings_.end()) {
            return it->second;
        }
        if (parent_) {
            return parent_->get(name);
        }
        throw std::runtime_error("Undefined variable: " + name);
    }

    bool has(const std::string& name) const {
        if (bindings_.find(name) != bindings_.end()) {
            return true;
        }
        return parent_ && parent_->has(name);
    }

    std::shared_ptr<Context> parent() const { return parent_; }

private:
    std::unordered_map<std::string, Value> bindings_;
    std::shared_ptr<Context> parent_;
};

} // namespace eval
