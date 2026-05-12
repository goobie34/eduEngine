#include <entt/entt.hpp>
#pragma once

enum Event : std::uint8_t
{
    EVENT0,
    EVENT1,
    EVENT2,
};

class EventListener {
public:
    virtual ~EventListener() {};
    virtual void OnNotify(EventSource& entity, Event event) = 0;
};

class EventSource {
private:
    std::vector<std::weak_ptr<EventListener>> _observers;
    int _numberOfObservers = 0;
protected:
    void Notify(Event event);
public:
    void AddObserver(EventListener* observer);
    void RemoveObserver(EventListener* observer);
};