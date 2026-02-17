/*
 * Stub header for telemetry_busmessage_sender.h
 * Used for L2 HAL Mock testing
 */
#ifndef TELEMETRY_BUSMESSAGE_SENDER_H
#define TELEMETRY_BUSMESSAGE_SENDER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Stub implementation - does nothing */
static inline void t2_init(char* component) {
    (void)component;
}

static inline void t2_event_s(const char* marker, const char* value) {
    (void)marker;
    (void)value;
}

static inline void t2_event_d(const char* marker, int value) {
    (void)marker;
    (void)value;
}

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_BUSMESSAGE_SENDER_H */
