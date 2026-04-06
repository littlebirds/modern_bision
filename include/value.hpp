#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "type_table.hpp"

namespace eval { 

// Primitive types
struct Null {};
using Bool = bool;

// Typed pointer - pair of TypeId and raw pointer
using TypedPtr = std::pair<TypeId, void*>;
 

// Value holder - only primitives + object reference 
class Value {
public:
    using ValueType = std::variant<Null, Bool, int64_t, double, TypedPtr>;

    // Constructors for primitives
    Value() : data_(Null{}) {}
    Value(Null) : data_(Null{}) {}
    Value(bool b) : data_(b) {}
    Value(int64_t i) : data_(i) {}
    Value(int i) : data_(static_cast<int64_t>(i)) {}
    Value(double d) : data_(d) {}

    // Constructor for objects (strings, arrays, etc.)
    Value(const TypedPtr& obj) : data_(obj) {} 

    // Type checking
    bool isNull() const { return std::holds_alternative<Null>(data_); }
    bool isBool() const { return std::holds_alternative<Bool>(data_); }
    bool isInt() const { return std::holds_alternative<int64_t>(data_); }
    bool isFloat() const { return std::holds_alternative<double>(data_); }    
    bool isNumber() const { return isInt() || isFloat(); }
    bool isReference() const { return std::holds_alternative<TypedPtr>(data_); } 

    // Value accessors
    bool asBool() const {
        if (!isBool())
            throw std::runtime_error("Value is not a boolean");
        return std::get<Bool>(data_);
    }

    int64_t asInt() const {
        if (!isInt())
            throw std::runtime_error("Value is not an integer");
        return std::get<int64_t>(data_);
    }

    double asFloat() const {
        if (isFloat())
            return std::get<double>(data_);
        if (isInt())
            return static_cast<double>(std::get<int64_t>(data_));
        throw std::runtime_error("Value is not a number");
    }
 
    const TypedPtr& deref()  const {
        if (!isReference())
            throw std::runtime_error("Value is not a reference");
        return std::get<TypedPtr>(data_);
    }

    // String representation
    std::string toString() const;

    // Truthiness for conditionals
    bool isTruthy() const {
        if (isNull())
            return false;
        if (isBool())
            return asBool();
        if (isInt())
            return asInt() != 0;
        if (isFloat())
            return asFloat() != 0.0;
        if (isReference())
            return deref().second != nullptr;
        return false;
    }

    // Equality comparison
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }

private:
    ValueType data_;
};

// Object types enum
enum class ObjectType {
    STRING,
    LIST,
    // Future: FUNCTION, MAP, CLASS_INSTANCE, etc.
};

// Base Object class for heap-allocated values
class Object {
public:
    virtual ~Object() = default;

    virtual ObjectType type() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const Object& other) const = 0;

protected:
    Object() = default;
}; 

// Value::toString implementation
inline std::string Value::toString() const {
    if (isNull())
        return "null";
    if (isBool())
        return asBool() ? "true" : "false";
    if (isInt())
        return std::to_string(asInt());
    if (isFloat()) {
        std::ostringstream ss;
        ss << asFloat();
        return ss.str();
    }
    if (isReference()) {
        const auto& obj = deref();
        return " <TODO: " + TypeTable::instance().getTypeName(obj.first) + "> ";
    }
    return "<unknown>";
}

// Value::operator== implementation
inline bool Value::operator==(const Value& other) const {
    // Both are objects - compare by pointer first, then by value
    if (isReference() && other.isReference()) {
        const auto& a = deref();
        const auto& b = other.deref();
        if (a.first != b.first) {
            return false;
        } 
        if (a.second == b.second) {
            return true;
        }
        // TODO: compare object values
        return false;
    } else {
        // Otherwise compare variant directly
        return data_ == other.data_;
    }
}

} // namespace eval
