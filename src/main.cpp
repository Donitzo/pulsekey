///////////////////////////////////////////////////////////////////////////////
// Libraries

#define NOMINMAX
#define SDL_MAIN_HANDLED

#include <chrono>
#include <map>
#include <thread>
#include <vector>
#include <windows.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"

#include "game_config.h"

///////////////////////////////////////////////////////////////////////////////
// Metrics

const int FRAMERATE_AVERAGE_WINDOW_SIZE = 120;

///////////////////////////////////////////////////////////////////////////////
// Current game and settings

std::map<std::string, game_config> games;

int selected_game_index = 0;

game_config game;

bool use_vsync = true;
int target_framerate = 120;

///////////////////////////////////////////////////////////////////////////////
// ImGui

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

///////////////////////////////////////////////////////////////////////////////
// Windows input emulation

INPUT input;

void send_key_input(WORD vk_code, DWORD dw_flag) {
    if (vk_code == 0) {
        return;
    }
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = MapVirtualKeyW(vk_code, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = dw_flag | KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));
}

void send_mouse_input(DWORD dw_flag, DWORD mouse_data = 0) {
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = dw_flag;
    input.mi.mouseData = mouse_data;
    SendInput(1, &input, sizeof(INPUT));
}

void send_mouse_move_input(LONG dx, LONG dy) {
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
}

void perform_mouse_action(mouse_action e) {
    switch (e) {
        case mouse_action::MOUSE_LEFT_PRESS:
            send_mouse_input(MOUSEEVENTF_LEFTDOWN);
            SDL_Delay(1);
            send_mouse_input(MOUSEEVENTF_LEFTUP);
            break;
        case mouse_action::MOUSE_LEFT_DOWN:
            send_mouse_input(MOUSEEVENTF_LEFTDOWN);
            break;
        case mouse_action::MOUSE_LEFT_UP:
            send_mouse_input(MOUSEEVENTF_LEFTUP);
            break;
        case mouse_action::MOUSE_RIGHT_PRESS:
            send_mouse_input(MOUSEEVENTF_RIGHTDOWN);
            SDL_Delay(1);
            send_mouse_input(MOUSEEVENTF_RIGHTUP);
            break;
        case mouse_action::MOUSE_RIGHT_DOWN:
            send_mouse_input(MOUSEEVENTF_RIGHTDOWN);
            break;
        case mouse_action::MOUSE_RIGHT_UP:
            send_mouse_input(MOUSEEVENTF_RIGHTUP);
            break;
        case mouse_action::MOUSE_WHEEL_DOWN:
            send_mouse_input(MOUSEEVENTF_WHEEL, -WHEEL_DELTA);
            break;
        case mouse_action::MOUSE_WHEEL_UP:
            send_mouse_input(MOUSEEVENTF_WHEEL, WHEEL_DELTA);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Gamepad mapper

SDL_GameController* controller = NULL;
Sint32 controller_index = -1;

float left_stick_move_x_acc = 0;
float left_stick_move_y_acc = 0;

float right_stick_move_x_acc = 0;
float right_stick_move_y_acc = 0;

float look_x_acc = 0;
float look_y_acc = 0;

bool left_stick_key_left_pressed = false;
bool left_stick_key_right_pressed = false;
bool left_stick_key_up_pressed = false;
bool left_stick_key_down_pressed = false;

bool right_stick_key_left_pressed = false;
bool right_stick_key_right_pressed = false;
bool right_stick_key_up_pressed = false;
bool right_stick_key_down_pressed = false;

bool trigger_left_pressed = false;
bool trigger_right_pressed = false;

void update_analog_input(float dt) {
    if (controller_index == -1) {
        return;
    }

    // Simulate keyboard movement using the left analog stick
    if (game.LOOK_STICK != 0) {
        // Get the axis values
        Sint16 move_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 move_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

        // Calculate the pulse-width modulation duty cycle based on the axis values
        float move_ptp = (float)(game.MOVE_SPEED_MAX_AXIS_VALUE - game.MOVE_SPEED_MIN_AXIS_VALUE);
        float duty_cycle_x = std::min(1.0f, (float)std::max(0, abs(move_x) - game.MOVE_SPEED_MIN_AXIS_VALUE) / move_ptp);
        float duty_cycle_y = std::min(1.0f, (float)std::max(0, abs(move_y) - game.MOVE_SPEED_MIN_AXIS_VALUE) / move_ptp);

        // Increment PWM accumulators
        if (duty_cycle_x >= game.MOVE_SPEED_DUTY_CYCLE_MIN) {
            left_stick_move_x_acc += duty_cycle_x * (move_x > 0 ? 1 : -1);
        }
        if (duty_cycle_y >= game.MOVE_SPEED_DUTY_CYCLE_MIN) {
            left_stick_move_y_acc += duty_cycle_y * (move_y > 0 ? 1 : -1);
        }

        // Emulate key presses and releases
        if (left_stick_move_x_acc <= -1) {
            left_stick_move_x_acc += 1;

            if (!left_stick_key_left_pressed) {
                send_key_input(game.LEFT_STICK_KEY_LEFT, 0);
                left_stick_key_left_pressed = true;
            }
        } else if (left_stick_key_left_pressed) {
            send_key_input(game.LEFT_STICK_KEY_LEFT, KEYEVENTF_KEYUP);
            left_stick_key_left_pressed = false;
        }

        if (left_stick_move_x_acc >= 1) {
            left_stick_move_x_acc -= 1;

            if (!left_stick_key_right_pressed) {
                send_key_input(game.LEFT_STICK_KEY_RIGHT, 0);
                left_stick_key_right_pressed = true;
            }
        } else if (left_stick_key_right_pressed) {
            send_key_input(game.LEFT_STICK_KEY_RIGHT, KEYEVENTF_KEYUP);
            left_stick_key_right_pressed = false;
        }

        if (left_stick_move_y_acc <= -1) {
            left_stick_move_y_acc += 1;

            if (!left_stick_key_up_pressed) {
                send_key_input(game.LEFT_STICK_KEY_UP, 0);
                left_stick_key_up_pressed = true;
            }
        } else if (left_stick_key_up_pressed) {
            send_key_input(game.LEFT_STICK_KEY_UP, KEYEVENTF_KEYUP);
            left_stick_key_up_pressed = false;
        }

        if (left_stick_move_y_acc >= 1) {
            left_stick_move_y_acc -= 1;

            if (!left_stick_key_down_pressed) {
                send_key_input(game.LEFT_STICK_KEY_DOWN, 0);
                left_stick_key_down_pressed = true;
            }
        } else if (left_stick_key_down_pressed) {
            send_key_input(game.LEFT_STICK_KEY_DOWN, KEYEVENTF_KEYUP);
            left_stick_key_down_pressed = false;
        }
    }

    // Simulate keyboard movement using the right analog stick
    if (game.LOOK_STICK != 1) {
        // Get the axis values
        Sint16 move_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
        Sint16 move_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

        // Calculate the pulse-width modulation duty cycle based on the axis values
        float move_ptp = (float)(game.MOVE_SPEED_MAX_AXIS_VALUE - game.MOVE_SPEED_MIN_AXIS_VALUE);
        float duty_cycle_x = std::min(1.0f, (float)std::max(0, abs(move_x) - game.MOVE_SPEED_MIN_AXIS_VALUE) / move_ptp);
        float duty_cycle_y = std::min(1.0f, (float)std::max(0, abs(move_y) - game.MOVE_SPEED_MIN_AXIS_VALUE) / move_ptp);

        // Increment PWM accumulators
        if (duty_cycle_x >= game.MOVE_SPEED_DUTY_CYCLE_MIN) {
            right_stick_move_x_acc += duty_cycle_x * (move_x > 0 ? 1 : -1);
        }
        if (duty_cycle_y >= game.MOVE_SPEED_DUTY_CYCLE_MIN) {
            right_stick_move_y_acc += duty_cycle_y * (move_y > 0 ? 1 : -1);
        }

        // Emulate key presses and releases
        if (right_stick_move_x_acc <= -1) {
            right_stick_move_x_acc += 1;

            if (!right_stick_key_left_pressed) {
                send_key_input(game.RIGHT_STICK_KEY_LEFT, 0);
                right_stick_key_left_pressed = true;
            }
        } else if (right_stick_key_left_pressed) {
            send_key_input(game.RIGHT_STICK_KEY_LEFT, KEYEVENTF_KEYUP);
            right_stick_key_left_pressed = false;
        }

        if (right_stick_move_x_acc >= 1) {
            right_stick_move_x_acc -= 1;

            if (!right_stick_key_right_pressed) {
                send_key_input(game.RIGHT_STICK_KEY_RIGHT, 0);
                right_stick_key_right_pressed = true;
            }
        } else if (right_stick_key_right_pressed) {
            send_key_input(game.RIGHT_STICK_KEY_RIGHT, KEYEVENTF_KEYUP);
            right_stick_key_right_pressed = false;
        }

        if (right_stick_move_y_acc <= -1) {
            right_stick_move_y_acc += 1;

            if (!right_stick_key_up_pressed) {
                send_key_input(game.RIGHT_STICK_KEY_UP, 0);
                right_stick_key_up_pressed = true;
            }
        } else if (right_stick_key_up_pressed) {
            send_key_input(game.RIGHT_STICK_KEY_UP, KEYEVENTF_KEYUP);
            right_stick_key_up_pressed = false;
        }

        if (right_stick_move_y_acc >= 1) {
            right_stick_move_y_acc -= 1;

            if (!right_stick_key_down_pressed) {
                send_key_input(game.RIGHT_STICK_KEY_DOWN, 0);
                right_stick_key_down_pressed = true;
            }
        } else if (right_stick_key_down_pressed) {
            send_key_input(game.RIGHT_STICK_KEY_DOWN, KEYEVENTF_KEYUP);
            right_stick_key_down_pressed = false;
        }
    }

    // Simulate mouse look using either analog stick
    if (game.LOOK_STICK == 0 || game.LOOK_STICK == 1) {
        // Get the axis values
        Sint16 look_x = SDL_GameControllerGetAxis(controller, game.LOOK_STICK == 0 ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX);
        Sint16 look_y = SDL_GameControllerGetAxis(controller, game.LOOK_STICK == 0 ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY);

        // Calculate the mouse speed
        float look_ptp = (float)(game.LOOK_SPEED_MAX_AXIS_VALUE - game.LOOK_SPEED_MIN_AXIS_VALUE);
        float look_speed_x = std::min(1.0f, (float)std::max(0, abs(look_x) - game.LOOK_SPEED_MIN_AXIS_VALUE) / look_ptp);
        float look_speed_y = std::min(1.0f, (float)std::max(0, abs(look_y) - game.LOOK_SPEED_MIN_AXIS_VALUE) / look_ptp);

        // Increment mouse sub-pixel movement accumulators
        if (look_speed_x != 0) {
            float look_speed_ptp_x = game.LOOK_SPEED_MAX_X - game.LOOK_SPEED_MIN_X;
            look_x_acc += (game.LOOK_SPEED_MIN_X + look_speed_x * look_speed_ptp_x) * (look_x > 0 ? 1 : -1) * dt;
        }
        if (look_speed_y != 0) {
            float look_speed_ptp_y = game.LOOK_SPEED_MAX_Y - game.LOOK_SPEED_MIN_Y;
            look_y_acc += (game.LOOK_SPEED_MIN_Y + look_speed_y * look_speed_ptp_y) * (look_y > 0 ? 1 : -1) * dt;
        }

        // Move mouse whole integers while saving sub-pixels in the accumulator

        LONG dx = (LONG)look_x_acc;
        LONG dy = (LONG)look_y_acc;

        look_x_acc -= dx;
        look_y_acc -= dy;

        if (dx != 0 || dy != 0) {
            send_mouse_move_input(dx, dy);
        }
    }

    // Translate trigger presses into button events

    Sint16 trigger_left = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    Sint16 trigger_right = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);

    bool trigger_left_last = trigger_left > game.TRIGGER_DEPTH;
    bool trigger_right_last = trigger_right > game.TRIGGER_DEPTH;

    if (trigger_left_last != trigger_left_pressed) {
        trigger_left_pressed = trigger_left_last;

        SDL_Event e;
        e.type = trigger_left_pressed ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP;
        e.cbutton.which = controller_index;
        e.cbutton.button = SDL_CONTROLLER_TRIGGER_LEFT;
        SDL_PushEvent(&e);
    }

    if (trigger_right_last != trigger_right_pressed) {
        trigger_right_pressed = trigger_right_last;

        SDL_Event e;
        e.type = trigger_right_pressed ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP;
        e.cbutton.which = controller_index;
        e.cbutton.button = SDL_CONTROLLER_TRIGGER_RIGHT;
        SDL_PushEvent(&e);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Program loop

void swap_game(std::string name) {
    // Select a new game
    game = games.at(name);

    // Reset v-sync and target framerate
    use_vsync = game.USE_VSYNC;
    target_framerate = game.INJECTION_FRAMERATE;

    // Set v-sync enabled/disabled in SDL
    SDL_GL_SetSwapInterval(game.USE_VSYNC ? 1 : 0);
}

int main(int argc, char* argv[]) {
    // Redirect stdout, stderr to "log.txt"
    freopen("log.txt", "a", stdout);
    freopen("log.txt", "a", stderr);

    printf("Application started\n");

    // Read game configurations from YAML

    games = load_games("games.yaml");

    if (games.size() == 0) {
        printf("No games found in \"games.yaml\"");
        return -1;
    }

    // Get game names
    std::vector<std::string> game_names;
    for (const auto& pair : games) {
        game_names.push_back(pair.first);
    }

    // Initialize video first
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error initializing SDL video: %s\n", SDL_GetError());
        return -1;
    }

    // Create SDL window

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("PulseKey",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 240, 168,
        SDL_WINDOW_ALLOW_HIGHDPI |
        SDL_WINDOW_OPENGL);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    // Setup ImGui

    printf("Initializing ImGui\n");

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& imgui_io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Initialize gamecontroller and events

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        printf("Error initializing SDL gamecontroller or events: %s\n", SDL_GetError());
        return -1;
    }

    // Get default controller

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (SDL_NumJoysticks() > 0) {
        controller = SDL_GameControllerOpen(0);
        controller_index = 0;

        printf("Using controller: %s!\n", SDL_GameControllerName(controller));
    } else {
        printf("No controller detected\n");
    }

    // Select the first game
    swap_game(game_names[0]);

    // Create multimedia timer

    HANDLE timer;
    LARGE_INTEGER li;

    if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL))) {
        printf("Error creating timer");
        return -1;
    }

    // Main loop

    SDL_Event event;

    bool running = true;

    Uint64 next_frame_microseconds = static_cast<Uint64>(SDL_GetTicks()) * 1000ULL;

    Uint64 last_update_counter = SDL_GetPerformanceCounter();
    Uint64 last_framerate_counter = SDL_GetPerformanceCounter();

    float framerate = 0;
    int frames_until_calculate_framerate = FRAMERATE_AVERAGE_WINDOW_SIZE;

    while (running) {
        // Calculate average framerate
        if (--frames_until_calculate_framerate <= 0) {
            Uint64 framerate_counter = SDL_GetPerformanceCounter();
            float elapsed_seconds = (float)(framerate_counter - last_framerate_counter) / SDL_GetPerformanceFrequency();
            last_framerate_counter = framerate_counter;

            framerate = FRAMERATE_AVERAGE_WINDOW_SIZE / elapsed_seconds;

            frames_until_calculate_framerate = FRAMERATE_AVERAGE_WINDOW_SIZE;
        }

        // Handle events
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_CONTROLLERDEVICEADDED:
                    if (controller_index == -1) {
                        // Change controller
                        controller = SDL_GameControllerOpen(event.cdevice.which);
                        controller_index = event.cdevice.which;

                        printf("Using controller: %s!\n", SDL_GameControllerName(controller));
                    }
                    break;

                case SDL_CONTROLLERDEVICEREMOVED:
                    if (controller_index == event.cdevice.which) {
                        printf("Controller disconnected\n");

                        SDL_GameControllerClose(controller);
                        controller_index = -1;
                    }
                    break;

                case SDL_CONTROLLERBUTTONDOWN:
                    // Translate button presses to keyboard/mouse events
                    if (event.cbutton.which == controller_index) {
                        if (game.BUTTON_DOWN_TO_KEY_PRESS.count(event.cbutton.button)) {
                            send_key_input(game.BUTTON_DOWN_TO_KEY_PRESS.at(event.cbutton.button), 0);
                            send_key_input(game.BUTTON_DOWN_TO_KEY_PRESS.at(event.cbutton.button), KEYEVENTF_KEYUP);
                        }
                        if (game.BUTTON_DOWN_TO_KEY_DOWN.count(event.cbutton.button)) {
                            send_key_input(game.BUTTON_DOWN_TO_KEY_DOWN.at(event.cbutton.button), 0);
                        }
                        if (game.BUTTON_DOWN_TO_MOUSE_EVENT.count(event.cbutton.button)) {
                            perform_mouse_action(game.BUTTON_DOWN_TO_MOUSE_EVENT.at(event.cbutton.button));
                        }
                    }
                    break;

                case SDL_CONTROLLERBUTTONUP:
                    // Translate button releases to keyboard/mouse events
                    if (event.cbutton.which == controller_index) {
                        if (game.BUTTON_UP_TO_KEY_UP.count(event.cbutton.button)) {
                            send_key_input(game.BUTTON_UP_TO_KEY_UP.at(event.cbutton.button), KEYEVENTF_KEYUP);
                        }
                        if (game.BUTTON_UP_TO_KEY_PRESS.count(event.cbutton.button)) {
                            send_key_input(game.BUTTON_UP_TO_KEY_PRESS.at(event.cbutton.button), 0);
                            send_key_input(game.BUTTON_UP_TO_KEY_PRESS.at(event.cbutton.button), KEYEVENTF_KEYUP);
                        }
                        if (game.BUTTON_UP_TO_MOUSE_EVENT.count(event.cbutton.button)) {
                            perform_mouse_action(game.BUTTON_UP_TO_MOUSE_EVENT.at(event.cbutton.button));
                        }
                    }
                    break;
            }
        }

        // Handle analog control emulation
        Uint64 update_counter = SDL_GetPerformanceCounter();
        if (controller_index > -1) {
            float elapsed_seconds = (float)(update_counter - last_update_counter) / SDL_GetPerformanceFrequency();
            update_analog_input(elapsed_seconds);
        }
        last_update_counter = update_counter;

        // Create new ImGui frame

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        bool show_window = true;

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240, 168), ImGuiCond_FirstUseEver);
        ImGui::Begin("PulseKey", &show_window,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar);

        // Dropdown to select game
        ImGui::Text("Select game");
        if (ImGui::BeginCombo("##Games", game_names[selected_game_index].c_str())) {
            for (int i = 0; i < game_names.size(); i++) {
                if (ImGui::Selectable(game_names[i].c_str(), selected_game_index == i)) {
                    selected_game_index = i;
                    swap_game(game_names[selected_game_index]);
                    next_frame_microseconds = static_cast<Uint64>(SDL_GetTicks()) * 1000ULL;
                }

                if (selected_game_index == i) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Enable/disable v-sync
        if (ImGui::Checkbox("Use v-sync", &use_vsync)) {
            if (use_vsync) {
                SDL_GL_SetSwapInterval(1);
            } else {
                SDL_GL_SetSwapInterval(0);

                next_frame_microseconds = static_cast<Uint64>(SDL_GetTicks()) * 1000ULL;
            }
        }

        if (use_vsync) {
            ImGui::Text("Input at VBLANK + %i%%", (int)(game.INJECTION_FRAME_OFFSET_FRACTION * 100.0));
        } else {
            // Updates per second slider
            ImGui::Text("Updates per second:");
            ImGui::SliderInt("##Framerate", &target_framerate, 10, 240);
        }

        // Show controller info
        ImGui::Text(controller_index == -1 ? "No controller found" : SDL_GameControllerName(controller));

        // Show display info
        int display_index = SDL_GetWindowDisplayIndex(window);
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(display_index, 0, &mode);

        ImGui::Text("Display: %s (%d Hz)", SDL_GetDisplayName(display_index), mode.refresh_rate);

        // Show framerate
        ImGui::Text("Input framerate: %.2f Hz", framerate);

        // Render ImGui window

        ImGui::End();

        ImGui::Render();

        glViewport(0, 0, (int)imgui_io.DisplaySize.x, (int)imgui_io.DisplaySize.y);
        glClearColor(clear_color.x* clear_color.w, clear_color.y* clear_color.w, clear_color.z* clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers (and vertical sync if enabled)
        SDL_GL_SwapWindow(window);

        // Limit framerate
        if (use_vsync) {
            // Inject input at a fraction of the current frame after the vertical sync
            if (game.INJECTION_FRAME_OFFSET_FRACTION > 0 && mode.refresh_rate > 0) {
                float frame_microseconds = 1000000.0f / mode.refresh_rate;
                li.QuadPart = (Sint64)(frame_microseconds * game.INJECTION_FRAME_OFFSET_FRACTION) * -10;
                if (SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
                    WaitForSingleObject(timer, INFINITE);
                }
            }
        } else {
            // Use a target frame rate
            Uint64 current_microseconds = static_cast<Uint64>(SDL_GetTicks()) * 1000ULL;
            if (current_microseconds < next_frame_microseconds) {
                li.QuadPart = (Sint64)(next_frame_microseconds - current_microseconds) * -10;
                if (SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
                    WaitForSingleObject(timer, INFINITE);
                }
            }
            next_frame_microseconds += static_cast<Uint64>(1000000.0 / target_framerate);
        }
    }

    // Cleanup

    CloseHandle(timer);

    if (controller_index > -1) {
        SDL_GameControllerClose(controller);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);

    SDL_Quit();

    printf("Application closed\n");

    return 0;
}
