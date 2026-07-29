"""
lvgl 9.5.0's lv_blend_helium.S unconditionally includes headers that pull in
the toolchain's <stdint.h>, whose typedefs leak straight into the assembler
input regardless of the LV_USE_DRAW_SW_ASM guard -- breaking the build on
any non-ARM assembler (including Xtensa/ESP32). The file is ARM Helium SIMD
code, never relevant on this target, so it's safe to neuter outright.

Runs pre-build every time so the fix survives a `.pio` wipe and dependency
reinstall.
"""

import os

Import("env")

MARKER = "/* world-end-pagger: stripped, ARM Helium asm not applicable on Xtensa/ESP32 */\n"

target = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"),
    env.subst("$PIOENV"),
    "lvgl", "src", "draw", "sw", "blend", "helium", "lv_blend_helium.S",
)

if os.path.isfile(target):
    with open(target, "r") as f:
        content = f.read()
    if content != MARKER:
        with open(target, "w") as f:
            f.write(MARKER)
