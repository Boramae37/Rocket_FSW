#ifndef communication_h
#define communication_h

#include <string>

class packet{
    public:

    int team_id;
    std::string mission_time;
    int packet_count;
    std::string mode;
    std::string state;
    float altitude, temperature, pressure, voltage, gyro_roll, gyro_pitch, gyro_yaw, acc_x, acc_y, acc_z, mag_roll, mag_pitch, mag_yaw, rot_rate,
          gps_lat, gps_lon, gps_alt;

    std::string gps_time;
    
    float gps_sats, filtered_roll, filtered_pitch, filtered_yaw;

    std::string cmd_echo;

    packet();
    std::string packet_combine();

};


#endif