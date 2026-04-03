#pragma once

#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace eval {

// Forward declarations
class Object;
class Value;

// Primitive types
struct Null {};
using Bool = bool;

// Object pointer type for heap-allocated values (strings, arrays, functions, etc.)
using ObjectPtr = std::shared_ptr<Object>;

// Value holder - only primitives + object reference
// Size: ~24-32 bytes (variant overhead + ObjectPtr)
class Value {
public:
    using ValueType = std::variant<Null, Bool, int64_t, double, ObjectPtr>;

    // Constructors for primitives
    Value() : data_(Null{}) {}
    Value(Null) : data_(Null{}) {}
    Value(bool b) : data_(b) {}
    Value(int64_t i) : data_(i) {}
    Value(int i) : data_(static_cast<int64_t>(i)) {}
    Value(double d) : data_(d) {}

    // Constructor for objects (strings, arrays, etc.)
    Value(const ObjectPtr& obj) : data_(obj) {}
    Value(ObjectPtr&& obj) : data_(std::move(obj)) {}

    // Type checking
    bool isNull() const { return std::holds_alternative<Null>(data_); }
    bool isBool() const { return std::holds_alternative<Bool>(data_); }
    bool isInt() const { return std::holds_alternative<int64_t>(data_); }
    bool isFloat() const { return std::holds_alternative<double>(data_); }
    bool isObject() const { return std::holds_alternative<ObjectPtr>(data_); }

    // Type helpers
    bool isNumber() const { return isInt() || isFloat(); }

    // Value accessors
    bool asBool() const {
        if (!isBool()) throw std::runtime_error("Value is not a boolean");
        return std::get<Bool>(data_);
    }

    int64_t asInt() const {
        if (!isInt()) throw std::runtime_error("Value is not an integer");
        return std::get<int64_t>(data_);
    }

    double asFloat() const {
        if (isFloat()) return std::get<double>(data_);
        if (isInt()) return static_cast<double>(std::get<int64_t>(data_));
        throw std::runtime_error("Value is not a number");
    }

    const ObjectPtr& asObject() const {
        if (!isObject()) throw std::runtime_error("Value is not an object");
        return std::get<ObjectPtr>(data_);
    }

    // String representation
    std::string toString() const;

    // Truthiness for conditionals
    bool isTruthy() const {
        if (isNull()) return false;
        if (isBool()) return asBool();
        if (isInt()) return asInt() != 0;
        if (isFloat()) return asFloat() != 0.0;
        if (isObject()) return asObject() != nullptr;
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

// String object
class StringObject : public Object {
public:
    StringObject() = default;
    StringObject(std::string value) : value_(std::move(value)) {}
    StringObject(const char* s) : value_(s) {}

    ObjectType type() const override { return ObjectType::STRING; }

    const std::string& value() const { return value_; }
    std::string& value() { return value_; }

    std::string toString() const override {
        return "\"" + value_ + "\"";
    }

    bool equals(const Object& other) const override {
        if (other.type() != ObjectType::STRING) return false;
        return value_ == static_cast<const StringObject&>(other).value_;
    }

    size_t size() const { return value_.size(); }

private:
    std::string value_;
};

// List object
class ListObject : public Object {
public:
    using Elements = std::vector<Value>;

    ListObject() = default;
    ListObject(Elements elements) : elements_(std::move(elements)) {}

    ObjectType type() const override { return ObjectType::LIST; }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < elements_.size(); ++i) {
            ss << elements_[i].toString();
            if (i < elements_.size() - 1) ss << ", ";
        }
        ss << "]";
        return ss.str();
    }

    bool equals(const Object& other) const override {
        if (other.type() != ObjectType::LIST) return false;
        return elements_ == static_cast<const ListObject&>(other).elements_;
    }

    // Element access
    const Elements& elements() const { return elements_; }
    Elements& elements() { return elements_; }

    size_t size() const { return elements_.size(); }

    const Value& at(size_t index) const {
        if (index >= elements_.size()) {
            throw std::out_of_range("List index out of bounds");
        }
        return elements_[index];
    }

    Value& at(size_t index) {
        if (index >= elements_.size()) {
            throw std::out_of_range("List index out of bounds");
        }
        return elements_[index];
    }

    void append(const Value& value) { elements_.push_back(value); }
    void append(Value&& value) { elements_.push_back(std::move(value)); }

private:
    Elements elements_;
};

// Value::toString implementation
inline std::string Value::toString() const {
    if (isNull()) return "null";
    if (isBool()) return asBool() ? "true" : "false";
    if (isInt()) return std::to_string(asInt());
    if (isFloat()) {
        std::ostringstream ss;
        ss << asFloat();
        return ss.str();
    }
    if (isObject()) {
        const auto& obj = asObject();
        return obj ? obj->toString() : "null";
    }
    return "<unknown>";
}

// Value::operator== implementation
inline bool Value::operator==(const Value& other) const {
    // Both are objects - compare by pointer first, then by value
    if (isObject() && other.isObject()) {
        const auto& a = asObject();
        const auto& b = other.asObject();
        if (!a && !b) return true;
        if (!a || !b) return false;
        return a->equals(*b);
    }
    // Otherwise compare variant directly
    return data_ == other.data_;
}

} // namespace eval
