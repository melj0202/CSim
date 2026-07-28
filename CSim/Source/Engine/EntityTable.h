#pragma once 
#include <vector> 
#include <unordered_map> 
#include <stdexcept>
#include "Engine/ModuleObject.h" 

class EntityTable {
    std::vector<ModuleObject> objects;
    std::unordered_map<ObjectID, size_t> lookup;
    std::unordered_map<ObjectID, DirtyFlags> dirty;
    ObjectID nextID;

public:
    EntityTable() : nextID(0) {}

    ObjectID CreateEntity() {
        ObjectID id = nextID++;
        objects.push_back(ModuleObject{ id });
        lookup[id] = objects.size() - 1;
        dirty[id] = DirtyFlags::None;
        return id;
    }

    void Remove(ObjectID id) {
        auto it = lookup.find(id);
        if (it == lookup.end()) return; // Fixed: Changed 'return nullptr' to 'return'

        size_t index = it->second;
        size_t lastIndex = objects.size() - 1;
        
        // Fixed: Only update the map if we aren't already removing the very last element
        if (index != lastIndex) {
            ObjectID lastID = objects[lastIndex].id;
            std::swap(objects[index], objects[lastIndex]);
            lookup[lastID] = index;
        }

        objects.pop_back();
        lookup.erase(it); // Optimized: Erase using the iterator directly
        dirty.erase(id);
    }

    void Update(ObjectID id, ModuleObject object) {
        object.id = id;
        auto it = lookup.find(id);
        if (it == lookup.end()) return;

        objects[it->second] = object; // Optimized: Used iterator instead of a second lookup
        dirty[id] = DirtyFlags::None;
    }

    ModuleObject& Get(ObjectID id) {
        auto it = lookup.find(id);
        if (it == lookup.end()) throw std::runtime_error("Invalid ObjectID");
        return objects[it->second];
    }

    DirtyFlags GetDirtyFlags(ObjectID id) {
        auto it = dirty.find(id);
        if (it == dirty.end()) return DirtyFlags::None;
        return it->second;
    }

    void SetDirtyFlags(ObjectID id, DirtyFlags flags) {
        dirty[id] = flags;
    }
};
