#include <map>
#include <string>
#include "InputManager.hpp"
#pragma once

namespace eeng {
    using Key = InputManager::Key;

    struct InputMap {
        InputMap(std::string name = "Input Map") : name(name) {}

        const std::string name;
        std::map<std::string, std::vector<Key>> keybinds;

        void addKeybind(std::string action_identifier, std::vector<Key> keys) {
            keybinds[action_identifier] = keys;
        }

        bool isPressed(const std::string action_identifier, const InputManagerPtr input) const {
            auto it = keybinds.find(action_identifier);
            if (it == keybinds.end()) return false;

            auto key_vector = keybinds.at(action_identifier);

            for(auto key : key_vector)
                if (input->IsKeyPressed(key))
                    return true;

            return false;
        }
    };
};