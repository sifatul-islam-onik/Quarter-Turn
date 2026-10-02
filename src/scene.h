
#ifndef SCENE_H
#define SCENE_H

#include "common.h"
#include "gears.h"
#include "press.h"
#include "geneva.h"
#include "conveyor.h"
#include "blanks.h"
#include "fixtures.h"

namespace scene {

inline void build_lists() {
    build_fixtures();
    build_press();
    build_gears();
    build_geneva();
    build_conveyor();
    build_blanks();
}

inline void draw(float th, long turns) {
    const float B = lay::belt_travel(th, turns);   // belt travel, in stations

    glCallList(floor_list);
    glCallList(panel_list);
    glCallList(frame_list);
    glCallList(motor_list);
    glCallList(belt_list);

    draw_gears(th);           // gears.h
    draw_press(th);           // press.h     - chains A and B
    draw_geneva_driver(th);   // geneva.h    - chain C
    draw_conveyor(B);         // conveyor.h  - chain D, and the Geneva wheel
    draw_blanks(th, B);       // blanks.h    - the squash branch of chain D
    draw_stack_light();       // fixtures.h
}

} // namespace scene
#endif
