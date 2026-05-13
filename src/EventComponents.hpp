#include <entt/entt.hpp>
#include <functional>
#pragma once

struct Event {
    Event(std::uint8_t type, float timeStamp, std::string message = "", int data_int = 0, float data_float = 0.0f, entt::entity entity = entt::null)
        : type(type), timeStamp(timeStamp), message(message), entity(entity), data_int(data_int), data_float(data_float) {}
    
    std::uint8_t type; //can be mapped onto enum
    float timeStamp;
    std::string message;
    entt::entity entity;
    int data_int;
    float data_float;
};

struct ObserverComponent {
    std::function<void(Event)> OnNotify;
};

struct SourceComponent {
    std::uint8_t capacity;
    std::vector<Event> events;
    std::vector<entt::entity> observers;
    std::vector<entt::entity> observersToAdd;
    std::vector<entt::entity> observersToRemove;
    void AddEvent(Event event) { if (events.size() < capacity) events.push_back(event); }
    void AddObserver(entt::entity observer) { if (observersToAdd.size() < capacity) observersToAdd.push_back(observer); }
    void RemoveObserver(entt::entity observer) { if (observersToRemove.size() < capacity) observersToRemove.push_back(observer); }
};

struct EventQueueComponent {
    std::uint8_t capacity = 255;
    float delay = 100;
    float time_since_broadcast;
    std::vector<Event> queue;
    void EnqueueEvent(Event event) { if (queue.size() < capacity) queue.push_back(event);}
};