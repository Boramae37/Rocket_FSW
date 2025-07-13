#include <iostream>

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

    float temperature_data;
    float pressure_data;
    float barometer_data;

    struct nvm_calib_data{
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
