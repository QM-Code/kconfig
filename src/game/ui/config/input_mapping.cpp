#include "ui/config/input_mapping.hpp"

#include "karma/window/window.hpp"

namespace ui::input::mapping {

#if defined(KARMA_UI_BACKEND_IMGUI)
ImGuiKey ToImGuiKey(window::Key key) {
    switch (key) {
        case window::Key::A: return ImGuiKey_A;
        case window::Key::B: return ImGuiKey_B;
        case window::Key::C: return ImGuiKey_C;
        case window::Key::D: return ImGuiKey_D;
        case window::Key::E: return ImGuiKey_E;
        case window::Key::F: return ImGuiKey_F;
        case window::Key::G: return ImGuiKey_G;
        case window::Key::H: return ImGuiKey_H;
        case window::Key::I: return ImGuiKey_I;
        case window::Key::J: return ImGuiKey_J;
        case window::Key::K: return ImGuiKey_K;
        case window::Key::L: return ImGuiKey_L;
        case window::Key::M: return ImGuiKey_M;
        case window::Key::N: return ImGuiKey_N;
        case window::Key::O: return ImGuiKey_O;
        case window::Key::P: return ImGuiKey_P;
        case window::Key::Q: return ImGuiKey_Q;
        case window::Key::R: return ImGuiKey_R;
        case window::Key::S: return ImGuiKey_S;
        case window::Key::T: return ImGuiKey_T;
        case window::Key::U: return ImGuiKey_U;
        case window::Key::V: return ImGuiKey_V;
        case window::Key::W: return ImGuiKey_W;
        case window::Key::X: return ImGuiKey_X;
        case window::Key::Y: return ImGuiKey_Y;
        case window::Key::Z: return ImGuiKey_Z;
        case window::Key::Num0: return ImGuiKey_0;
        case window::Key::Num1: return ImGuiKey_1;
        case window::Key::Num2: return ImGuiKey_2;
        case window::Key::Num3: return ImGuiKey_3;
        case window::Key::Num4: return ImGuiKey_4;
        case window::Key::Num5: return ImGuiKey_5;
        case window::Key::Num6: return ImGuiKey_6;
        case window::Key::Num7: return ImGuiKey_7;
        case window::Key::Num8: return ImGuiKey_8;
        case window::Key::Num9: return ImGuiKey_9;
        case window::Key::F1: return ImGuiKey_F1;
        case window::Key::F2: return ImGuiKey_F2;
        case window::Key::F3: return ImGuiKey_F3;
        case window::Key::F4: return ImGuiKey_F4;
        case window::Key::F5: return ImGuiKey_F5;
        case window::Key::F6: return ImGuiKey_F6;
        case window::Key::F7: return ImGuiKey_F7;
        case window::Key::F8: return ImGuiKey_F8;
        case window::Key::F9: return ImGuiKey_F9;
        case window::Key::F10: return ImGuiKey_F10;
        case window::Key::F11: return ImGuiKey_F11;
        case window::Key::F12: return ImGuiKey_F12;
        case window::Key::F13: return ImGuiKey_F13;
        case window::Key::F14: return ImGuiKey_F14;
        case window::Key::F15: return ImGuiKey_F15;
        case window::Key::F16: return ImGuiKey_F16;
        case window::Key::F17: return ImGuiKey_F17;
        case window::Key::F18: return ImGuiKey_F18;
        case window::Key::F19: return ImGuiKey_F19;
        case window::Key::F20: return ImGuiKey_F20;
        case window::Key::F21: return ImGuiKey_F21;
        case window::Key::F22: return ImGuiKey_F22;
        case window::Key::F23: return ImGuiKey_F23;
        case window::Key::F24: return ImGuiKey_F24;
        case window::Key::Space: return ImGuiKey_Space;
        case window::Key::Escape: return ImGuiKey_Escape;
        case window::Key::Enter: return ImGuiKey_Enter;
        case window::Key::Tab: return ImGuiKey_Tab;
        case window::Key::Backspace: return ImGuiKey_Backspace;
        case window::Key::Left: return ImGuiKey_LeftArrow;
        case window::Key::Right: return ImGuiKey_RightArrow;
        case window::Key::Up: return ImGuiKey_UpArrow;
        case window::Key::Down: return ImGuiKey_DownArrow;
        case window::Key::LeftBracket: return ImGuiKey_LeftBracket;
        case window::Key::RightBracket: return ImGuiKey_RightBracket;
        case window::Key::Minus: return ImGuiKey_Minus;
        case window::Key::Equal: return ImGuiKey_Equal;
        case window::Key::Apostrophe: return ImGuiKey_Apostrophe;
        case window::Key::GraveAccent: return ImGuiKey_GraveAccent;
        case window::Key::LeftShift: return ImGuiKey_LeftShift;
        case window::Key::RightShift: return ImGuiKey_RightShift;
        case window::Key::LeftControl: return ImGuiKey_LeftCtrl;
        case window::Key::RightControl: return ImGuiKey_RightCtrl;
        case window::Key::LeftAlt: return ImGuiKey_LeftAlt;
        case window::Key::RightAlt: return ImGuiKey_RightAlt;
        case window::Key::LeftSuper: return ImGuiKey_LeftSuper;
        case window::Key::RightSuper: return ImGuiKey_RightSuper;
        case window::Key::Menu: return ImGuiKey_Menu;
        case window::Key::Home: return ImGuiKey_Home;
        case window::Key::End: return ImGuiKey_End;
        case window::Key::PageUp: return ImGuiKey_PageUp;
        case window::Key::PageDown: return ImGuiKey_PageDown;
        case window::Key::Insert: return ImGuiKey_Insert;
        case window::Key::Delete: return ImGuiKey_Delete;
        default: return ImGuiKey_None;
    }
}

int ToImGuiMouseButton(window::MouseButton button) {
    switch (button) {
        case window::MouseButton::Left: return 0;
        case window::MouseButton::Right: return 1;
        case window::MouseButton::Middle: return 2;
        case window::MouseButton::Button4: return 3;
        case window::MouseButton::Button5: return 4;
        case window::MouseButton::Button6: return 5;
        case window::MouseButton::Button7: return 6;
        case window::MouseButton::Button8: return 7;
        default: return 0;
    }
}

void UpdateImGuiModifiers(ImGuiIO &io, window::Window *window) {
    if (!window) {
        return;
    }
    const bool shift = window->isKeyDown(window::Key::LeftShift) || window->isKeyDown(window::Key::RightShift);
    const bool ctrl = window->isKeyDown(window::Key::LeftControl) || window->isKeyDown(window::Key::RightControl);
    const bool alt = window->isKeyDown(window::Key::LeftAlt) || window->isKeyDown(window::Key::RightAlt);
    const bool super = window->isKeyDown(window::Key::LeftSuper) || window->isKeyDown(window::Key::RightSuper);
    io.AddKeyEvent(ImGuiKey_ModShift, shift);
    io.AddKeyEvent(ImGuiKey_ModCtrl, ctrl);
    io.AddKeyEvent(ImGuiKey_ModAlt, alt);
    io.AddKeyEvent(ImGuiKey_ModSuper, super);
}
#endif

#if defined(KARMA_UI_BACKEND_RMLUI)
Rml::Input::KeyIdentifier ToRmlKey(window::Key key) {
    switch (key) {
        case window::Key::A: return Rml::Input::KI_A;
        case window::Key::B: return Rml::Input::KI_B;
        case window::Key::C: return Rml::Input::KI_C;
        case window::Key::D: return Rml::Input::KI_D;
        case window::Key::E: return Rml::Input::KI_E;
        case window::Key::F: return Rml::Input::KI_F;
        case window::Key::G: return Rml::Input::KI_G;
        case window::Key::H: return Rml::Input::KI_H;
        case window::Key::I: return Rml::Input::KI_I;
        case window::Key::J: return Rml::Input::KI_J;
        case window::Key::K: return Rml::Input::KI_K;
        case window::Key::L: return Rml::Input::KI_L;
        case window::Key::M: return Rml::Input::KI_M;
        case window::Key::N: return Rml::Input::KI_N;
        case window::Key::O: return Rml::Input::KI_O;
        case window::Key::P: return Rml::Input::KI_P;
        case window::Key::Q: return Rml::Input::KI_Q;
        case window::Key::R: return Rml::Input::KI_R;
        case window::Key::S: return Rml::Input::KI_S;
        case window::Key::T: return Rml::Input::KI_T;
        case window::Key::U: return Rml::Input::KI_U;
        case window::Key::V: return Rml::Input::KI_V;
        case window::Key::W: return Rml::Input::KI_W;
        case window::Key::X: return Rml::Input::KI_X;
        case window::Key::Y: return Rml::Input::KI_Y;
        case window::Key::Z: return Rml::Input::KI_Z;
        case window::Key::Num0: return Rml::Input::KI_0;
        case window::Key::Num1: return Rml::Input::KI_1;
        case window::Key::Num2: return Rml::Input::KI_2;
        case window::Key::Num3: return Rml::Input::KI_3;
        case window::Key::Num4: return Rml::Input::KI_4;
        case window::Key::Num5: return Rml::Input::KI_5;
        case window::Key::Num6: return Rml::Input::KI_6;
        case window::Key::Num7: return Rml::Input::KI_7;
        case window::Key::Num8: return Rml::Input::KI_8;
        case window::Key::Num9: return Rml::Input::KI_9;
        case window::Key::F1: return Rml::Input::KI_F1;
        case window::Key::F2: return Rml::Input::KI_F2;
        case window::Key::F3: return Rml::Input::KI_F3;
        case window::Key::F4: return Rml::Input::KI_F4;
        case window::Key::F5: return Rml::Input::KI_F5;
        case window::Key::F6: return Rml::Input::KI_F6;
        case window::Key::F7: return Rml::Input::KI_F7;
        case window::Key::F8: return Rml::Input::KI_F8;
        case window::Key::F9: return Rml::Input::KI_F9;
        case window::Key::F10: return Rml::Input::KI_F10;
        case window::Key::F11: return Rml::Input::KI_F11;
        case window::Key::F12: return Rml::Input::KI_F12;
        case window::Key::F13: return Rml::Input::KI_F13;
        case window::Key::F14: return Rml::Input::KI_F14;
        case window::Key::F15: return Rml::Input::KI_F15;
        case window::Key::F16: return Rml::Input::KI_F16;
        case window::Key::F17: return Rml::Input::KI_F17;
        case window::Key::F18: return Rml::Input::KI_F18;
        case window::Key::F19: return Rml::Input::KI_F19;
        case window::Key::F20: return Rml::Input::KI_F20;
        case window::Key::F21: return Rml::Input::KI_F21;
        case window::Key::F22: return Rml::Input::KI_F22;
        case window::Key::F23: return Rml::Input::KI_F23;
        case window::Key::F24: return Rml::Input::KI_F24;
        case window::Key::Space: return Rml::Input::KI_SPACE;
        case window::Key::Escape: return Rml::Input::KI_ESCAPE;
        case window::Key::Enter: return Rml::Input::KI_RETURN;
        case window::Key::Tab: return Rml::Input::KI_TAB;
        case window::Key::Backspace: return Rml::Input::KI_BACK;
        case window::Key::Left: return Rml::Input::KI_LEFT;
        case window::Key::Right: return Rml::Input::KI_RIGHT;
        case window::Key::Up: return Rml::Input::KI_UP;
        case window::Key::Down: return Rml::Input::KI_DOWN;
        case window::Key::LeftBracket: return Rml::Input::KI_OEM_4;
        case window::Key::RightBracket: return Rml::Input::KI_OEM_6;
        case window::Key::Minus: return Rml::Input::KI_OEM_MINUS;
        case window::Key::Equal: return Rml::Input::KI_OEM_PLUS;
        case window::Key::Apostrophe: return Rml::Input::KI_OEM_7;
        case window::Key::GraveAccent: return Rml::Input::KI_OEM_3;
        case window::Key::LeftShift: return Rml::Input::KI_LSHIFT;
        case window::Key::RightShift: return Rml::Input::KI_RSHIFT;
        case window::Key::LeftControl: return Rml::Input::KI_LCONTROL;
        case window::Key::RightControl: return Rml::Input::KI_RCONTROL;
        case window::Key::LeftAlt: return Rml::Input::KI_LMENU;
        case window::Key::RightAlt: return Rml::Input::KI_RMENU;
        case window::Key::LeftSuper: return Rml::Input::KI_LMETA;
        case window::Key::RightSuper: return Rml::Input::KI_RMETA;
        case window::Key::Home: return Rml::Input::KI_HOME;
        case window::Key::End: return Rml::Input::KI_END;
        case window::Key::PageUp: return Rml::Input::KI_PRIOR;
        case window::Key::PageDown: return Rml::Input::KI_NEXT;
        case window::Key::Insert: return Rml::Input::KI_INSERT;
        case window::Key::Delete: return Rml::Input::KI_DELETE;
        case window::Key::CapsLock: return Rml::Input::KI_CAPITAL;
        case window::Key::NumLock: return Rml::Input::KI_NUMLOCK;
        case window::Key::ScrollLock: return Rml::Input::KI_SCROLL;
        default: return Rml::Input::KI_UNKNOWN;
    }
}

int ToRmlMods(const window::Modifiers &mods) {
    int out = 0;
    if (mods.control) {
        out |= Rml::Input::KM_CTRL;
    }
    if (mods.shift) {
        out |= Rml::Input::KM_SHIFT;
    }
    if (mods.alt) {
        out |= Rml::Input::KM_ALT;
    }
    if (mods.super) {
        out |= Rml::Input::KM_META;
    }
    return out;
}

int CurrentRmlMods(window::Window *window) {
    if (!window) {
        return 0;
    }
    window::Modifiers mods;
    mods.shift = window->isKeyDown(window::Key::LeftShift) || window->isKeyDown(window::Key::RightShift);
    mods.control = window->isKeyDown(window::Key::LeftControl) || window->isKeyDown(window::Key::RightControl);
    mods.alt = window->isKeyDown(window::Key::LeftAlt) || window->isKeyDown(window::Key::RightAlt);
    mods.super = window->isKeyDown(window::Key::LeftSuper) || window->isKeyDown(window::Key::RightSuper);
    return ToRmlMods(mods);
}

int RmlModsForEvent(const window::Event &event, window::Window *window) {
    const int mods = ToRmlMods(event.mods);
    if (mods == 0) {
        return CurrentRmlMods(window);
    }
    return mods;
}

int ToRmlMouseButton(window::MouseButton button) {
    switch (button) {
        case window::MouseButton::Left: return 0;
        case window::MouseButton::Right: return 1;
        case window::MouseButton::Middle: return 2;
        case window::MouseButton::Button4: return 3;
        case window::MouseButton::Button5: return 4;
        case window::MouseButton::Button6: return 5;
        case window::MouseButton::Button7: return 6;
        case window::MouseButton::Button8: return 7;
        default: return 0;
    }
}
#endif

} // namespace ui::input::mapping
