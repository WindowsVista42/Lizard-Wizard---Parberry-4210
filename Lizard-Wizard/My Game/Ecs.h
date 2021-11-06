// If you're wondering why this file exists its because me (sean) and ethan were talking and
// accidentally reinvented ecs. It turns out we s couple solid use-cases for an ecs so here it is.
//
// The data for entities is not stored in a global "MetaTable", instead the idea is to define
// all of the Tables in Game.h
//
// The benefit being that I don't have to go through and figure out how the hell to
// wrap up multiple tables into a singleton.
//
// While there are certainly other ways of organizing data, the power of a half-baked ecs where
// we can add and remove functionality by adding and removing the actual data is quite nice.
//
// This would probably be not-the-worst-thing-in-the-world to make thread-safe, considering
// its pretty inherently lock-free.

#ifndef ECS_H
#define ECS_H

#include "Defines.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <ctime>
#include <random>
#include <chrono>
#include <iostream>

class Entity {
    static usize& gen() {
        static usize gen= ~0;
        return gen;
    }

public:
    usize id;

    bool operator ==(const Entity& rhs) const {
        return this->id == rhs.id;
    }

    Entity() {
        gen() += 1;
        id = gen();
    }
};

namespace std {
    template<> struct hash<Entity> {
        size_t operator()(Entity const& e) const {
            size_t h = e.id;
            return h;
        }
    };
}

/// An 'Ecs' data structure that stores a list of entities bounded to components.
/// Entities in this table can be thought of as containing the component, but the benefit
/// is that adding and removing components can be done dynamically at run-time.
template <typename T>
class Table { // Sean

    // Mapping of Handle --> index.
    std::unordered_map<Entity, usize> mapping;

    // Components in a packed vector.
    // This favors iteration performance.
    std::vector<T> components;

    // Store which entity we are a part of.
    // This is required for removal in its current implementation.
    std::vector<Entity> entities;

public:
    /// Add item to the table and automatically generate a new "Entity"
    Entity Add(T t) {
        Entity e = Entity();
        components.push_back(t);
        mapping.insert(std::make_pair(e, components.size() - 1));
        entities.push_back(e);
        return e;
    }

    /// Add an existing "Entity" to the Table.
    void AddExisting(Entity e, T t) {
        components.push_back(t);
        mapping.insert(std::make_pair(e, components.size() - 1));
        entities.push_back(e);
    }

    /// Remove an "Entity" from the Table.
    void Remove(Entity e) {
        // Get the index of the item to be removed
        //if (mapping.count(e) != 1) { std::cout << typeid(T).name() << "Item not found!\n"; exit(1); }
        usize index = mapping[e];

        // Write last item to index
        if (index != entities.size() - 1) {
            components[index] = components[components.size() - 1];
            entities[index] = entities[entities.size() - 1];
        }

        // Resize data
        components.pop_back();
        entities.pop_back();

        // Remove old item
        mapping.erase(e);

        // Remap last item
        if (index != entities.size()) {
            mapping[entities[index]] = index;
        }
    }

    /// Clear the table of all data.
    void Clear() {
        mapping.clear();
        components.clear();
        entities.clear();
    }

    /// Get the component corresponding to the Entity.
    T* Get(Entity e) {
        //if (mapping.count(e) != 1) { std::cout << typeid(*this).name() << " Item not found!\n"; exit(1); }
        return &components[mapping[e]];
    }

    // Checks if a Entity is a member of table.
    b8 Contains(Entity e) {
        if (mapping[e]) {
            return true;
        } else {
            return false;
        }
    }

    /// Get the number of components in the table.
    usize Size() {
        return components.size();
    }

    /// Get a pointer to the start of the components array.
    /// This array is contiguous and bounded by Table.Size().
    /// This array has as 1 to 1 mapping with Entities().
    T* Components() {
        return components.data();
    }

    /// Get a pointer to the start of the entities array.
    /// This array is contiguous and bounded by Table.Size().
    /// This array has a one to one mapping with Components().
    Entity* Entities() {
        return entities.data();
    }

    const type_info T() const noexcept {
        return typeid(T);
    }
};

/// An 'Ecs' data structure that stores a list of entities.
/// Entities in this group can be thought of as being 'tagged' with a specific function.
class Group {
    std::unordered_map<Entity, usize> mapping;
    std::vector<Entity> entities;

public:
    /// Automatically generate a new "Entity" and add it to the group.
    Entity Add() {
        Entity e = Entity();
        entities.push_back(e);
        mapping.insert(std::make_pair(e, entities.size() - 1));
        return e;
    }

    /// Add an existing "Entity" to the group.
    void AddExisting(Entity e) {
        entities.push_back(e);
        mapping.insert(std::make_pair(e, entities.size() - 1));
    }

    /// Remove an existing "Entity" from the group.
    void Remove(Entity e) {
        usize index = mapping[e];

        // Write last item to index
        if (index != entities.size() - 1) {
            entities[index] = entities[entities.size() - 1];
        }

        // Resize data
        entities.pop_back();

        // Remove old item
        mapping.erase(e);

        // Remap last item
        if (index != entities.size()) {
            mapping[entities[index]] = index;
        }
    }

    /// Clear the Group of all data.
    void Clear() {
        entities.clear();
        mapping.clear();
    }

    // Checks if a Entity is a member of group.
    b8 Contains(Entity e) {
        if (mapping[e]) {
            return true;
        }
        else {
            return false;
        }
    }

    /// Get the number of entities in this group.
    usize Size() {
        return entities.size();
    }

    /// Get a pointer to the start of the entities array.
    /// This array is contiguous.
    Entity* Entities() {
        return entities.data();
    }

    Entity RemoveTail() {
        Entity e = entities.back();
        entities.pop_back();
        mapping.erase(e);
        return e;
    }
};

struct Action {
    u32 max_cooldown;
    u32 max_active;

    f32 windup;
    f32 winddown;

    f32 duration;
    Group active;

    f32 cooldown;
    Group timers;
};

class Ecs {
public:
    template <typename A, typename T, typename F>
    static void RemoveConditionally(T& table, Group& group, const F& filter) {
        std::vector<Entity> to_remove;

        for every(index, group.Size()) {
            Entity e = group.Entities()[index];
            A* t = table.Get(e);

            if (filter(t)) {
                to_remove.push_back(e);
            }
        }

        for every(index, to_remove.size()) {
            group.Remove(to_remove[index]);
            table.Remove(to_remove[index]);
        }
    }

    static void ActivateAction(Table<f32>& timers, Action& action) {
        if (action.timers.Size() < action.max_cooldown && action.active.Size() < action.max_active) {
            Entity e = Entity();
            action.active.AddExisting(e);
            timers.AddExisting(e, action.duration);

            e = Entity();
            action.timers.AddExisting(e);
            timers.AddExisting(e, action.cooldown);
        }
    }

    template <typename F>
    static void ApplyEvery(Group& group, F& function) {
        for every(index, group.Size()) {
            function(group.Entities()[index]);
        }
    }
};

#endif
