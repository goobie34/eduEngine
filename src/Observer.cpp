#include "Observer.hpp"
#include <array>
#pragma once

void EventSource::Notify(Event event)
{
    int index_end = _observers.size() - 1;
    for(int i = index_end; i >= 0; i--)
    {
        if(auto observer = _observers[i].lock())
        {
            observer->OnNotify(*this, event);
        }
    }
}
void EventSource::AddObserver(EventListener* observer)
{
    if (observer == nullptr) return;
    std::shared_ptr<EventListener> observer_shared(observer); 
    std::weak_ptr<EventListener>   observer_weak  (observer_shared);
    _observers.push_back(observer_weak);
}
void EventSource::RemoveObserver(EventListener* observer)
{
    std::shared_ptr<EventListener> observer_shared(observer); 

    int index_end = _observers.size() - 1;
    for(int i = index_end; i >= 0; i--)
    {
        if(auto observer = _observers[i].lock())
        {
            if (observer == observer_shared) {
                _observers.erase(_observers.begin() + i);
                return;
            }
        }
    }
}
