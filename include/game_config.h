#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <cstdint>
#include <map>
#include <string>

enum class mouse_action {
    MOUSE_LEFT_PRESS = 0,
    MOUSE_LEFT_DOWN = 1,
    MOUSE_LEFT_UP = 2,
    MOUSE_RIGHT_PRESS = 3,
    MOUSE_RIGHT_DOWN = 4,
    MOUSE_RIGHT_UP = 5,
    MOUSE_WHEEL_DOWN = 6,
    MOUSE_WHEEL_UP = 7
};

const std::uint8_t SDL_CONTROLLER_TRIGGER_LEFT = 100;
const std::uint8_t SDL_CONTROLLER_TRIGGER_RIGHT = 101;

struct game_config {
    bool USE_VSYNC;
    float INJECTION_FRAME_OFFSET_FRACTION;
    std::uint16_t INJECTION_FRAMERATE;

    std::map<std::uint8_t, std::uint16_t> BUTTON_DOWN_TO_KEY_PRESS;
    std::map<std::uint8_t, std::uint16_t> BUTTON_DOWN_TO_KEY_DOWN;
    std::map<std::uint8_t, std::uint16_t> BUTTON_UP_TO_KEY_PRESS;
    std::map<std::uint8_t, std::uint16_t> BUTTON_UP_TO_KEY_UP;
    std::map<std::uint8_t, mouse_action> BUTTON_DOWN_TO_MOUSE_EVENT;
    std::map<std::uint8_t, mouse_action> BUTTON_UP_TO_MOUSE_EVENT;

    std::int8_t LOOK_STICK;

    std::uint16_t LEFT_STICK_KEY_LEFT;
    std::uint16_t LEFT_STICK_KEY_RIGHT;
    std::uint16_t LEFT_STICK_KEY_UP;
    std::uint16_t LEFT_STICK_KEY_DOWN;

    std::uint16_t RIGHT_STICK_KEY_LEFT;
    std::uint16_t RIGHT_STICK_KEY_RIGHT;
    std::uint16_t RIGHT_STICK_KEY_UP;
    std::uint16_t RIGHT_STICK_KEY_DOWN;

    std::uint16_t MOVE_SPEED_MIN_AXIS_VALUE;
    std::uint16_t MOVE_SPEED_MAX_AXIS_VALUE;
    float MOVE_SPEED_DUTY_CYCLE_MIN;

    std::uint16_t LOOK_SPEED_MIN_AXIS_VALUE;
    std::uint16_t LOOK_SPEED_MAX_AXIS_VALUE;
    float LOOK_SPEED_MIN_X;
    float LOOK_SPEED_MAX_X;
    float LOOK_SPEED_MIN_Y;
    float LOOK_SPEED_MAX_Y;
    float LOOK_EASING_X;
    float LOOK_EASING_Y;

    std::uint16_t TRIGGER_DEPTH;
};

std::map<std::string, game_config> load_games(const std::string& yaml_path);

#endif
