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
struct Null {
    bool operator==(const Null&) const = default;
};
using Bool = bool;

// Forward declaration so Value can reference Object
class Object;

// Value holder - only primitives + object reference 
class Value {
public:
    using ValueType = std::variant<Null, Bool, int64_t, double, std::shared_ptr<Object>>;

    // Constructors for primitives
    Value() : data_(Null{}) {}
    Value(Null) : data_(Null{}) {}
    Value(bool b) : data_(b) {}
    Value(int64_t i) : data_(i) {}
    Value(int i) : data_(static_cast<int64_t>(i)) {}
    Value(double d) : data_(d) {}

    // Constructor for objects (strings, arrays, etc.)
    Value(std::shared_ptr<Object> obj) : data_(std::move(obj)) {}

    // Type checking
    bool isNull() const { return std::holds_alternative<Null>(data_); }
    bool isBool() const { return std::holds_alternative<Bool>(data_); }
    bool isInt() const { return std::holds_alternative<int64_t>(data_); }
    bool isFloat() const { return std::holds_alternative<double>(data_); }    
    bool isNumber() const { return isInt() || isFloat(); }
    bool isReference() const { return std::holds_alternative<std::shared_ptr<Object>>(data_); }

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
 
    const std::shared_ptr<Object>& deref() const {
        if (!isReference())
            throw std::runtime_error("Value is not a reference");
        return std::get<std::shared_ptr<Object>>(data_);
    }

    // Data type
    TypeId typeId() const;

    // String representation
    std::string toString() const;

    // Truthiness for conditionals
    bool isTruthy() const;

    // Access the owned object, cast to a concrete type
    template <typename T>
    const T* as() const;

    // Equality comparison
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }

private:
    ValueType data_;
};

// Base Object class for heap-allocated values
class Object {
public:
    virtual ~Object() = default;

    virtual TypeId getTypeId() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const Object& other) const = 0;

protected:
    Object() = default;
};

// String object for heap-allocated strings
class StringObject : public Object {
public:
    explicit StringObject(std::string value) : value_(std::move(value)) {}

    TypeId getTypeId() const override { return TYPE_STRING; }
    std::string toString() const override { return value_; }
    bool equals(const Object& other) const override {
        if (other.getTypeId() != this->getTypeId()) {
            return false;
        }
        const auto& str_other = dynamic_cast<const StringObject&>(other);
        return value_ == str_other.value_;
    }

    const std::string& value() const { return value_; }

private:
    std::string value_;
};

// Array object for heap-allocated homogeneous arrays.
class ArrayObject : public Object {
public:
    ArrayObject(std::vector<Value>&& elements, TypeId elementTypeId)
        : elements_(std::move(elements)),
          elementTypeId_(elementTypeId),
          myTid_(TypeTable::instance().getArrayTypeId(elementTypeId, elements_.size())) 
        {}

    TypeId getTypeId() const override { return this->myTid_; }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < elements_.size(); ++i) {
            ss << elements_[i].toString();
            if (i + 1 < elements_.size()) {
                ss << ", ";
            }
        }
        ss << "]";
        return ss.str();
    }

    bool equals(const Object& other) const override {
        if (other.getTypeId() != this->getTypeId()) {
            return false;
        }
        const auto& array_other = dynamic_cast<const ArrayObject&>(other);
        return elements_ == array_other.elements_;
    }

    const std::vector<Value>& elements() const { return elements_; }
    size_t size() const { return elements_.size(); }
    TypeId elementTypeId() const { return elementTypeId_; }

private:
    std::vector<Value> elements_;
    TypeId elementTypeId_ = TYPE_UNKNOWN;
    TypeId myTid_ = TYPE_UNKNOWN;
};

inline TypeId Value::typeId() const {
    if (isNull()) return TYPE_NULL;
    if (isBool()) return TYPE_BOOL;
    if (isInt()) return TYPE_INT;
    if (isFloat()) return TYPE_FLOAT;
    if (isReference()) {
        const auto& obj = deref();
        if (!obj)
            return TYPE_UNKNOWN;
        if (dynamic_cast<const StringObject*>(obj.get()) != nullptr)
            return TYPE_STRING;
        return obj->getTypeId();
    }
    return TYPE_UNKNOWN;
}

inline bool Value::isTruthy() const {
    if (isNull())
        return false;
    if (isBool())
        return asBool();
    if (isInt())
        return asInt() != 0;
    if (isFloat())
        return asFloat() != 0.0;
    if (isReference())
        return deref() != nullptr;
    return false;
}

template <typename T>
inline const T* Value::as() const {
    const auto& ptr = deref();
    return dynamic_cast<const T*>(ptr.get());
}

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
        if (obj != nullptr)
            return obj->toString();
        return "<" + TypeTable::instance().getTypeName(TYPE_UNKNOWN) + ">";
    }
    return "<unknown>";
}

// Value::operator== implementation
inline bool Value::operator==(const Value& other) const {
    if (isReference() && other.isReference()) {
        const auto& a = deref();
        const auto& b = other.deref();
        if (!a || !b)
            return a == b;
        if (a->getTypeId() != b->getTypeId())
            return false;
        // Same object (pointer identity)
        if (a == b)
            return true;
        // Delegate to Object's own equality (value semantics)
        return a->equals(*b);
    }
    return data_ == other.data_;
}

} // namespace eval
