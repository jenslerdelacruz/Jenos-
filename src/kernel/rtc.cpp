#include "rtc.h"
#include <stdint.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static inline uint8_t inb_rtc(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb_rtc(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static int get_update_in_progress_flag() {
      outb_rtc(CMOS_ADDRESS, 0x0A);
      return (inb_rtc(CMOS_DATA) & 0x80);
}

static uint8_t get_rtc_register(int reg) {
      outb_rtc(CMOS_ADDRESS, reg);
      return inb_rtc(CMOS_DATA);
}

void rtc_read_time(int* hours, int* minutes, int* seconds) {
      uint8_t second, minute, hour;
      uint8_t last_second, last_minute, last_hour;
      uint8_t registerB;

      while (get_update_in_progress_flag());
      second = get_rtc_register(0x00);
      minute = get_rtc_register(0x02);
      hour = get_rtc_register(0x04);

      do {
            last_second = second;
            last_minute = minute;
            last_hour = hour;

            while (get_update_in_progress_flag());
            second = get_rtc_register(0x00);
            minute = get_rtc_register(0x02);
            hour = get_rtc_register(0x04);
      } while( (last_second != second) || (last_minute != minute) || (last_hour != hour) );

      registerB = get_rtc_register(0x0B);

      // Convert BCD to binary values if necessary
      if (!(registerB & 0x04)) {
            second = (second & 0x0F) + ((second / 16) * 10);
            minute = (minute & 0x0F) + ((minute / 16) * 10);
            hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
      }

      // Convert 12 hour clock to 24 hour clock if necessary
      if (!(registerB & 0x02) && (hour & 0x80)) {
            hour = ((hour & 0x7F) + 12) % 24;
      }

      // Adjust to local time (+8 for Philippines, wrap around 24)
      hour = (hour + 8) % 24;

      if (seconds) *seconds = second;
      if (minutes) *minutes = minute;
      if (hours) *hours = hour;
}
