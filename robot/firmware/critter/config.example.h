// Copy this file to config.h and fill it in. config.h is not committed to git.
#pragma once

// Leave WIFI_SSID empty to run offline: eyes only, button cycles through moods.
#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

// The computer running the relay (it prints its address when it starts).
#define RELAY_HOST  "192.168.1.50"
#define RELAY_PORT  8787
#define RELAY_TOKEN ""          // must match CRITTER_TOKEN on the relay, if set

// The button used as a "poke". GPIO 0 is the BOOT button already on the board.
#define BUTTON_PIN 0

// Airflow ("puff") sensor on an analog pin, or -1 if not fitted.
#define PUFF_PIN       -1
#define PUFF_THRESHOLD 400      // how far the reading must jump to count as a puff
#define PUFF_DEBUG     0        // 1 = print raw readings to the serial monitor
