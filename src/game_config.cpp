#include "game_config.h"

#include <iostream>
#include <optional>
#include <yaml-cpp/yaml.h>

std::optional<game_config> load_game_config(const YAML::Node& node) {
    game_config config;

    try {
        // Parse YAML properties for game configuration

        config.USE_VSYNC = node["USE_VSYNC"].as<bool>();
        config.INJECTION_FRAME_OFFSET_FRACTION = node["INJECTION_FRAME_OFFSET_FRACTION"].as<float>();

        config.INJECTION_FRAMERATE = node["INJECTION_FRAMERATE"].as<std::uint16_t>();

        auto iterator = node["BUTTON_DOWN_TO_KEY_PRESS"];
        for (YAML::const_iterator it = iterator.begin(); it != iterator.end(); ++it) {
            config.BUTTON_DOWN_TO_KEY_PRESS[it->first.as<std::uint8_t>()] = it->second.as<std::uint16_t>();
        }
        iterator = node["BUTTON_DOWN_TO_KEY_DOWN"];
        for (YAML::const_iterator it = iterator.begin(); it != iterator.end(); ++it) {
            config.BUTTON_DOWN_TO_KEY_DOWN[it->first.as<std::uint8_t>()] = it->second.as<std::uint16_t>();
        }
        iterator = node["BUTTON_UP_TO_KEY_PRESS"];
        for (YAML::const_iterator it = iterator.begin(); it != iterator.end(); ++it) {
            config.BUTTON_UP_TO_KEY_PRESS[it->first.as<std::uint8_t>()] = it->second.as<std::uint16_t>();
        }
        iterator = node["BUTTON_UP_TO_KEY_UP"];
        for (YAML::const_iterator it = iterator.begin(); it != iterator.end(); ++it) {
            config.BUTTON_UP_TO_KEY_UP[it->first.as<std::uint8_t>()] = it->second.as<std::uint16_t>();
        }
        iterator = node["BUTTON_DOWN_TO_MOUSE_EVENT"];
        for (YAML::const_iterator it = iterator.begin(); it != iterator.end(); ++it) {
            config.BUTTON_DOWN_TO_MOUSE_EVENT[it->first.as<std::uint8_t>()] = static_cast<mouse_action>(it->second.as<int>());
        }
        iterator = node["BUTTON_UP_TO_MOUSE_EVENT"];
        for (YAML::const_iterator it = iterator.begin(); it != iterator.end(); ++it) {
            config.BUTTON_UP_TO_MOUSE_EVENT[it->first.as<std::uint8_t>()] = static_cast<mouse_action>(it->second.as<int>());
        }

        config.LOOK_STICK = node["LOOK_STICK"].as<std::int8_t>();

        config.LEFT_STICK_KEY_LEFT = node["LEFT_STICK_KEY_LEFT"].as<std::uint16_t>();
        config.LEFT_STICK_KEY_RIGHT = node["LEFT_STICK_KEY_RIGHT"].as<std::uint16_t>();
        config.LEFT_STICK_KEY_UP = node["LEFT_STICK_KEY_UP"].as<std::uint16_t>();
        config.LEFT_STICK_KEY_DOWN = node["LEFT_STICK_KEY_DOWN"].as<std::uint16_t>();

        config.RIGHT_STICK_KEY_LEFT = node["RIGHT_STICK_KEY_LEFT"].as<std::uint16_t>();
        config.RIGHT_STICK_KEY_RIGHT = node["RIGHT_STICK_KEY_RIGHT"].as<std::uint16_t>();
        config.RIGHT_STICK_KEY_UP = node["RIGHT_STICK_KEY_UP"].as<std::uint16_t>();
        config.RIGHT_STICK_KEY_DOWN = node["RIGHT_STICK_KEY_DOWN"].as<std::uint16_t>();

        config.MOVE_SPEED_MIN_AXIS_VALUE = node["MOVE_SPEED_MIN_AXIS_VALUE"].as<std::uint16_t>();
        config.MOVE_SPEED_MAX_AXIS_VALUE = node["MOVE_SPEED_MAX_AXIS_VALUE"].as<std::uint16_t>();
        config.MOVE_SPEED_DUTY_CYCLE_MIN = node["MOVE_SPEED_DUTY_CYCLE_MIN"].as<float>();

        config.LOOK_SPEED_MIN_AXIS_VALUE = node["LOOK_SPEED_MIN_AXIS_VALUE"].as<std::uint16_t>();
        config.LOOK_SPEED_MAX_AXIS_VALUE = node["LOOK_SPEED_MAX_AXIS_VALUE"].as<std::uint16_t>();
        config.LOOK_SPEED_MIN_X = node["LOOK_SPEED_MIN_X"].as<float>();
        config.LOOK_SPEED_MAX_X = node["LOOK_SPEED_MAX_X"].as<float>();
        config.LOOK_SPEED_MIN_Y = node["LOOK_SPEED_MIN_Y"].as<float>();
        config.LOOK_SPEED_MAX_Y = node["LOOK_SPEED_MAX_Y"].as<float>();
        config.LOOK_EASING_X = node["LOOK_EASING_X"].as<float>();
        config.LOOK_EASING_Y = node["LOOK_EASING_Y"].as<float>();

        config.TRIGGER_DEPTH = node["TRIGGER_DEPTH"].as<std::uint16_t>();

        return config;
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;

        return std::nullopt;
    }
}

std::map<std::string, game_config> load_games(const std::string& yaml_path) {
    // Parse all games from the YAML file

    YAML::Node root = YAML::LoadFile(yaml_path);
    std::map<std::string, game_config> games;

    for (const auto& game : root) {
        auto game_config_opt = load_game_config(game.second);
        if (game_config_opt) {
            games[game.first.as<std::string>()] = *game_config_opt;
        } else {
            std::cerr << "Skipping game due to invalid configuration: " << game.first.as<std::string>() << std::endl;
        }
    }

    return games;
}
