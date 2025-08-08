#include <pigpiod_if2.h>
#include <chrono>
#include <thread>

int main() {
  int pi = pigpio_start(NULL, NULL);     // 데몬 접속
  int gpio = 12;                         // BCM12 (핀아웃 확인 필수)

  // 펄스폭 μs: 보통 1000=최소, 1500=중립, 2000=최대 (서보마다 약간 다름)
  set_servo_pulsewidth(pi, gpio, 1500);
  std::this_thread::sleep_for(std::chrono::seconds(2));

  set_servo_pulsewidth(pi, gpio, 1000);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  set_servo_pulsewidth(pi, gpio, 2000);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  set_servo_pulsewidth(pi, gpio, 0);     // 0 → 펄스 중단(해제)
  pigpio_stop(pi);
}