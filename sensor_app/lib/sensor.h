#include <iostream>
#include <vector>
// abstract class
class Sensor{
public:
  Sensor() {}
  virtual ~Sensor() {}
   
  virtual bool init() = 0; // i2c slave addr
  virtual void read_data() = 0; 
  virtual std::vector<float> get_data() = 0;
};


class BMP390 : public Sensor{
  private:
    uint8_t slave_addr;

    float temperature;
    float pressure;
    float barometer_data;

    struct nvm_calib_data_st{
     /*! Trim Variables */
      uint16_t par_t1;
      uint16_t par_t2;
      int8_t par_t3;
      int16_t par_p1;
      int16_t par_p2;
      int8_t par_p3;
      int8_t par_p4;
      uint16_t par_p5;
      uint16_t par_p6;
      int8_t par_p7;
      int8_t par_p8;
      int16_t par_p9;
      int8_t par_p10;
      int8_t par_p11;
      //int64_t t_lin;  // compensated temperature / got from temperature compensation
      // 그냥 멤버 변수에 저장..
    }; 
    nvm_calib_data_st nvm_calib_data;

    float calib_data_t[4]; // par_t1, t2, t3, t_lin
    float calib_data_p[11]; // par_p1 ... p11
    

  public:
    BMP390() {}
    virtual ~BMP390(){}

    virtual bool init();
    virtual void read_data();
    
    // return {pressure, temperature} 
    virtual std::vector<float> get_data();
    
    // compensate functioins. recommend to use just _data. 
    // because, compensate_data is just exicuting compensate_temp, _pres function.
    // but not call the functions. 
    // And, compesate_pres depends on compenstate_temp.
    // Beacause it uses compensated temperature data.
    // After all, we usually need to compensate for both temperature and pressure,
    // so let's use the compensate_data function.
    void compensate_data();
    //void compensate_temp(); if you need, make it. just copy some of compensate_data()
    //void compensate_pres(); 


    // soft reset function
    void reset();
};


class BNO055 : public Sensor{
  private:
    uint8_t slave_addr;
    uint8_t status;

    float acc_x;
    float acc_y;
    float acc_z;

    float gyro_x;
    float gyro_y;
    float gyro_z;

    float Euler_ang_x;
    float Euler_ang_y;
    float Euler_ang_z;

    float Quternion_ang_w;
    float Quternion_ang_x;
    float Quternion_ang_y;
    float Quternion_ang_z;

    float linear_acc_x;
    float linear_acc_y;
    float linear_acc_z;

public:
    BNO055() {}
    virtual ~BNO055(){}

    virtual bool init();
    virtual void read_data();
    
    // return {acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z,
    //         euler_x, euler_y, euler_z,
    //         quaternion_w, quaternion_x, quaternion_y, quaternion_z,
    //         linear_acc_x, linear_acc_y, linear_acc_z}
    virtual std::vector<float> get_data();

    // soft reset function
    void reset();

};