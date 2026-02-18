#include "ui/backends/rmlui/internal.hpp"

#if defined(KARMA_HAS_RMLUI)

namespace karma::ui::backend::rmlui {

int MapModifiers(const window::Modifiers& mods) {
    int flags = 0;
    if (mods.ctrl) {
        flags |= Rml::Input::KM_CTRL;
    }
    if (mods.shift) {
        flags |= Rml::Input::KM_SHIFT;
    }
    if (mods.alt) {
        flags |= Rml::Input::KM_ALT;
    }
    if (mods.super) {
        flags |= Rml::Input::KM_META;
    }
    return flags;
}

Rml::Input::KeyIdentifier MapKey(window::Key key) {
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
        case window::Key::Minus: return Rml::Input::KI_OEM_MINUS;
        case window::Key::Equals: return Rml::Input::KI_OEM_PLUS;
        case window::Key::LeftBracket: return Rml::Input::KI_OEM_4;
        case window::Key::RightBracket: return Rml::Input::KI_OEM_6;
        case window::Key::Backslash: return Rml::Input::KI_OEM_5;
        case window::Key::Semicolon: return Rml::Input::KI_OEM_1;
        case window::Key::Apostrophe: return Rml::Input::KI_OEM_7;
        case window::Key::Comma: return Rml::Input::KI_OEM_COMMA;
        case window::Key::Slash: return Rml::Input::KI_OEM_2;
        case window::Key::Grave: return Rml::Input::KI_OEM_3;
        case window::Key::LeftControl: return Rml::Input::KI_LCONTROL;
        case window::Key::RightControl: return Rml::Input::KI_RCONTROL;
        case window::Key::LeftAlt: return Rml::Input::KI_LMENU;
        case window::Key::RightAlt: return Rml::Input::KI_RMENU;
        case window::Key::LeftSuper: return Rml::Input::KI_LMETA;
        case window::Key::RightSuper: return Rml::Input::KI_RMETA;
        case window::Key::Menu: return Rml::Input::KI_APPS;
        case window::Key::Home: return Rml::Input::KI_HOME;
        case window::Key::End: return Rml::Input::KI_END;
        case window::Key::PageUp: return Rml::Input::KI_PRIOR;
        case window::Key::PageDown: return Rml::Input::KI_NEXT;
        case window::Key::Insert: return Rml::Input::KI_INSERT;
        case window::Key::Delete: return Rml::Input::KI_DELETE;
        case window::Key::CapsLock: return Rml::Input::KI_CAPITAL;
        case window::Key::NumLock: return Rml::Input::KI_NUMLOCK;
        case window::Key::ScrollLock: return Rml::Input::KI_SCROLL;
        case window::Key::Left: return Rml::Input::KI_LEFT;
        case window::Key::Right: return Rml::Input::KI_RIGHT;
        case window::Key::Up: return Rml::Input::KI_UP;
        case window::Key::Down: return Rml::Input::KI_DOWN;
        case window::Key::LeftShift: return Rml::Input::KI_LSHIFT;
        case window::Key::RightShift: return Rml::Input::KI_RSHIFT;
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
        case window::Key::Enter: return Rml::Input::KI_RETURN;
        case window::Key::Space: return Rml::Input::KI_SPACE;
        case window::Key::Tab: return Rml::Input::KI_TAB;
        case window::Key::Period: return Rml::Input::KI_OEM_PERIOD;
        case window::Key::Backspace: return Rml::Input::KI_BACK;
        case window::Key::Escape: return Rml::Input::KI_ESCAPE;
        case window::Key::Unknown:
        default: return Rml::Input::KI_UNKNOWN;
    }
}

int MapMouseButton(window::MouseButton button) {
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

} // namespace karma::ui::backend::rmlui

#endif // defined(KARMA_HAS_RMLUI)
