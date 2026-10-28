#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>

namespace miran {
struct MotionConfig {
    int horizontal = 0;
    int vertical = 5;
    int interval_ms = 40;
    bool both_buttons = true;
    void clamp() {
        horizontal = std::clamp(horizontal, -15, 15);
        vertical = std::clamp(vertical, 0, 30);
        interval_ms = std::clamp(interval_ms, 10, 200);
    }
};
struct Movement { int x; int y; };
class Scheduler {
    std::uint64_t next_ = 0;
    bool active_ = false;
public:
    void reset() { next_ = 0; active_ = false; }
    std::optional<Movement> tick(std::uint64_t now, bool enabled, bool game_foreground,
                                  bool left, bool right, MotionConfig config) {
        config.clamp();
        if (!enabled || !game_foreground || !left || (config.both_buttons && !right)) {
            reset();
            return std::nullopt;
        }
        if (config.horizontal == 0 && config.vertical == 0) {
            reset();
            return std::nullopt;
        }
        if (active_ && now < next_) return std::nullopt;
        active_ = true;
        // Never replay missed movements after a delayed UI message.
        next_ = now + static_cast<std::uint64_t>(config.interval_ms);
        return Movement{config.horizontal, config.vertical};
    }
};
} // namespace miran
