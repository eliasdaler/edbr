#include <edbr/Input/InputStringMap.h>

#include <unordered_map>

template<typename T>
class StringBimap {
public:
    void addMapping(const T& value, const std::string& str)
    {
        strValueMap.emplace(value, str);
        valueStrMap.emplace(str, value);
    }

    const std::string& at(const T& value) const { return strValueMap.at(value); }

    const T& at(const std::string& str) const { return valueStrMap.at(str); }

private:
    std::unordered_map<T, std::string> strValueMap;
    std::unordered_map<std::string, T> valueStrMap;
};

StringBimap<SDL_Scancode> initKeyboardMapSDL()
{
    StringBimap<SDL_Scancode> bimap;
    bimap.addMapping(SDL_SCANCODE_0, "0");
    bimap.addMapping(SDL_SCANCODE_1, "1");
    bimap.addMapping(SDL_SCANCODE_2, "2");
    bimap.addMapping(SDL_SCANCODE_3, "3");
    bimap.addMapping(SDL_SCANCODE_4, "4");
    bimap.addMapping(SDL_SCANCODE_5, "5");
    bimap.addMapping(SDL_SCANCODE_6, "6");
    bimap.addMapping(SDL_SCANCODE_7, "7");
    bimap.addMapping(SDL_SCANCODE_8, "8");
    bimap.addMapping(SDL_SCANCODE_9, "9");
    bimap.addMapping(SDL_SCANCODE_A, "A");
    // AC = application keyboard
    bimap.addMapping(SDL_SCANCODE_AC_BACK, "AC_Back");
    bimap.addMapping(SDL_SCANCODE_AC_BOOKMARKS, "AC_Bookmarks");
    bimap.addMapping(SDL_SCANCODE_AC_FORWARD, "AC_Forward");
    bimap.addMapping(SDL_SCANCODE_AC_HOME, "AC_Home");
    bimap.addMapping(SDL_SCANCODE_AC_REFRESH, "AC_Refresh");
    bimap.addMapping(SDL_SCANCODE_AC_SEARCH, "AC_Search");
    bimap.addMapping(SDL_SCANCODE_AC_STOP, "AC_Stop");
    bimap.addMapping(SDL_SCANCODE_AGAIN, "Again");
    bimap.addMapping(SDL_SCANCODE_ALTERASE, "AltErase");
    bimap.addMapping(SDL_SCANCODE_APOSTROPHE, "Apostrophe");
    bimap.addMapping(SDL_SCANCODE_APPLICATION, "Application");
    bimap.addMapping(SDL_SCANCODE_AUDIOMUTE, "AudioMute");
    bimap.addMapping(SDL_SCANCODE_AUDIONEXT, "AudioNext");
    bimap.addMapping(SDL_SCANCODE_AUDIOPLAY, "AudioPlay");
    bimap.addMapping(SDL_SCANCODE_AUDIOPREV, "AudioPrev");
    bimap.addMapping(SDL_SCANCODE_AUDIOSTOP, "AudioStop");
    bimap.addMapping(SDL_SCANCODE_B, "B");
    bimap.addMapping(SDL_SCANCODE_BACKSLASH, "Backslash");
    bimap.addMapping(SDL_SCANCODE_BACKSPACE, "Backspace");
    bimap.addMapping(SDL_SCANCODE_BRIGHTNESSDOWN, "BrightnessDown");
    bimap.addMapping(SDL_SCANCODE_BRIGHTNESSUP, "BrightnessUp");
    bimap.addMapping(SDL_SCANCODE_C, "C");
    bimap.addMapping(SDL_SCANCODE_CALCULATOR, "Calculator");
    bimap.addMapping(SDL_SCANCODE_CANCEL, "Cancel");
    bimap.addMapping(SDL_SCANCODE_CAPSLOCK, "CapsLock");
    bimap.addMapping(SDL_SCANCODE_CLEAR, "Clear");
    bimap.addMapping(SDL_SCANCODE_CLEARAGAIN, "ClearAgain");
    bimap.addMapping(SDL_SCANCODE_COMMA, "Comma");
    bimap.addMapping(SDL_SCANCODE_COMPUTER, "Computer");
    bimap.addMapping(SDL_SCANCODE_COPY, "Copy");
    bimap.addMapping(SDL_SCANCODE_CRSEL, "Crsel");
    bimap.addMapping(SDL_SCANCODE_CURRENCYSUBUNIT, "CurrencySubUnit");
    bimap.addMapping(SDL_SCANCODE_CURRENCYUNIT, "CurrencyUnit");
    bimap.addMapping(SDL_SCANCODE_CUT, "Cut");
    bimap.addMapping(SDL_SCANCODE_D, "D");
    bimap.addMapping(SDL_SCANCODE_DECIMALSEPARATOR, "DecimalSeparator");
    bimap.addMapping(SDL_SCANCODE_DELETE, "Delete");
    bimap.addMapping(SDL_SCANCODE_DISPLAYSWITCH, "DisplaySwitch");
    bimap.addMapping(SDL_SCANCODE_DOWN, "Down");
    bimap.addMapping(SDL_SCANCODE_E, "E");
    bimap.addMapping(SDL_SCANCODE_EJECT, "Eject");
    bimap.addMapping(SDL_SCANCODE_END, "End");
    bimap.addMapping(SDL_SCANCODE_EQUALS, "Equals");
    bimap.addMapping(SDL_SCANCODE_ESCAPE, "Escape");
    bimap.addMapping(SDL_SCANCODE_EXECUTE, "Execute");
    bimap.addMapping(SDL_SCANCODE_EXSEL, "Exsel");
    bimap.addMapping(SDL_SCANCODE_F, "F");
    bimap.addMapping(SDL_SCANCODE_F1, "F1");
    bimap.addMapping(SDL_SCANCODE_F10, "F10");
    bimap.addMapping(SDL_SCANCODE_F11, "F11");
    bimap.addMapping(SDL_SCANCODE_F12, "F12");
    bimap.addMapping(SDL_SCANCODE_F13, "F13");
    bimap.addMapping(SDL_SCANCODE_F14, "F14");
    bimap.addMapping(SDL_SCANCODE_F15, "F15");
    bimap.addMapping(SDL_SCANCODE_F16, "F16");
    bimap.addMapping(SDL_SCANCODE_F17, "F17");
    bimap.addMapping(SDL_SCANCODE_F18, "F18");
    bimap.addMapping(SDL_SCANCODE_F19, "F19");
    bimap.addMapping(SDL_SCANCODE_F2, "F2");
    bimap.addMapping(SDL_SCANCODE_F20, "F20");
    bimap.addMapping(SDL_SCANCODE_F21, "F21");
    bimap.addMapping(SDL_SCANCODE_F22, "F22");
    bimap.addMapping(SDL_SCANCODE_F23, "F23");
    bimap.addMapping(SDL_SCANCODE_F24, "F24");
    bimap.addMapping(SDL_SCANCODE_F3, "F3");
    bimap.addMapping(SDL_SCANCODE_F4, "F4");
    bimap.addMapping(SDL_SCANCODE_F5, "F5");
    bimap.addMapping(SDL_SCANCODE_F6, "F6");
    bimap.addMapping(SDL_SCANCODE_F7, "F7");
    bimap.addMapping(SDL_SCANCODE_F8, "F8");
    bimap.addMapping(SDL_SCANCODE_F9, "F9");
    bimap.addMapping(SDL_SCANCODE_FIND, "Find");
    bimap.addMapping(SDL_SCANCODE_G, "G");
    bimap.addMapping(SDL_SCANCODE_GRAVE, "Grave");
    bimap.addMapping(SDL_SCANCODE_H, "H");
    bimap.addMapping(SDL_SCANCODE_HELP, "Help");
    bimap.addMapping(SDL_SCANCODE_HOME, "Home");
    bimap.addMapping(SDL_SCANCODE_I, "I");
    bimap.addMapping(SDL_SCANCODE_INSERT, "Insert");
    bimap.addMapping(SDL_SCANCODE_J, "J");
    bimap.addMapping(SDL_SCANCODE_K, "K");
    // Keyboard illumination - WHAT?
    bimap.addMapping(SDL_SCANCODE_KBDILLUMDOWN, "KBDIllumDown");
    bimap.addMapping(SDL_SCANCODE_KBDILLUMTOGGLE, "KBDIllumToggle");
    bimap.addMapping(SDL_SCANCODE_KBDILLUMUP, "KBDIllumUp");
    bimap.addMapping(SDL_SCANCODE_KP_0, "KP_0");
    bimap.addMapping(SDL_SCANCODE_KP_00, "KP_00");
    bimap.addMapping(SDL_SCANCODE_KP_000, "KP_000");
    bimap.addMapping(SDL_SCANCODE_KP_1, "KP_1");
    bimap.addMapping(SDL_SCANCODE_KP_2, "KP_2");
    bimap.addMapping(SDL_SCANCODE_KP_3, "KP_3");
    bimap.addMapping(SDL_SCANCODE_KP_4, "KP_4");
    bimap.addMapping(SDL_SCANCODE_KP_5, "KP_5");
    bimap.addMapping(SDL_SCANCODE_KP_6, "KP_6");
    bimap.addMapping(SDL_SCANCODE_KP_7, "KP_7");
    bimap.addMapping(SDL_SCANCODE_KP_8, "KP_8");
    bimap.addMapping(SDL_SCANCODE_KP_9, "KP_9");
    bimap.addMapping(SDL_SCANCODE_KP_A, "KP_a");
    bimap.addMapping(SDL_SCANCODE_KP_AMPERSAND, "KP_Ampersand");
    bimap.addMapping(SDL_SCANCODE_KP_AT, "KP_At");
    bimap.addMapping(SDL_SCANCODE_KP_B, "KP_B");
    bimap.addMapping(SDL_SCANCODE_KP_BACKSPACE, "KP_Backspace");
    bimap.addMapping(SDL_SCANCODE_KP_BINARY, "KP_Binary");
    bimap.addMapping(SDL_SCANCODE_KP_C, "KP_C");
    bimap.addMapping(SDL_SCANCODE_KP_CLEAR, "KP_Clear");
    bimap.addMapping(SDL_SCANCODE_KP_CLEARENTRY, "KP_Clearentry");
    bimap.addMapping(SDL_SCANCODE_KP_COLON, "KP_Colon");
    bimap.addMapping(SDL_SCANCODE_KP_COMMA, "KP_Comma");
    bimap.addMapping(SDL_SCANCODE_KP_D, "KP_D");
    bimap.addMapping(SDL_SCANCODE_KP_DBLAMPERSAND, "KP_DblAmpersand");
    bimap.addMapping(SDL_SCANCODE_KP_DBLVERTICALBAR, "KP_DblVerticalBar");
    bimap.addMapping(SDL_SCANCODE_KP_DECIMAL, "KP_Decimal");
    bimap.addMapping(SDL_SCANCODE_KP_DIVIDE, "KP_Divide");
    bimap.addMapping(SDL_SCANCODE_KP_E, "KP_E");
    bimap.addMapping(SDL_SCANCODE_KP_ENTER, "KP_Enter");
    bimap.addMapping(SDL_SCANCODE_KP_EQUALS, "KP_Equals");
    bimap.addMapping(SDL_SCANCODE_KP_EQUALSAS400, "KP_EqualsAS400");
    bimap.addMapping(SDL_SCANCODE_KP_EXCLAM, "KP_Exclam");
    bimap.addMapping(SDL_SCANCODE_KP_F, "KP_F");
    bimap.addMapping(SDL_SCANCODE_KP_GREATER, "KP_Greater");
    bimap.addMapping(SDL_SCANCODE_KP_HASH, "KP_Hash");
    bimap.addMapping(SDL_SCANCODE_KP_HEXADECIMAL, "KP_Hexadecimal");
    bimap.addMapping(SDL_SCANCODE_KP_LEFTBRACE, "KP_LeftBrace");
    bimap.addMapping(SDL_SCANCODE_KP_LEFTPAREN, "KP_LeftParen");
    bimap.addMapping(SDL_SCANCODE_KP_LESS, "KP_Less");
    bimap.addMapping(SDL_SCANCODE_KP_MEMADD, "KP_MemAdd");
    bimap.addMapping(SDL_SCANCODE_KP_MEMCLEAR, "KP_MemClear");
    bimap.addMapping(SDL_SCANCODE_KP_MEMDIVIDE, "KP_MemDivide");
    bimap.addMapping(SDL_SCANCODE_KP_MEMMULTIPLY, "KP_MemMultiply");
    bimap.addMapping(SDL_SCANCODE_KP_MEMRECALL, "KP_MemRecall");
    bimap.addMapping(SDL_SCANCODE_KP_MEMSTORE, "KP_MemStore");
    bimap.addMapping(SDL_SCANCODE_KP_MEMSUBTRACT, "KP_MemSubtract");
    bimap.addMapping(SDL_SCANCODE_KP_MINUS, "KP_Minus");
    bimap.addMapping(SDL_SCANCODE_KP_MULTIPLY, "KP_Multiply");
    bimap.addMapping(SDL_SCANCODE_KP_OCTAL, "KP_Octal");
    bimap.addMapping(SDL_SCANCODE_KP_PERCENT, "KP_Percent");
    bimap.addMapping(SDL_SCANCODE_KP_PERIOD, "KP_Period");
    bimap.addMapping(SDL_SCANCODE_KP_PLUS, "KP_Plus");
    bimap.addMapping(SDL_SCANCODE_KP_PLUSMINUS, "KP_PlusMinus");
    bimap.addMapping(SDL_SCANCODE_KP_POWER, "KP_Power");
    bimap.addMapping(SDL_SCANCODE_KP_RIGHTBRACE, "KP_RightBrace");
    bimap.addMapping(SDL_SCANCODE_KP_RIGHTPAREN, "KP_RightParen");
    bimap.addMapping(SDL_SCANCODE_KP_SPACE, "KP_Space");
    bimap.addMapping(SDL_SCANCODE_KP_TAB, "KP_Tab");
    bimap.addMapping(SDL_SCANCODE_KP_VERTICALBAR, "KP_Verticalbar");
    bimap.addMapping(SDL_SCANCODE_KP_XOR, "KP_XOR");
    bimap.addMapping(SDL_SCANCODE_L, "L");
    bimap.addMapping(SDL_SCANCODE_LALT, "LAlt");
    bimap.addMapping(SDL_SCANCODE_LCTRL, "LCtrl");
    bimap.addMapping(SDL_SCANCODE_LEFT, "Left");
    bimap.addMapping(SDL_SCANCODE_LEFTBRACKET, "LeftBracket");
    bimap.addMapping(SDL_SCANCODE_LGUI, "LGUI");
    bimap.addMapping(SDL_SCANCODE_LSHIFT, "LShift");
    bimap.addMapping(SDL_SCANCODE_M, "M");
    bimap.addMapping(SDL_SCANCODE_MAIL, "Mail");
    bimap.addMapping(SDL_SCANCODE_MEDIASELECT, "MediaSelect");
    bimap.addMapping(SDL_SCANCODE_MENU, "Menu");
    bimap.addMapping(SDL_SCANCODE_MINUS, "Minus");
    bimap.addMapping(SDL_SCANCODE_MODE, "Mode");
    bimap.addMapping(SDL_SCANCODE_MUTE, "Mute");
    bimap.addMapping(SDL_SCANCODE_N, "N");
    bimap.addMapping(SDL_SCANCODE_NUMLOCKCLEAR, "NumLockClear");
    bimap.addMapping(SDL_SCANCODE_O, "O");
    bimap.addMapping(SDL_SCANCODE_OPER, "Oper");
    bimap.addMapping(SDL_SCANCODE_OUT, "Out");
    bimap.addMapping(SDL_SCANCODE_P, "P");
    bimap.addMapping(SDL_SCANCODE_PAGEDOWN, "PageDown");
    bimap.addMapping(SDL_SCANCODE_PAGEUP, "PageUp");
    bimap.addMapping(SDL_SCANCODE_PASTE, "Paste");
    bimap.addMapping(SDL_SCANCODE_PAUSE, "Pause");
    bimap.addMapping(SDL_SCANCODE_PERIOD, "Period");
    bimap.addMapping(SDL_SCANCODE_POWER, "Power");
    bimap.addMapping(SDL_SCANCODE_PRINTSCREEN, "PrintScreen");
    bimap.addMapping(SDL_SCANCODE_PRIOR, "Prior");
    bimap.addMapping(SDL_SCANCODE_Q, "Q");
    bimap.addMapping(SDL_SCANCODE_R, "R");
    bimap.addMapping(SDL_SCANCODE_RALT, "RAlt");
    bimap.addMapping(SDL_SCANCODE_RCTRL, "RCtrl");
    bimap.addMapping(SDL_SCANCODE_RETURN, "Return");
    bimap.addMapping(SDL_SCANCODE_RETURN2, "Return2");
    bimap.addMapping(SDL_SCANCODE_RGUI, "RGUI");
    bimap.addMapping(SDL_SCANCODE_RIGHT, "Right");
    bimap.addMapping(SDL_SCANCODE_RIGHTBRACKET, "RightBracket");
    bimap.addMapping(SDL_SCANCODE_RSHIFT, "RShift");
    bimap.addMapping(SDL_SCANCODE_S, "S");
    bimap.addMapping(SDL_SCANCODE_SCROLLLOCK, "ScrollLock");
    bimap.addMapping(SDL_SCANCODE_SELECT, "Select");
    bimap.addMapping(SDL_SCANCODE_SEMICOLON, "Semicolon");
    bimap.addMapping(SDL_SCANCODE_SEPARATOR, "Separator");
    bimap.addMapping(SDL_SCANCODE_SLASH, "Slash");
    bimap.addMapping(SDL_SCANCODE_SLEEP, "Sleep");
    bimap.addMapping(SDL_SCANCODE_SPACE, "Space");
    bimap.addMapping(SDL_SCANCODE_STOP, "Stop");
    bimap.addMapping(SDL_SCANCODE_SYSREQ, "SysReq");
    bimap.addMapping(SDL_SCANCODE_T, "T");
    bimap.addMapping(SDL_SCANCODE_TAB, "Tab");
    bimap.addMapping(SDL_SCANCODE_THOUSANDSSEPARATOR, "ThousandsSeparator");
    bimap.addMapping(SDL_SCANCODE_U, "U");
    bimap.addMapping(SDL_SCANCODE_UNDO, "Undo");
    bimap.addMapping(SDL_SCANCODE_UNKNOWN, "Unknown");
    bimap.addMapping(SDL_SCANCODE_UP, "Up");
    bimap.addMapping(SDL_SCANCODE_V, "V");
    bimap.addMapping(SDL_SCANCODE_VOLUMEDOWN, "VolumeDown");
    bimap.addMapping(SDL_SCANCODE_VOLUMEUP, "VolumeUp");
    bimap.addMapping(SDL_SCANCODE_W, "W");
    bimap.addMapping(SDL_SCANCODE_WWW, "WWW");
    bimap.addMapping(SDL_SCANCODE_X, "X");
    bimap.addMapping(SDL_SCANCODE_Y, "Y");
    bimap.addMapping(SDL_SCANCODE_Z, "Z");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL1, "International1");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL2, "International2");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL3, "International3");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL4, "International4");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL5, "International5");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL6, "International6");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL7, "International7");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL8, "International8");
    bimap.addMapping(SDL_SCANCODE_INTERNATIONAL9, "International9");
    bimap.addMapping(SDL_SCANCODE_LANG1, "Lang1");
    bimap.addMapping(SDL_SCANCODE_LANG2, "Lang2");
    bimap.addMapping(SDL_SCANCODE_LANG3, "Lang3");
    bimap.addMapping(SDL_SCANCODE_LANG4, "Lang4");
    bimap.addMapping(SDL_SCANCODE_LANG5, "Lang5");
    bimap.addMapping(SDL_SCANCODE_LANG6, "Lang6");
    bimap.addMapping(SDL_SCANCODE_LANG7, "Lang7");
    bimap.addMapping(SDL_SCANCODE_LANG8, "Lang8");
    bimap.addMapping(SDL_SCANCODE_LANG9, "Lang9");
    // bimap.addMapping(SDL_SCANCODE_LOCKINGCAPSLOCK, "LockingCapsLock");
    // bimap.addMapping(SDL_SCANCODE_LOCKINGNUMLOCK, "LockingNumLock");
    // bimap.addMapping(SDL_SCANCODE_LOCKINGSCROLLLOCK, "LockingScrollLock");
    bimap.addMapping(SDL_SCANCODE_NONUSBACKSLASH, "NonUSBackslash");
    bimap.addMapping(SDL_SCANCODE_NONUSHASH, "NonUSHash");

    return bimap;
}

static StringBimap<SDL_Scancode> keyboardMapSDL = initKeyboardMapSDL();

SDL_Scancode toSDLScancode(const std::string& str)
{
    return keyboardMapSDL.at(str);
}

const std::string& toString(SDL_Scancode scancode)
{
    return keyboardMapSDL.at(scancode);
}

StringBimap<SDL_GameControllerButton> initGamepadButtonMapSDL()
{
    StringBimap<SDL_GameControllerButton> bimap;

    bimap.addMapping(SDL_CONTROLLER_BUTTON_A, "A");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_B, "B");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_X, "X");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_Y, "Y");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_BACK, "Back");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_GUIDE, "Guide");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_START, "Start");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_LEFTSTICK, "LeftStick");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_RIGHTSTICK, "RightStick");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "LeftShoulder");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "RightShoulder");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_DPAD_UP, "DPadUp");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_DPAD_DOWN, "DPadDown");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_DPAD_LEFT, "DPadLeft");
    bimap.addMapping(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "DPadRight");

    return bimap;
}

static StringBimap<SDL_GameControllerButton> gamepadButtonMapSDL = initGamepadButtonMapSDL();

SDL_GameControllerButton toSDLGameControllerButton(const std::string& str)
{
    return gamepadButtonMapSDL.at(str);
}

const std::string& toString(SDL_GameControllerButton axis)
{
    return gamepadButtonMapSDL.at(axis);
}

StringBimap<SDL_GameControllerAxis> initGamepadAxisMapSDL()
{
    StringBimap<SDL_GameControllerAxis> bimap;

    bimap.addMapping(SDL_CONTROLLER_AXIS_LEFTX, "LeftX");
    bimap.addMapping(SDL_CONTROLLER_AXIS_LEFTY, "LeftY");
    bimap.addMapping(SDL_CONTROLLER_AXIS_RIGHTX, "RightX");
    bimap.addMapping(SDL_CONTROLLER_AXIS_RIGHTY, "RightY");
    bimap.addMapping(SDL_CONTROLLER_AXIS_TRIGGERLEFT, "LeftTrigger");
    bimap.addMapping(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, "RightTrigger");

    return bimap;
}

static StringBimap<SDL_GameControllerAxis> gamepadAxisMapSDL = initGamepadAxisMapSDL();

SDL_GameControllerAxis toSDLGameControllerAxis(const std::string& str)
{
    return gamepadAxisMapSDL.at(str);
}

const std::string& toString(SDL_GameControllerAxis axis)
{
    return gamepadAxisMapSDL.at(axis);
}
