// Created by Carl Johan Gribel.
// Licensed under the MIT License. See LICENSE file for details.

#ifndef InputManager_hpp
#define InputManager_hpp

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace eeng {

    class InputManager
    {
    public:
        enum class Key {
            A, B, C, D, E, F, G, H, I, J, K, L, M,
            N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
            LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
            Up, Down, Left, Right,
            Space, Enter, Backspace, Tab, Escape,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
            None
        };

        struct MouseState 
        {
            int x = 0, y = 0;
            bool leftButton = false;
            bool rightButton = false;
        };

        struct ControllerState 
        {
            float axisLeftX = 0.0f, axisLeftY = 0.0f;
            float axisRightX = 0.0f, axisRightY = 0.0f;
            float triggerLeft = 0.0f, triggerRight = 0.0f;
            std::unordered_map<int, bool> buttonStates;
            std::string name;
        };
        using ControllerMap = std::unordered_map<int, ControllerState>;

        InputManager();
        ~InputManager();

        void HandleEvent(const void* event);
        void Update();

        // Query functions
        bool IsKeyPressed(Key key) const;
        bool IsMouseButtonDown(int button) const;
        const MouseState& GetMouseState() const;
        const ControllerState& GetControllerState(int controllerIndex) const;
        int GetConnectedControllerCount() const;
        ControllerMap& GetControllers();

    private:

        // pImpl pattern to make the header more light weight
        struct Impl;
        Impl* pImpl;
    };
} // namespace eeng

using InputManagerPtr = std::shared_ptr<eeng::InputManager>;

#endif // INPUTMANAGER_H
