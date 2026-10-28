#include "control_logic.h"
#include <cassert>
#include <iostream>

int main() {
    miran::Scheduler s;
    miran::MotionConfig c;
    assert(!s.tick(0, false, true, true, true, c));
    assert(!s.tick(0, true, false, true, true, c));
    assert(!s.tick(0, true, true, true, false, c));
    assert(!s.tick(0, true, true, false, true, c));
    auto first = s.tick(100, true, true, true, true, c);
    assert(first && first->x == 0 && first->y == 5);
    assert(!s.tick(139, true, true, true, true, c));
    assert(s.tick(140, true, true, true, true, c));
    assert(s.tick(10000, true, true, true, true, c));
    assert(!s.tick(10001, true, true, true, true, c));
    assert(!s.tick(10002, true, false, true, true, c));
    assert(s.tick(10003, true, true, true, true, c));
    assert(!s.tick(10004, false, true, true, true, c));
    assert(s.tick(10005, true, true, true, true, c));
    c.both_buttons = false;
    s.reset();
    assert(s.tick(1, true, true, true, false, c));
    c.horizontal = -1000; c.vertical = 999; c.interval_ms = -99;
    s.reset();
    auto clamped = s.tick(20, true, true, true, true, c);
    assert(clamped && clamped->x == -15 && clamped->y == 30);
    assert(!s.tick(29, true, true, true, true, c));
    assert(s.tick(30, true, true, true, true, c));
    c.horizontal = c.vertical = 0;
    assert(!s.tick(40, true, true, true, true, c));
    std::cout << "PASS: gating, interval, release/reset, no catch-up, zero movement, bounds\n";
}
