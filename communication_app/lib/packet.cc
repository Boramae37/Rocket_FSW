#include "communication.h"
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>


packet::packet(){
    team_id = 8888;
    mission_time = "00:00:00";
    packet_count = 0;
    mode = "F";
    state = "LAUNCH_PAD";
    altitude = 0.0;
    temperature = 0.0;
    pressure = 0.0;
    voltage = 0.0;
    gyro_roll = 0.0;
    gyro_pitch = 0.0;
    gyro_yaw = 0.0;
    acc_x = 0.0;
    acc_y = 0.0;
    acc_z = 0.0;
    mag_roll = 0.0;
    mag_pitch = 0.0;
    mag_yaw = 0.0;
    rot_rate = 0.0;
    gps_lat = 0.0;
    gps_lon = 0.0;
    gps_alt = 0.0;
    gps_time = "00:00:00";
    gps_sats = 0.0;
    filtered_pitch = 0.0;
    filtered_roll = 0.0;
    filtered_yaw = 0.0;
    cmd_echo = "None";
}

// packet 묶어서 string으로 전달해줌
std::string packet::packet_combine(){
   return fmt::format("{},{},{},{},{},{:.2f},{:.2f},{:.2f},{:.2f},"
                           "{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},{:.2f},"
                           "{:.2f},{:.2f},{:.2f},{:.2f},{:.6f},{:.6f},{:.2f},"
                           "{},{:.0f},{:.2f},{:.2f},{:.2f},{}",
            team_id, mission_time, packet_count, mode, state,
            altitude, temperature, pressure, voltage,
            gyro_roll, gyro_pitch, gyro_yaw,
            acc_x, acc_y, acc_z,
            mag_roll, mag_pitch, mag_yaw,
            rot_rate, gps_lat, gps_lon, gps_alt,
            gps_time, gps_sats, filtered_roll, filtered_pitch, filtered_yaw,
            cmd_echo
        );
}