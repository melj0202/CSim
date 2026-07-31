#pragma once
#include <vector>
#include <cstdint>
#include "Foundation/MathTypes.h"

// Lightweight scene-graph node id (not an ECS entity; see archive/dead-engine for EntityTable).
using ObjectID = uint32_t;

struct SceneObject {
	SceneObject* parent = nullptr;
	std::vector<SceneObject*> children;
	Matrix4 transform = Matrix4(1.0f);
	ObjectID id;

	SceneObject() : id(0) {}
	explicit SceneObject(ObjectID objectId) : id(objectId) {}
	~SceneObject() = default;

	void AddChild(SceneObject* child)
	{
		children.push_back(child);
	}

	void RemoveChild(SceneObject* child)
	{
		for (std::vector<SceneObject*>::iterator it = children.begin(); it != children.end(); ++it)
		{
			if (*it == child)
			{
				children.erase(it);
				break;
			}
		}
	}

	void ClearChildren()
	{
		children.clear();
	}

	void SetParent(SceneObject* newParent)
	{
		if (newParent != this->parent)
		{
			if (this->parent)
			{
				this->parent->RemoveChild(this);
			}
			this->parent = newParent;
			if (newParent)
			{
				newParent->AddChild(this);
			}
		}
	}

	void RemoveParent()
	{
		if (parent)
		{
			parent->RemoveChild(this);
			parent = nullptr;
		}
	}

	ObjectID GetID() const { return id; }

	SceneObject* operator[](ObjectID searchId)
	{
		for (SceneObject* child : children)
		{
			if (child->id == searchId)
			{
				return child;
			}
			SceneObject* result = (*child)[searchId];
			if (result)
			{
				return result;
			}
		}
		return nullptr;
	}
};
