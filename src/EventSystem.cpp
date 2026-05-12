#include "EventSystem.hpp"
#include <array>

#pragma once

void EventSource::Notify(Event event)
{
    int index_end = m_listeners.size() - 1;
    for(int i = index_end; i >= 0; i--)
    {
        if(auto listener = m_listeners[i].lock())
        {
            listener->OnNotify(*this, event);
        }
    }
}
void EventSource::AddObserver(EventListener* listener_ptr)
{
    if (listener_ptr == nullptr) return;
    std::shared_ptr<EventListener> listener_shared (listener_ptr); 
    std::weak_ptr<EventListener>   listener_weak   (listener_shared);
    m_listeners.push_back(listener_weak);
}
void EventSource::RemoveObserver(EventListener* listener_ptr)
{
    std::shared_ptr<EventListener> listener_to_remove (listener_ptr); 

    int index_end = m_listeners.size() - 1;
    for(int i = index_end; i >= 0; i--)
    {
        if(auto listener = m_listeners[i].lock())
        {
            if (listener == listener_to_remove) {
                m_listeners.erase(m_listeners.begin() + i);
                return;
            }
        }
    }
}

void EventQueue::EnqueueEvent(Event event) {
    if (m_queue.size() >= m_capacity) return;
    m_queue.push_back(event);
}

void EventQueue::BroadcastAllEvents() {
    for(auto& event : m_queue) {
        Notify(event);
    }
    m_queue.clear();
}

void EventQueue::OnNotify(EventSource& entity, Event event) {
    EnqueueEvent(event);
}
