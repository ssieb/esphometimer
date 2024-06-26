/****
Copyright (c) 2024 RebbePod

This library is free software; you can redistribute it and/or modify it 
under the terms of the GNU Lesser GeneralPublic License as published by the Free Software Foundation; 
either version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; 
without even the impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU Lesser General Public License for more details. 
You should have received a copy of the GNU Lesser General Public License along with this library; 
if not, write tothe Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA, 
or connect to: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
****/

/***************************************************
*      ***** Timer Format Cheat Guide *****
* **************************************************
*              ***** Switches *****
*   *** Value set to '1' enabled '0' disabled ***
* 0 = Enabled
* 1-7 = Days (sun-sat)
* 8 = Repeat
* 9 = Negative Offset
*               ***** Numbers *****
* 10 = Time hour
* 11 = Time minute
* 12 = Output  {value '0' for the first position of switch in the 'relays' variable}
* 13 = Action  {'0' turn off, '1' turn on, '2' toggle}
* 14 = Offset hour
* 15 = Offset minute
* 16 = Mode    {'0' use time, '1' use sunrise, '2' use sunset} 
* sample "Live;1,Mode;0,Time;2:36,Repeat;1,Days;SMTWTFS,Output;1,Action;2,Offset;-0:30,"
****************************************************/

#pragma once
#include <string>

struct Timer {
    bool live : 1;
    bool repeat : 1;
    bool use_negative_offset : 1;
    union {
        struct {
            bool sun : 1;
            bool mon : 1;
            bool tue : 1;
            bool wed : 1;
            bool thu : 1;
            bool fri : 1;
            bool sat : 1;
        } day;
        uint8_t raw;
    } days;
    uint8_t mode : 2; // Using 2 bits to give up to 4 options
    uint8_t action : 2; // Using 2 bits to give up to 4 options
    uint8_t output : 2; // Using 2 bits to give up to 4 options
    uint8_t hour : 5; // Using 5 bits to represent hours (0-23)
    uint8_t minute : 6; // Using 6 bits to represent minutes (0-59)
    uint8_t offset_hour : 5; // Using 5 bits to represent hours (0-23)
    uint8_t offset_minute : 6; // Using 6 bits to represent minutes (0-59)
    time_t last_ran_timestamp; // Uses 32 bits


    std::string to_string() const {
        std::string result = "Live;" + std::to_string(live) +
                             ",Mode;" + std::to_string(mode);

        // Include time only if mode is 0
        if (mode == 0) {
            result += ",Time;" + std::to_string(hour) + ":" +
                      (minute < 10 ? "0" + std::to_string(minute) : std::to_string(minute));
        }

        result += ",Repeat;" + std::to_string(repeat) +
                  ",Days;" +
                  (days.day.sun ? "S" : "-") +
                  (days.day.mon ? "M" : "-") +
                  (days.day.tue ? "T" : "-") +
                  (days.day.wed ? "W" : "-") +
                  (days.day.thu ? "T" : "-") +
                  (days.day.fri ? "F" : "-") +
                  (days.day.sat ? "S" : "-") +
                  ",Output;" + std::to_string(output) +
                  ",Action;" + std::to_string(action);

        // Include offset if it's not zero
        if (offset_hour != 0 || offset_minute != 0) {
            result += ",Offset;";
            result += use_negative_offset ? '-' : '+';
            result += std::to_string(offset_hour) + ":" +
                      (offset_minute < 10 ? "0" + std::to_string(offset_minute) : std::to_string(offset_minute));
        }

        return result;
    }

    void reset() {
        live = false;
        repeat = false;
        use_negative_offset = false;
        days.raw = 0;
        mode = 0;
        action = 0;
        output = 0;
        hour = 0;
        minute = 0;
        offset_hour = 0;
        offset_minute = 0;
        last_ran_timestamp = 0;
    }

    void from_string(const std::string& settings) {
        reset(); // resets the timer
        uint16_t index = 0;
        while (index < settings.size()) {
            // Find the next key-value pair
            size_t delimiterPos = settings.find(',', index);
            if (delimiterPos == std::string::npos) {
                delimiterPos = settings.size();  // Handle the last key-value pair
            }
            std::string keyValuePair = settings.substr(index, delimiterPos - index);

            // Split key and value
            size_t keyDelimiterPos = keyValuePair.find(';');
            if (keyDelimiterPos == std::string::npos) {
                break;
            }
            std::string key = keyValuePair.substr(0, keyDelimiterPos);
            std::string value = keyValuePair.substr(keyDelimiterPos + 1);

            // Update struct settings based on the key-value pair
            if (key == "Live") {
                live = std::stoi(value);
            } else if (key == "Mode") {
                mode = std::stoi(value);
            } else if (key == "Time") {
                size_t colonPos = value.find(':');
                if (colonPos != std::string::npos) {
                    hour = std::stoi(value.substr(0, colonPos));
                    minute = std::stoi(value.substr(colonPos + 1));
                }
            } else if (key == "Repeat") {
                repeat = std::stoi(value);
            } else if (key == "Days") {
                for (size_t i = 0; i < value.size(); ++i) {
                    if (value[i] != '-' && value[i] != '0') {
                        switch (i) {
                            case 0: days.day.sun = true; break;
                            case 1: days.day.mon = true; break;
                            case 2: days.day.tue = true; break;
                            case 3: days.day.wed = true; break;
                            case 4: days.day.thu = true; break;
                            case 5: days.day.fri = true; break;
                            case 6: days.day.sat = true; break;
                            default: break;
                        }
                    }
                }
            } else if (key == "Output") {
                output = std::stoi(value);
            } else if (key == "Action") {
                action = std::stoi(value);
            } else if (key == "Offset") {
                if (!value.empty()) {
                    size_t colonPos = value.find(':');
                    // check if offset starts with a + or - (or blank)
                    if (value[0] == '-') {
                        use_negative_offset = true;
                        if (colonPos != std::string::npos) {
                            offset_hour = std::stoi(value.substr(1, colonPos));
                        }
                    } else if (value[0] == '+') {
                        use_negative_offset = false;
                        if (colonPos != std::string::npos) {
                            offset_hour = std::stoi(value.substr(1, colonPos));
                        }
                    } else {
                        use_negative_offset = false;
                        if (colonPos != std::string::npos) {
                            offset_hour = std::stoi(value.substr(0, colonPos));
                        }
                    }
                    offset_minute = std::stoi(value.substr(colonPos + 1));
                }
            }

            // Move to the next key-value pair
            index = delimiterPos + 1;
        }
    }

} ATTRIBUTE_PACKED; // struct Timer

void doRelayAction(uint8_t i, time_t timestamp, bool set_relays) {
    ESP_LOGD("doRelayAction", "------ doRelayAction ran ------");
   
    // when not setting relays we just run the other actions below
    if (set_relays) {
        uint8_t output = id(global_timers)[i].output;
        uint8_t action = id(global_timers)[i].action;

        switch (output) {
            case 0:
                switch (action) {
                    case 2: 
                        id(relay_0).toggle(); 
                        break;
                    default: 
                        id(relay_0).publish_state(action); 
                        break;
                } break;
            case 1:
                switch (action) {
                    case 2: 
                        id(relay_1).toggle(); 
                        break;
                    default: 
                        id(relay_1).publish_state(action); 
                        break;
                } break;
            default: break;
        }
    }
    id(global_timers)[i].last_ran_timestamp = timestamp;

    // if repeat was disabled or not set deactivate timer after running
    if(id(global_timers)[i].repeat == false) {
        id(global_timers)[i].live = false;
    }
}

bool dayMatches(const esphome::ESPTime& date, const struct Timer& timer) {
    // we get the value based on the position to check date 
    if ((timer.days.raw & (1 << date.day_of_week - 1)) != 0) {
        ESP_LOGD("dayMatches", "------ day matched ------");
        return true;
    }
    return false;
}

// Function to process a single timer setting
time_t getTimerTimestamp(const struct Timer& timer) {
    // If the timer is disabled, skip checking the other values
    if (timer.live == false) {
        return 0;
    }
    // Get the current time
    esphome::ESPTime date = id(sntp_time).now();
    // If the day doesn't match return 0
    if (dayMatches(date, timer) == false) {
        return 0;
    }
    // run time match check
    time_t timestamp = 0;


    switch (timer.mode) {
        case 1:
            ESP_LOGD("getTimerTimestamp", "------ mode is set to sunrise ------");
            date.hour = date.minute = date.second = 0;                          // uptime time to 00:00:00
            date.recalc_timestamp_utc();
            timestamp = id(mysun).sunrise(date, -0.833)->timestamp;
            break;
        case 2:
            ESP_LOGD("getTimerTimestamp", "------ mode is set to sunset ------");
            date.hour = date.minute = date.second = 0;                          // uptime time to 00:00:00
            date.recalc_timestamp_utc();
            timestamp = id(mysun).sunset(date, -0.833)->timestamp;
            break;
        default: 
            ESP_LOGD("getTimerTimestamp", "------ mode is set to time ------");
            date.second = 0;
            date.hour = timer.hour;
            date.minute = timer.minute;
            struct tm tm;
            tm = date.to_c_tm();
            timestamp = mktime(&tm);
            break;
    }

    // calculate offset
    int offset_seconds = 0; // max value is 24 hours in seconds
    if(timer.use_negative_offset == true) {
        offset_seconds -= timer.offset_hour * 60 * 60;
        offset_seconds -= timer.offset_minute * 60;
    } else {
        offset_seconds += timer.offset_hour * 60 * 60;
        offset_seconds += timer.offset_minute * 60;
    }
    timestamp += offset_seconds;
    ESP_LOGD("getTimerTimestamp", "------ timestamp  %lu ------", timestamp);
    return timestamp;
} // getTimerTimestamp


void setTimerTimestamp(uint8_t i) {
    id(global_next_run)[i] = getTimerTimestamp(id(global_timers)[i]);
} // setTimerTimestamp

// Function to process a list of timer settings
void setAllTimersTimestamp() {
    ESP_LOGD("setAllTimersTimestamp", "------ updating all timer timestamps ------");
    for (uint8_t i = 0; i < id(num_of_timers); ++i) {
        setTimerTimestamp(i);
    }
} // setAllTimersTimestamp


void onInterval() {
    ESP_LOGD("onInterval", "------ Ran interval ------");
    if(id(override_timer).state == true) {
        ESP_LOGD("onInterval", "------ Disable All Timers is on, skipping all timer checks ------");
        return;
    }
    // set variables
    esphome::ESPTime date = id(sntp_time).now();
    date.timestamp -= date.timestamp % 60;   // set seconds to 0
    bool is_missed_timer = false;
    time_t global_missed_timers[id(num_of_timers)];
    // loop all timer timestamps
    for (uint8_t i = 0; i < id(num_of_timers); i++) {
        if(id(global_next_run)[i] > 0) {   // check if timer is active
            // ESP_LOGD("interval", "------ Timer %i Active ------", i + 1);
            // ESP_LOGD("interval", "------ Current Time %llu Next Run Time %llu Last Run Time %llu ------", date.timestamp, id(global_next_run)[i], id(global_last_run)[i]);
            if(id(global_next_run)[i] == date.timestamp) { // check if time matches
                ESP_LOGD("interval", "------ Timer %i matches current time ------", i + 1);
                doRelayAction(i, date.timestamp, true);
            }
        } else if(id(global_next_run)[i] > 0 && id(global_next_run)[i] < date.timestamp && id(global_timers)[i].last_ran_timestamp < (date.timestamp - 600)) {
            // timer time is before current time and the latest time ran is more then 23h59m ago (86340 seconds) - (changed to 10 minutes)
            ESP_LOGD("interval", "------ Timer %i not matched - Missed timer ------", i + 1);
            // add timestamp to list of missed timers
            global_missed_timers[i] = id(global_next_run)[i];
            is_missed_timer = true;
        } else {
            // ESP_LOGD("interval", "------ Timer %i not active ------", i + 1);
            // time not active
            global_missed_timers[i] = 0;
        }
    }
    // if(is_missed_timer == true) {
    //     ESP_LOGD("interval", "------ is_missed_timer is true ------");
    //     // set variables
    //     bool temp_relays[num_of_relays];
    //     uint8_t relay_index;
    //     uint8_t index[20] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    //     // sort array to earliest to latest got from https://stackoverflow.com/questions/46382252/sort-array-by-first-item-in-subarray-c
    //     std::sort(index, index + numTimers, [&](uint8_t n1, uint8_t n2){ return global_missed_timers[n1] < global_missed_timers[n2]; });
    //     // set the current values of the relays
    //     for (uint8_t i = 0; i < num_of_relays; i++) {
    //         temp_relays[i] = (*relays[i])->state;
    //         // ESP_LOGD("interval", "------ Set default state for relay %i ------", i);
    //     }
    //     // set the relays with the result of the timer sequence
    //     for (uint8_t i = 0; i < numTimers; i++) {
    //         // ESP_LOGD("interval", "------ loop index %i ------", index[i]);
    //         // use the sorted index location so we do the actions in the order the timer was set for them
    //         if(global_missed_timers[index[i]] > 0) {
    //             // do action without really setting relays
    //             doRelayAction(index[i], date.timestamp, false);
    //             ESP_LOGD("interval", "------ index %i is set do action ------", index[i]);
    //             // set relay index based on output from current timer
    //             relay_index = id(global_timer_numbers)[index[i]][2];
    //             if(id(global_timer_n umbers)[index[i]][3] == 2) {
    //                 // toggle
    //                 if(temp_relays[relay_index] == 0) {
    //                     temp_relays[relay_index] = 1;
    //                 } else {
    //                     temp_relays[relay_index] = 0;
    //                 }
    //             } else {
    //                 // set state
    //                 temp_relays[relay_index] = id(global_timer_numbers)[index[i]][3];
    //             }
    //         }
    //     }
    //     // loop relays and set state
    //     for (uint8_t i = 0; i < num_of_relays; i++) {
    //         ESP_LOGD("interval", "------ Set relay %i to state %i ------", i, temp_relays[i]);
    //         if((*relays[i])->state != temp_relays[i]) {
    //             (*relays[i])->publish_state(temp_relays[i]);
    //         }
    //     }
        
    // } // if(is_missed_timer == true)
} // onInterval

void onSelect(std::string x) {
    // ESP_LOGD("onSelect", "------ Ran onSelect ------");
    // ESP_LOGD("onSelect", "%s", x.c_str());

    uint8_t index = static_cast<int>(*id(select_timer).active_index());
    ESP_LOGD("onSelect", "------ Timer %i Selected ------", index);
    // ESP_LOGD("onSelect", "------ Timer %i last run at %llu next run at %llu ------", num_timer, id(global_last_run)[num_timer - 1], id(global_next_run)[num_timer - 1]);
    if(index > 0 && index <= id(num_of_timers)) {
        // set num_timer to be base on position starting from 0
        index -= 1;
        // set text string
        // ESP_LOGD("onSelect", "------ string %s ------", id(timer_text).state.c_str());
        id(timer_text).publish_state(id(global_timers)[index].to_string());
        ESP_LOGD("onSelect", "------ next run %lu ------", id(global_next_run)[index]);
    } else {
        id(timer_text).publish_state("");
    }
} // onSelect

void onPressSave() {
    ESP_LOGD("onPressSave", "------ Save Button Pressed ------");

    uint8_t index = static_cast<int>(*id(select_timer).active_index());
    if(index > 0 && index <= id(num_of_timers)) {
        // set num_timer to be base on position starting from 0
        index -= 1;
        id(global_timers)[index].from_string(id(timer_text).state);
        setTimerTimestamp(index);
    }
} // onPressSave