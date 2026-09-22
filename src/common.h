#ifndef COMMON_H
#define COMMON_H

#include "prim.h"
#include "layout.h"
#include "materials.h"

namespace scene {

using namespace cfg;
using namespace prim;
using lay::LAY;

enum {
    L_FLOOR = 0,        // fixtures.h
    L_MOTOR,            // fixtures.h
    L_FIXTURES,         // fixtures.h - magazine and exit hood
    L_STACK_SEG,        // fixtures.h - one stack-light lens
    L_PANEL,            // press.h    - drive panel, guide rails, brackets
    L_TOOTH,            // gears.h    - one tooth, instanced 124 times
    L_GENEVA,           // geneva.h   - the four-slot wheel
    L_CONVEYOR,         // conveyor.h - frame and legs
    L_BELT,             // conveyor.h - strips and the two half-shells
    L_CLEAT,            // conveyor.h - one cleat, instanced 24 times
    L_ROLLER,           // conveyor.h
    L_BLANK,            // blanks.h   - one blank, instanced up to 10 times
    L_GEAR0,            // gears.h    - five bodies, L_GEAR0 + 0..4
    L_COUNT = L_GEAR0 + 5
};

inline GLuint g_list = 0;
inline GLuint L(int i) { return g_list + (GLuint)i; }

} // namespace scene
#endif
