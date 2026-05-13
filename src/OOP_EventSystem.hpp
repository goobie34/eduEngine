#include <entt/entt.hpp>
#pragma once

enum EventType : std::uint8_t
{
    EVENT0,
    EVENT1,
    EVENT2
};

//contains a variety of fields for various purposes
struct Event {
    Event(EventType type, float timeStamp, std::string message = "", int data_int = 0, float data_float = 0.0f, entt::entity entity = entt::null)
        : type(type), timeStamp(timeStamp), message(message), entity(entity), data_int(data_int), data_float(data_float) {}
    
    EventType type;
    float timeStamp;
    std::string message;
    entt::entity entity;
    int data_int;
    float data_float;
};

//Abstract Observer class
class EventListener {
public:
    virtual ~EventListener() {};
    virtual void OnNotify(entt::entity entity, Event event) = 0;
};

//Source
class EventSource {
private:
    std::vector<std::weak_ptr<EventListener>> m_listeners;
protected:
    void Notify(Event event);
public:
    void AddObserver(EventListener* observer);
    void RemoveObserver(EventListener* observer);
    entt::entity m_entity;
};

class EventQueue : EventSource, EventListener {
private:
    std::uint8_t m_capacity;
    std::vector<Event> m_queue;
public:
    EventQueue(uint8_t capacity = 255) : m_capacity(capacity){};
    void EnqueueEvent(Event);
    void BroadcastAllEvents();
    void OnNotify(EventSource& entity, Event event); //From EventListener base class
};