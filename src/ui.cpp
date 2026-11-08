#include "chili/ui.hpp"

namespace chili {

void (*ui_sound)(UiSound sound) = nullptr;
Theme theme;

void play_ui_sound(UiSound sound) {
  if(ui_sound) ui_sound(sound);
}

}
