#pragma once

// Key states
struct ButtonState {
    bool pressed{false};
    bool wasPressed{false};

    ButtonState();

    void onNewFrame();
    void reset();
    bool wasJustPressed() const;
    bool wasJustReleased() const;
    bool isPressed() const;
    bool isHeld() const;
};
