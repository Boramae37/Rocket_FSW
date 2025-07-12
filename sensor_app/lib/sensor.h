#include <iostream>

// abstract class
class Sensor{
public:
  Sensor() {}
  virtual ~Sensor() {}
   
  virtual bool init() = 0; // i2c slave addr
  virtual void read_data() = 0;    
};


class BMP390 : public Sensor{
  private:
    uint8_t slave_addr;
    struct trim_cf{
      int

    };
    unsigned int 
    uint8_t pressure_data;
    uint8_t barometer_data;

  public:
    BMP390() {}
    virtual ~BMP390(){}

    virtual bool init();
    virtual void read_data();

    void reset();

};
