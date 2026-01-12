#ifndef _GAMEPAD_INTERFACE_H_
#define _GAMEPAD_INTERFACE_H_

#include <gainput/gainput.h>
#include <mutex>
#include <iostream>

struct GamepadData {
    float      lx = 0.0;
    float      ly = 0.0;
    float      rx = 0.0;
    float      ry = 0.0;
    uint8_t     start = 0;
    uint8_t     select = 0;
    uint8_t     left = 0;
    uint8_t     right = 0;
    uint8_t     up = 0;
    uint8_t     down = 0;
    uint8_t     a = 0;
    uint8_t     b = 0;
    uint8_t     x = 0;
    uint8_t     y = 0;
    uint8_t     l1 = 0;
    uint8_t     r1 = 0;
};

class GamepadInterface {
public:
    GamepadInterface(GamepadData& data)
        : gamepad_data_(data) {}

    int init() {
        gainput::DeviceId device_id = manager_.CreateDevice<gainput::InputDevicePad>();
        device_ = manager_.GetDevice(device_id);

        if (device_->IsAvailable()) {
            return 0;
        } else {
            return -1;
        }
    }

    void run() {
        manager_.Update();
        
        gamepad_data_.lx = device_->GetFloat(0);
        gamepad_data_.ly = device_->GetFloat(1);
        gamepad_data_.rx = device_->GetFloat(2);
        gamepad_data_.ry = device_->GetFloat(3);
        gamepad_data_.start = device_->GetBool(44);
        gamepad_data_.select = device_->GetBool(45);
        gamepad_data_.left = device_->GetBool(46);
        gamepad_data_.right = device_->GetBool(47);
        gamepad_data_.up = device_->GetBool(48);
        gamepad_data_.down = device_->GetBool(49);
        gamepad_data_.a = device_->GetBool(50);
        gamepad_data_.b = device_->GetBool(51);
        gamepad_data_.x = device_->GetBool(52);
        gamepad_data_.y = device_->GetBool(53);
        gamepad_data_.l1 = device_->GetBool(54);
        gamepad_data_.r1 = device_->GetBool(55);
    }

private:
    gainput::InputManager manager_;
    gainput::InputDevice* device_;
    bool running_flag_ = false;
    GamepadData& gamepad_data_;
};

#endif