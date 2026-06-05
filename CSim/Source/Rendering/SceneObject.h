#pragma once 
#include <vector>
#include "Util/Math.h"
#include "System/EntityTable.h"

struct SceneObject {
    SceneObject* parent = nullptr;
    std::vector<SceneObject*> children;
    
    ObjectID id;

    SceneObject(EntityTable* et) {
        //Initalize parent to idenity matrix
        id = et->CreateEntity();
    }
    ~SceneObject() = default;
    void AddChild(SceneObject* child) {
        children.push_back(child);
    }
    void RemoveChild(SceneObject* child) {
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (*it == child) {
                children.erase(it);
                break;
            }
        }
    }
    void ClearChildren() {
        children.clear();
    }
    void SetParent(SceneObject* parent) {
        if (parent != this->parent) {
            if (this->parent) {
                this->parent->RemoveChild(this);
            }
            this->parent = parent;
            parent->AddChild(this);
        }
    }
    void RemoveParent() {
        if (parent) {
            parent->RemoveChild(this);
            parent = nullptr;
        }
    }

    
    ObjectID GetID() const { return id; }

    SceneObject* operator[](ObjectID id) {
        for (auto& child : children) {
            // 1. Check if the current child is the match
            if (child->id == id) {
                return child; 
            }
            // 2. Recursively search the child's subtree using idiomatic syntax
            SceneObject* result = (*child)[id];
            if (result) {
                return result;
            }
        }
        return nullptr; // Assuming you return nullptr if nothing is found
    }

};