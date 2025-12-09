// Schedule.hpp
#ifndef SCHEDULE_HPP
#define SCHEDULE_HPP

#include <Arduino.h>
#include <vector>

struct IrrigationSchedule {
  uint8_t evId;
  uint8_t startHour;
  uint8_t startMinute;
  uint8_t endHour;
  uint8_t endMinute;
  bool enabled = true;

  String toString() const {
    char buf[30];
    sprintf(buf, "EV%d %02d:%02d-%02d:%02d", evId, startHour, startMinute, endHour, endMinute);
    return String(buf);
  }
};

class ScheduleManager {
public:
  std::vector<IrrigationSchedule> schedules;

  bool load();
  bool save();
  void clear();
  void addOrUpdate(const IrrigationSchedule& sched);
  bool remove(uint8_t evId);
  String toJson() const;
};

// Global instance (declared in .cpp)
extern ScheduleManager scheduleManager;

#endif