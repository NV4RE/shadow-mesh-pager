#include "screen_calibration.h"

#include <lvgl.h>

#include "../config.h"
#include "../display/display_driver.h"

namespace screen_calibration {

namespace {

enum class Step { First, Second };

constexpr int TARGET_MARGIN = 30;
constexpr uint32_t POLL_PERIOD_MS = 30;

lv_obj_t *root = nullptr;
lv_obj_t *label = nullptr;
lv_obj_t *crosshair = nullptr;
lv_timer_t *pollTimer = nullptr;

Step step = Step::First;
bool wasTouchedLastPoll = false;
CompleteCallback onComplete;

// Target screen positions for the two taps (inset from the corners so taps
// near the physical bezel still land inside the panel's touch area).
const lv_point_t targetScreen[2] = {
    {TARGET_MARGIN, TARGET_MARGIN},
    {SCREEN_WIDTH - TARGET_MARGIN, SCREEN_HEIGHT - TARGET_MARGIN},
};
int sampledRawX[2] = {0, 0};
int sampledRawY[2] = {0, 0};

void moveCrosshairTo(int idx) {
    lv_obj_set_pos(crosshair, targetScreen[idx].x - 10, targetScreen[idx].y - 10);
    lv_label_set_text_fmt(label, "Tap the target (%d/2)", idx + 1);
}

void finish() {
    lv_timer_del(pollTimer);
    pollTimer = nullptr;

    // Solve the linear raw->screen mapping independently per axis from the
    // two (screen, raw) sample pairs, then invert it to get the raw values
    // that correspond to the full screen edges (x=0/W-1, y=0/H-1). Works
    // regardless of axis direction/inversion since it's derived, not assumed.
    settings::TouchCalibration cal;

    double scaleX = static_cast<double>(targetScreen[1].x - targetScreen[0].x) / (sampledRawX[1] - sampledRawX[0]);
    double rawAtScreen0X = sampledRawX[0] - targetScreen[0].x / scaleX;
    cal.xMin = static_cast<int>(rawAtScreen0X);
    cal.xMax = static_cast<int>(rawAtScreen0X + (SCREEN_WIDTH - 1) / scaleX);

    double scaleY = static_cast<double>(targetScreen[1].y - targetScreen[0].y) / (sampledRawY[1] - sampledRawY[0]);
    double rawAtScreen0Y = sampledRawY[0] - targetScreen[0].y / scaleY;
    cal.yMin = static_cast<int>(rawAtScreen0Y);
    cal.yMax = static_cast<int>(rawAtScreen0Y + (SCREEN_HEIGHT - 1) / scaleY);

    lv_obj_del(root);
    root = nullptr;

    CompleteCallback cb = onComplete;
    onComplete = nullptr;
    if (cb) {
        cb(cal);
    }
}

void pollCb(lv_timer_t *timer) {
    int rx = 0;
    int ry = 0;
    bool touched = display::readRawTouch(rx, ry);

    // Edge-triggered on the down transition so a held tap doesn't register twice.
    if (touched && !wasTouchedLastPoll) {
        int idx = step == Step::First ? 0 : 1;
        sampledRawX[idx] = rx;
        sampledRawY[idx] = ry;

        if (step == Step::First) {
            step = Step::Second;
            moveCrosshairTo(1);
        } else {
            finish();
            wasTouchedLastPoll = false;
            return;
        }
    }
    wasTouchedLastPoll = touched;
}

} // namespace

void run(CompleteCallback cb) {
    onComplete = std::move(cb);
    step = Step::First;
    wasTouchedLastPoll = false;

    root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(root, 0, 0);

    label = lv_label_create(root);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    crosshair = lv_obj_create(root);
    lv_obj_set_size(crosshair, 20, 20);
    lv_obj_set_style_radius(crosshair, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(crosshair, lv_color_hex(0xFF3B30), 0);

    moveCrosshairTo(0);

    pollTimer = lv_timer_create(pollCb, POLL_PERIOD_MS, nullptr);
}

} // namespace screen_calibration
