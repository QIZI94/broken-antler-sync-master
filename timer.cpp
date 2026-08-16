#include "timer.h"

#include <Arduino.h>

uint32_t rtcNow() {
	return uint32_t(esp_timer_get_time() / 1ULL);
}
