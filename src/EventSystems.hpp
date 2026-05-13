#include "EventComponents.hpp"
#pragma once

static class ObserverSystem {
public:    
    static void Update(entt::registry& registry)
    {
        auto view = registry.view<SourceComponent>();
        for(auto entity : view) {
            auto& sourceComponent = view.get<SourceComponent>(entity);

            RemoveObservers(sourceComponent);
            AddObserver(sourceComponent);
            HandleEvents(sourceComponent, registry);
        }
    }

    static void RemoveObservers(SourceComponent& sourceComponent) {
        std::vector<entt::entity>& observers = sourceComponent.observers;
        int index_end = observers.size() - 1;
        for(auto observerToRemove : sourceComponent.observersToRemove) {
            for(int i = index_end; i >= 0; i--)
            {
                if (observers[i] == observerToRemove) {
                    observers.erase(observers.begin() + i);
                    break;
                }
            }
        }
        sourceComponent.observersToRemove.clear();
    }

    static void AddObserver(SourceComponent& sourceComponent) {
        std::vector<entt::entity>& observers = sourceComponent.observers;
        int index_end = observers.size() - 1;
        for(auto observerToAdd : sourceComponent.observersToAdd) {
            //here, a check could be added to ensure an observer cant be added if they are already subscribed
            observers.push_back(observerToAdd);
        }
        sourceComponent.observersToAdd.clear();
    }

    static void HandleEvents(SourceComponent& sourceComponent, entt::registry& registry) {
        std::vector<Event>& events = sourceComponent.events;
        std::vector<entt::entity>& observers = sourceComponent.observers;
        
        while(events.size() > 0) {
            int index_end = events.size() - 1;
            for(int i = index_end; i >= 0; i--)
            {
                NotifyObservers(events[i], observers, registry);
            }
            //erase those events we just handled, if more have arrived, the loop will go again
            events.erase(events.begin(), events.begin() + index_end + 1);

        }
    }

    static void NotifyObservers(Event event, std::vector<entt::entity>& observers, entt::registry& registry) {
        for(auto observer : observers) {
            if (auto observerComponent = registry.try_get<ObserverComponent>(observer)) {
                observerComponent->OnNotify(event);
            }
        }
    }
};

static class EventQueueSystem {
public:    
    static void Update(float dt, entt::registry& registry)
    {
        auto view = registry.view<EventQueueComponent>();
        for(auto entity : view) {
            auto& eventQueue = view.get<EventQueueComponent>(entity);
            assert(eventQueue.delay > 0 && "eventQueue.delay is 0 or less");
            eventQueue.time_since_broadcast += dt;
            if (eventQueue.time_since_broadcast >= eventQueue.delay) {
                BroadcastAll(entity, eventQueue, registry);
                eventQueue.time_since_broadcast = 0.0f;
            }
        }
    }

    static void BroadcastAll(entt::entity entity, EventQueueComponent& eventQueue, entt::registry& registry) {
        if (auto sourceComponent = registry.try_get<SourceComponent>(entity)) {
            for(auto event : eventQueue.queue) {
                sourceComponent->AddEvent(event);
            }
        }
    }
};