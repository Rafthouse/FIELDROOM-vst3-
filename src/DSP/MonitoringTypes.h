#pragma once

namespace fieldroom {

/**
 * Mutually exclusive listening audition modes.
 * Note: L/R Swap and DIM are independent monitoring toggles.
 */
enum class MonitoringMode : int {
    Normal = 0,
    Mono = 1,
    Side = 2,
    LeftSolo = 3,
    RightSolo = 4
};

} // namespace fieldroom
