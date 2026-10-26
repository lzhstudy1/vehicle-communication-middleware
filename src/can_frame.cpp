#include "vcm/can_frame.hpp"

#include <cstring>

CanFrame CanFrame::fromRaw(const struct can_frame& raw) {
    CanFrame f;
    f.is_extended = (raw.can_id & CAN_EFF_FLAG) != 0;
    f.is_rtr      = (raw.can_id & CAN_RTR_FLAG) != 0;

    if (f.is_extended) {
        f.id = raw.can_id & CAN_EFF_MASK;
    } else {
        f.id = raw.can_id & CAN_SFF_MASK;
    }

    f.dlc = raw.can_dlc;
    std::memcpy(f.data, raw.data, 8);
    return f;
}

struct can_frame CanFrame::toRaw() const {
    struct can_frame raw;
    std::memset(&raw, 0, sizeof(raw));

    uint32_t can_id = id;
    if (is_extended) {
        can_id |= CAN_EFF_FLAG;
    }
    if (is_rtr) {
        can_id |= CAN_RTR_FLAG;
    }
    raw.can_id = can_id;
    raw.can_dlc = dlc;
    std::memcpy(raw.data, data, 8);
    return raw;
}