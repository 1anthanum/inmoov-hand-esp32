/*
 * ServoCenter —— InMoov 舵机快速回中工具
 * 硬件：ESP32 DevKitC + PCA9685（I2C 0x40）+ 6V 外部供电
 * 接线：ESP32 3V3→PCA9685 VCC ｜ GND→GND（并与6V电源共地）
 *       GPIO21→SDA ｜ GPIO22→SCL ｜ 6V适配器→V+/GND 螺丝端子
 * 依赖：Arduino IDE 库管理器安装 "Adafruit PWM Servo Driver Library"
 * 开发板选择：ESP32 Dev Module ｜ 串口监视器波特率 115200
 *
 * 用法（串口输入指令，回车发送）：
 *   c            → 全部通道回中 90°（装绕线轮用这个）
 *   z            → 全部通道到 0°（装完轮、绑线前用这个；绑线时保持通电=舵机锁死不动）
 *   a 45         → 全部通道到 45°
 *   s 2 90       → 仅通道2 到 90°
 *   w 1          → 通道1 慢速摆动 0↔60°（装配后顺滑度测试）
 *   r 2 / R      → 释放通道2 / 释放全部（停止输出脉冲，舵机可手动转）
 *   u 600 2400   → 修改脉宽范围 µs（默认 500–2500；行程不对时微调）
 *   ?            → 重印菜单
 */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm(0x40);
const float FREQ = 50.0;          // 舵机标准 50Hz
int usMin = 500, usMax = 2500;    // MG996R 典型 0°/180° 脉宽

int toTicks(long us) {            // µs → PCA9685 计数(0-4095/20ms)
  return (int)(us * FREQ * 4096.0 / 1000000.0);
}

void setAngle(int ch, float deg) {
  deg = constrain(deg, 0.0, 180.0);
  long us = usMin + (long)((usMax - usMin) * deg / 180.0);
  pwm.setPWM(ch, 0, toTicks(us));
  Serial.printf("CH%d -> %.0f° (%ldus)\n", ch, deg, us);
}

void allAngle(float deg) { for (int c = 0; c < 16; c++) setAngle(c, deg); }
void releaseCh(int ch)   { pwm.setPWM(ch, 0, 0); Serial.printf("CH%d released\n", ch); }
void releaseAll()        { for (int c = 0; c < 16; c++) pwm.setPWM(c, 0, 0); Serial.println("ALL released"); }

void sweep(int ch) {
  Serial.printf("CH%d sweep 0-60-0 x3 (Ctrl+C=复位板子可中断)\n", ch);
  for (int k = 0; k < 3; k++) {
    for (int a = 0; a <= 60; a += 2) { setAngleQuiet(ch, a); delay(30); }
    for (int a = 60; a >= 0; a -= 2) { setAngleQuiet(ch, a); delay(30); }
    delay(500);
  }
  Serial.println("sweep done");
}
void setAngleQuiet(int ch, float deg) {
  long us = usMin + (long)((usMax - usMin) * constrain(deg,0,180) / 180.0);
  pwm.setPWM(ch, 0, toTicks(us));
}

void menu() {
  Serial.println("\n=== ServoCenter 舵机回中工具 ===");
  Serial.println("c=全部90° | z=全部0° | a <deg>=全部到角度");
  Serial.println("s <ch> <deg>=单通道 | w <ch>=摆动测试");
  Serial.println("r <ch>=释放单通道 | R=释放全部 | u <min> <max>=脉宽µs | ?=菜单");
  Serial.printf ("当前脉宽范围: %d-%dus @50Hz\n", usMin, usMax);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pwm.begin();
  pwm.setPWMFreq(FREQ);
  delay(100);
  releaseAll();               // 上电默认不输出，舵机自由——安全第一
  menu();
}

void loop() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  char cmd = line.charAt(0);
  int ch = -1, v1 = -1, v2 = -1;
  sscanf(line.c_str() + 1, "%d %d", &v1, &v2);

  switch (cmd) {
    case 'c': allAngle(90);  Serial.println(">> 全部 90°（置中完成，可装绕线轮）"); break;
    case 'z': allAngle(0);   Serial.println(">> 全部 0°（保持通电即锁定，可绑线张紧）"); break;
    case 'a': if (v1 >= 0) allAngle(v1); break;
    case 's': if (v1 >= 0 && v2 >= 0) setAngle(v1, v2); break;
    case 'w': if (v1 >= 0) sweep(v1); break;
    case 'r': if (v1 >= 0) releaseCh(v1); break;
    case 'R': releaseAll(); break;
    case 'u': if (v1 > 0 && v2 > v1) { usMin = v1; usMax = v2; Serial.printf("脉宽改为 %d-%dus\n", usMin, usMax); } break;
    default : menu(); break;
  }
}
