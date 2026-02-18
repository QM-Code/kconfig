#include "ui/backends/imgui/internal.hpp"

#if defined(KARMA_HAS_IMGUI)

namespace karma::ui::backend::imgui {

ImGuiKey MapKey(window::Key key) {
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
        case window::Key::Minus: return ImGuiKey_Minus;
        case window::Key::Equals: return ImGuiKey_Equal;
        case window::Key::LeftBracket: return ImGuiKey_LeftBracket;
        case window::Key::RightBracket: return ImGuiKey_RightBracket;
        case window::Key::Backslash: return ImGuiKey_Backslash;
        case window::Key::Semicolon: return ImGuiKey_Semicolon;
        case window::Key::Apostrophe: return ImGuiKey_Apostrophe;
        case window::Key::Comma: return ImGuiKey_Comma;
        case window::Key::Slash: return ImGuiKey_Slash;
        case window::Key::Grave: return ImGuiKey_GraveAccent;
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
        case window::Key::CapsLock: return ImGuiKey_CapsLock;
        case window::Key::NumLock: return ImGuiKey_NumLock;
        case window::Key::ScrollLock: return ImGuiKey_ScrollLock;
        case window::Key::Left: return ImGuiKey_LeftArrow;
        case window::Key::Right: return ImGuiKey_RightArrow;
        case window::Key::Up: return ImGuiKey_UpArrow;
        case window::Key::Down: return ImGuiKey_DownArrow;
        case window::Key::LeftShift: return ImGuiKey_LeftShift;
        case window::Key::RightShift: return ImGuiKey_RightShift;
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
        case window::Key::Enter: return ImGuiKey_Enter;
        case window::Key::Space: return ImGuiKey_Space;
        case window::Key::Tab: return ImGuiKey_Tab;
        case window::Key::Period: return ImGuiKey_Period;
        case window::Key::Backspace: return ImGuiKey_Backspace;
        case window::Key::Escape: return ImGuiKey_Escape;
        case window::Key::Unknown:
        default: return ImGuiKey_None;
    }
}

int MapMouseButton(window::MouseButton button) {
    switch (button) {
        case window::MouseButton::Left: return ImGuiMouseButton_Left;
        case window::MouseButton::Right: return ImGuiMouseButton_Right;
        case window::MouseButton::Middle: return ImGuiMouseButton_Middle;
        case window::MouseButton::Button4: return 3;
        case window::MouseButton::Button5: return 4;
        default: return -1;
    }
}

void PushModifiers(ImGuiIO& io, const window::Modifiers& mods) {
    io.AddKeyEvent(ImGuiMod_Shift, mods.shift);
    io.AddKeyEvent(ImGuiMod_Ctrl, mods.ctrl);
    io.AddKeyEvent(ImGuiMod_Alt, mods.alt);
    io.AddKeyEvent(ImGuiMod_Super, mods.super);
}

} // namespace karma::ui::backend::imgui

#endif // defined(KARMA_HAS_IMGUI)
