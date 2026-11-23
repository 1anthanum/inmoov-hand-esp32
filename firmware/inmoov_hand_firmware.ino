/*
 * InMoov 左手固件 v1 —— 五指 + 转腕 · 校准表 + 缓动 + 手势库 + 串口控制
 * 硬件：ESP32 DevKitC + PCA9685(0x40) + MG996R x6（CH0-4=拇食中无小，CH5=转腕）
 * 接线：3V3→VCC GND→GND(共地!) GPIO21→SDA GPIO22→SCL ｜ 6V适配器→V+/GND
 * 库：Adafruit PWM Servo Driver ｜ 板：ESP32 Dev Module ｜ 串口 115200
 *
 * ★ 使用前必做的校准（张线完成后）：
 *   1) 逐指用 `raw 0 20` 这类指令试出"完全伸直"和"舒适握拢"的真实角度
 *   2) 填进下方 OPEN_A[] / CLOSE_A[]（转腕填左右极限），重新烧录
 *   3) 之后一律用百分比/手势指令，不再直接用角度
 *
 * 串口指令：
 *   open / fist / point / ok / peace / three  → 手势
 *   f <指0-4> <0-100>   → 单指百分比（0=全伸直 100=握拢）
 *   h <0-100>           → 五指同步百分比
 *   w <0-100>           → 手腕（0=左极限 50=中位 100=右极限）
 *   raw <ch> <deg>      → 直接角度（仅校准用，慢速小步！）
 *   speed <1-10>        → 动作速度（默认3，越大越快）
 *   rest                → 释放全部舵机（断脉冲，散热/安全）
 *   demo                → 循环演示   |  ?  → 菜单
 */
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm(0x40);
const float FREQ = 50.0;
const int   US_MIN = 500, US_MAX = 2500;      // MG996R 0-180°脉宽

// ------- ★ 校准表：先用保守值，张线后按实测修改 -------
//                  拇指  食指  中指  无名  小指  手腕
float OPEN_A[6]  = {  10,   5,    5,    5,    5,   45 }; // 伸直/腕左限
float CLOSE_A[6] = {  70,  85,   85,   85,   80,  135 }; // 握拢/腕右限
bool  INVERT[6]  = {false,false,false,false,false,false}; // 某指方向反了改true
// -----------------------------------------------------

float cur[6], tgt[6];          // 当前/目标角度
int   stepDeg = 3;             // 每20ms最大步进(速度)
bool  live[6] = {false,false,false,false,false,false};

int toTicks(long us){ return (int)(us*FREQ*4096.0/1000000.0); }
void writeDeg(int ch, float d){
  d = constrain(d, 0, 180);
  pwm.setPWM(ch, 0, toTicks(US_MIN + (long)((US_MAX-US_MIN)*d/180.0)));
}
float pctToDeg(int i, float p){
  p = constrain(p, 0, 100);
  if (INVERT[i]) p = 100 - p;
  return OPEN_A[i] + (CLOSE_A[i]-OPEN_A[i]) * p / 100.0;
}
void setFinger(int i, float pct){ tgt[i]=pctToDeg(i,pct); live[i]=true; }
void setHand(float pct){ for(int i=0;i<5;i++) setFinger(i,pct); }
void gesture(const float g[5]){ for(int i=0;i<5;i++) setFinger(i,g[i]); }

const float G_OPEN [5]={0,0,0,0,0},      G_FIST [5]={100,100,100,100,100};
const float G_POINT[5]={100,0,100,100,100}, G_OK  [5]={80,80,0,0,0};
const float G_PEACE[5]={100,0,0,100,100},   G_THREE[5]={100,0,0,0,100};

void restAll(){ for(int c=0;c<6;c++){ pwm.setPWM(c,0,0); live[c]=false; } Serial.println("all released"); }

void menu(){
  Serial.println("\n=== InMoov Hand v1 ===");
  Serial.println("open/fist/point/ok/peace/three | f <i> <pct> | h <pct> | w <pct>");
  Serial.println("raw <ch> <deg>(校准) | speed <1-10> | rest | demo | ?");
}

void setup(){
  Serial.begin(115200);
  Wire.begin(21,22);
  pwm.begin(); pwm.setPWMFreq(FREQ);
  for(int i=0;i<6;i++){ cur[i]=OPEN_A[i]; tgt[i]=OPEN_A[i]; }
  restAll();                       // 上电静默，等待指令——安全第一
  menu();
}

unsigned long lastTick=0, demoT=0; int demoStep=-1;
void loop(){
  // --- 20ms 缓动节拍 ---
  if (millis()-lastTick >= 20){
    lastTick = millis();
    for(int i=0;i<6;i++){
      if(!live[i]) continue;
      float d = tgt[i]-cur[i];
      if (fabs(d) > 0.5){
        cur[i] += constrain(d, -stepDeg, stepDeg);
        writeDeg(i, cur[i]);
      }
    }
  }
  // --- demo 循环 ---
  if (demoStep>=0 && millis()-demoT>1600){
    demoT=millis();
    switch(demoStep++%6){
      case 0: gesture(G_OPEN); break;  case 1: gesture(G_FIST);  break;
      case 2: gesture(G_OPEN); break;  case 3: gesture(G_POINT); break;
      case 4: gesture(G_OK);   break;  case 5: gesture(G_PEACE); break;
    }
  }
  // --- 串口解析 ---
  if(!Serial.available()) return;
  String s = Serial.readStringUntil('\n'); s.trim();
  if(!s.length()) return;
  demoStep = -1;                                   // 任何指令终止demo
  if     (s=="open" ) gesture(G_OPEN);
  else if(s=="fist" ) gesture(G_FIST);
  else if(s=="point") gesture(G_POINT);
  else if(s=="ok"   ) gesture(G_OK);
  else if(s=="peace") gesture(G_PEACE);
  else if(s=="three") gesture(G_THREE);
  else if(s=="rest" ) restAll();
  else if(s=="demo" ){ demoStep=0; demoT=0; Serial.println("demo... 任意指令停止"); }
  else if(s=="?"    ) menu();
  else{
    char c; int a=-1, b=-1;
    c = s.charAt(0);
    sscanf(s.c_str()+1, "%d %d", &a, &b);
    if     (c=='f' && a>=0 && a<5 && b>=0) setFinger(a,b);
    else if(c=='h' && a>=0)                setHand(a);
    else if(c=='w' && a>=0)                setFinger(5,a);
    else if(c=='r' && s.startsWith("raw") ){ int ch,dg; if(sscanf(s.c_str()+3,"%d %d",&ch,&dg)==2 && ch>=0&&ch<6){ cur[ch]=dg; tgt[ch]=dg; live[ch]=true; writeDeg(ch,dg); Serial.printf("RAW CH%d=%d°\n",ch,dg);} }
    else if(c=='s' && s.startsWith("speed")){ int v; if(sscanf(s.c_str()+5,"%d",&v)==1) stepDeg=constrain(v,1,10); Serial.printf("speed=%d\n",stepDeg); }
    else menu();
  }
}
