#pragma once

// Type table for managing type metadata
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <atomic>

namespace eval {

// Type ID - unique identifier for each type
using TypeId = size_t;

// Invalid/unknown type marker
constexpr TypeId TYPE_UNKNOWN = 0;

// Built-in type IDs
constexpr TypeId TYPE_NULL = 1;
constexpr TypeId TYPE_STRING = 2;
constexpr TypeId TYPE_BOOL = 3;
constexpr TypeId TYPE_FLOAT = 4;
constexpr TypeId TYPE_INT = 6;

// User-defined types start from 100

// Type category
enum class TypeCategory {
    // null, bool, int, float
    PRIMITIVE,

    // built-in types other than primitives
    STRING,
    ARRAY,    
    DICTIONARY,
    FUNCTION,

    // composite types
    OBJECT,
};

// Type metadata
struct BuiltinType {
    TypeId id;
    std::string name; 
    size_t size; // Size in bytes for Value representation 

    BuiltinType() : id(TYPE_UNKNOWN), name("unknown"), size(0) 
    {}
    BuiltinType(TypeId tid, std::string tname, size_t sz)
        : id(tid), name(std::move(tname)), size(sz)  
    {}
};

struct ArrayType {
    TypeId element_tid;
    std::size_t length;  

    ArrayType() : element_tid(TYPE_UNKNOWN), length(0) 
    {}
    ArrayType(TypeId elementTid, std::size_t len)
        : element_tid(elementTid), length(len)  
    {}
};

// Type table - singleton for type registration and lookup
class TypeTable {
public:
    static TypeTable& instance() {
        static TypeTable table;
        return table;
    }
 
    // Look up type by name
    const TypeCategory getTypeCategory(TypeId tid) {
        if (tid == TYPE_STRING) {
            return TypeCategory::STRING;
        } else if (tid < basicTypes_.size() && basicTypes_[tid].id != TYPE_UNKNOWN) {
            return TypeCategory::PRIMITIVE;
        }
        if (this->arrayTypes_.count(tid) > 0) {
            return TypeCategory::ARRAY;
        }
        throw std::runtime_error("Type category not implemented yet nor tid does not exist");
    }

    TypeId getArrayTypeId(TypeId elementTid, size_t length) {
        for (const auto & [key, val] : arrayTypes_) {
            if (val.element_tid == elementTid && val.length == length) {
                return key;
            } 
        }
        TypeId newId = nextTypeId_ ++;
        arrayTypes_[newId] = ArrayType(elementTid, length);
        return newId;
    }

    std::string getTypeName(TypeId tid) const {
        if (tid < basicTypes_.size()) {
            return basicTypes_[tid].name;
        }
        if (arrayTypes_.count(tid) > 0) {
            const auto& arrayType = arrayTypes_.at(tid);
            return getTypeName(arrayType.element_tid) + "[" + std::to_string(arrayType.length) + "]";
        }
        return "unknown";
    }

    // Prevent copying
    TypeTable(const TypeTable&) = delete;
    TypeTable& operator=(const TypeTable&) = delete;

private:
    TypeTable() : nextTypeId_(100) { // User types start at 100
        // Register built-in types
        registerBuiltinTypes();
    }

    void registerBuiltinTypes() {
        // Primitive types
        basicTypes_[TYPE_NULL] = BuiltinType(TYPE_NULL, "null", 0);
        basicTypes_[TYPE_BOOL] = BuiltinType(TYPE_BOOL, "bool", sizeof(bool));
        basicTypes_[TYPE_INT] = BuiltinType(TYPE_INT, "int", sizeof(int64_t));
        basicTypes_[TYPE_FLOAT] = BuiltinType(TYPE_FLOAT, "float", sizeof(double));
        // Object types
        basicTypes_[TYPE_STRING] = BuiltinType(TYPE_STRING, "string", sizeof(void*));
    }

    std::array<BuiltinType, 16> basicTypes_;
    std::unordered_map<TypeId, ArrayType> arrayTypes_;
     
    std::atomic<TypeId> nextTypeId_;
};



} // namespace eval
