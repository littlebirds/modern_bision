#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace eval {

// Type ID - unique identifier for each type
using TypeId = size_t;

// Invalid/unknown type marker
constexpr TypeId TYPE_UNKNOWN = 0;

// Built-in type IDs
constexpr TypeId TYPE_NULL = 1;
constexpr TypeId TYPE_BOOL = 2;
constexpr TypeId TYPE_INT = 3;
constexpr TypeId TYPE_FLOAT = 4;
constexpr TypeId TYPE_STRING = 5;
// User-defined types start from 100

// Type category
enum class TypeCategory {
    PRIMITIVE,   // null, bool, int, float
    OBJECT,      // heap-allocated: string, list, function, map
    USER_DEFINED // user-defined types
};

// Type metadata
struct TypeInfo {
    TypeId id;
    std::string name;
    TypeCategory category;
    size_t size; // Size in bytes for Value representation
    std::string description;

    TypeInfo() : id(TYPE_UNKNOWN), name("unknown"), category(TypeCategory::PRIMITIVE), size(0) {}
    TypeInfo(TypeId tid, std::string tname, TypeCategory cat, size_t sz, std::string desc = "")
        : id(tid), name(std::move(tname)), category(cat), size(sz), description(std::move(desc)) {}
};

// Type table - singleton for type registration and lookup
class TypeTable {
public:
    static TypeTable& instance() {
        static TypeTable table;
        return table;
    }

    // Register a new type, returns its TypeId
    TypeId registerType(const std::string& name, TypeCategory category, size_t size,
                        const std::string& description = "") {
        TypeId id = nextTypeId_++;
        TypeInfo info(id, name, category, size, description);
        types_[id] = info;
        nameToId_[name] = id;
        return id;
    }

    // Look up type by ID
    const TypeInfo* getType(TypeId id) const {
        auto it = types_.find(id);
        return it != types_.end() ? &it->second : nullptr;
    }

    // Look up type by name
    const TypeInfo* getType(const std::string& name) const {
        auto it = nameToId_.find(name);
        return it != nameToId_.end() ? getType(it->second) : nullptr;
    }

    // Check if type exists
    bool hasType(TypeId id) const { return types_.find(id) != types_.end(); }

    bool hasType(const std::string& name) const { return nameToId_.find(name) != nameToId_.end(); }

    // Get type name by ID
    std::string getTypeName(TypeId id) const {
        const TypeInfo* info = getType(id);
        return info ? info->name : "unknown";
    }

    // Get all registered types
    std::vector<TypeInfo> getAllTypes() const {
        std::vector<TypeInfo> result;
        for (const auto& pair : types_) {
            result.push_back(pair.second);
        }
        return result;
    }

    // Get types by category
    std::vector<TypeInfo> getTypesByCategory(TypeCategory category) const {
        std::vector<TypeInfo> result;
        for (const auto& pair : types_) {
            if (pair.second.category == category) {
                result.push_back(pair.second);
            }
        }
        return result;
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
        types_[TYPE_NULL] = TypeInfo(TYPE_NULL, "null", TypeCategory::PRIMITIVE, 0, "Null/nil value");
        types_[TYPE_BOOL] = TypeInfo(TYPE_BOOL, "bool", TypeCategory::PRIMITIVE, sizeof(bool), "Boolean value");
        types_[TYPE_INT] = TypeInfo(TYPE_INT, "int", TypeCategory::PRIMITIVE, sizeof(int64_t), "64-bit signed integer");
        types_[TYPE_FLOAT] =
            TypeInfo(TYPE_FLOAT, "float", TypeCategory::PRIMITIVE, sizeof(double), "64-bit floating point");

        // Object types
        types_[TYPE_STRING] =
            TypeInfo(TYPE_STRING, "string", TypeCategory::OBJECT, sizeof(void*), "Heap-allocated string");

        // Populate name lookup
        nameToId_["null"] = TYPE_NULL;
        nameToId_["bool"] = TYPE_BOOL;
        nameToId_["int"] = TYPE_INT;
        nameToId_["float"] = TYPE_FLOAT;
        nameToId_["string"] = TYPE_STRING;
    }

    std::unordered_map<TypeId, TypeInfo> types_;
    std::unordered_map<std::string, TypeId> nameToId_;
    TypeId nextTypeId_;
};

// Helper macros for type registration
#define REGISTER_TYPE(name, category, size, desc) eval::TypeTable::instance().registerType(name, category, size, desc)

// Type checking helpers
inline bool isPrimitiveType(TypeId id) {
    const TypeInfo* info = TypeTable::instance().getType(id);
    return info && info->category == TypeCategory::PRIMITIVE;
}

inline bool isObjectType(TypeId id) {
    const TypeInfo* info = TypeTable::instance().getType(id);
    return info && info->category == TypeCategory::OBJECT;
}

inline bool isUserDefinedType(TypeId id) {
    const TypeInfo* info = TypeTable::instance().getType(id);
    return info && info->category == TypeCategory::USER_DEFINED;
}

} // namespace eval
