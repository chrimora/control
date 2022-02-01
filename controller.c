#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <xinput.h>
#include <windows.h>

#define true 1
#define false 0

#define SPEED_SCALER 12
#define STICK_MAX 32767.0f
#define INPUT_DEADZONE ( 0.22f * STICK_MAX ) // 22% deadzone

//gcc controller.c -l XInput1_4 -o out

void send_mouse_event(DWORD flag) {
    INPUT input;
    input.type = 0;
    input.mi.dwFlags = flag;

    SendInput(1, &input, sizeof(input));
}

void send_key(int key) {
    INPUT input;
    input.type = 1;
    input.ki.wVk = key;

    SendInput(1, &input, sizeof(input));
}


int pressed(XINPUT_GAMEPAD* prev_pad, XINPUT_GAMEPAD* pad, int button) {
    // prev_pad button up && pad button down
    return (((prev_pad->wButtons & button) == button) &&
            (pad->wButtons & button) != button);
}
int released(XINPUT_GAMEPAD* prev_pad, XINPUT_GAMEPAD* pad, int button) {
    // prev_pad button down && pad button up
    return (((prev_pad->wButtons & button) == button) &&
            (pad->wButtons & button) != button);
}
int held(XINPUT_GAMEPAD* prev_pad, XINPUT_GAMEPAD* pad, int button) {
    // prev_pad button down && pad button down
    return (((prev_pad->wButtons & button) == button) &&
            (pad->wButtons & button) == button);
}

/*
 * Actions that occur when the pad changes
 */
void act(XINPUT_GAMEPAD* prev_pad, XINPUT_GAMEPAD* pad) {

    if (released(prev_pad, pad, XINPUT_GAMEPAD_X)) {
        send_mouse_event(MOUSEEVENTF_RIGHTDOWN);
        send_mouse_event(MOUSEEVENTF_RIGHTUP);
    }
    if (released(prev_pad, pad, XINPUT_GAMEPAD_B))
        send_key(VK_ESCAPE);

    if (released(prev_pad, pad, XINPUT_GAMEPAD_DPAD_UP))
        send_key(VK_VOLUME_UP);
    if (released(prev_pad, pad, XINPUT_GAMEPAD_DPAD_DOWN))
        send_key(VK_VOLUME_DOWN);
    if (released(prev_pad, pad, XINPUT_GAMEPAD_DPAD_LEFT))
        send_key(VK_LEFT);
    if (released(prev_pad, pad, XINPUT_GAMEPAD_DPAD_RIGHT))
        send_key(VK_RIGHT);
}

void move_mouse(short x, short y) {
    // Deadzone explanation
    // https://docs.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput#dead-zone
    float magnitude = sqrt(x*x + y*y);
    if (magnitude > INPUT_DEADZONE) {
        if (magnitude > STICK_MAX)
            magnitude = STICK_MAX;

        magnitude -= INPUT_DEADZONE;

        // Normalise magnitude
        magnitude = magnitude / (STICK_MAX - INPUT_DEADZONE);
        // Attempt to make motion more exponential
        magnitude = pow(magnitude, 7);
        magnitude *= SPEED_SCALER;

        float angle = atan2(y, x);
        float _x = magnitude * cos(angle);
        float _y = magnitude * sin(angle);

        POINT p;
        GetCursorPos(&p);

        p.x += _x;
        p.y -= _y;

        SetCursorPos(p.x, p.y);
    }
}

/*
 * Actions that occur regardless of pad changes
 */
int mouse_down = false;
void continuous_act(XINPUT_GAMEPAD* prev_pad, XINPUT_GAMEPAD* pad) {
    move_mouse(pad->sThumbLX, pad->sThumbLY);

    if (held(prev_pad, pad, XINPUT_GAMEPAD_A)) {
        if (mouse_down == false) {
            send_mouse_event(MOUSEEVENTF_LEFTDOWN);
            mouse_down = true;
        }
    }
    else {
        if (mouse_down == true) {
            send_mouse_event(MOUSEEVENTF_LEFTUP);
            mouse_down = false;
        }
    }
}

int main() {
    XINPUT_STATE prev_state, state;

    DWORD ret = XInputGetState(0, &prev_state);
    if (ret == ERROR_DEVICE_NOT_CONNECTED) {
        printf("ERROR: Device is not connected.");
        return 1;
    }
    printf("INFO: Device is connected.");

    int packets = state.dwPacketNumber;
    while (true) {
        Sleep(10);

        ret = XInputGetState(0, &state);

        if (state.dwPacketNumber != prev_state.dwPacketNumber) {
            act(&prev_state.Gamepad, &state.Gamepad);
        }
        continuous_act(&prev_state.Gamepad, &state.Gamepad);

        prev_state = state;
    }
    return 0;
}

