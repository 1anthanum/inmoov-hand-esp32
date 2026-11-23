/*
 * InMoov 左手固件 v2 —— WiFi 网页控制台 + WebSocket + 串口双通道
 * = v1 全部功能（校准表/缓动/手势库/串口指令） + 浏览器滑杆/按钮/语音入口
 *
 * 硬件：ESP32 DevKitC + PCA9685(0x40) + MG996R×6（CH0-4指,CH5腕）
 * 库（库管理器安装）：
 *   - Adafruit PWM Servo Driver Library
 *   - WebSockets (by Markus Sattler / Links2004)
 * 使用：
 *   1) 下方填入 WiFi 名与密码，烧录
 *   2) 串口 115200 看到 "IP: x.x.x.x" 后，同一 WiFi 下浏览器打开 http://<IP>/
 *   3) 滑杆控单指，按钮出手势；串口指令与 v1 完全一致，两边随时可用
 * 协议：WebSocket(端口81) 收发纯文本，与串口指令同一语法（fist / f 2 50 / w 30 ...）
 */
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ------------ ★ 改这里 ------------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
// ---------------------------------

Adafruit_PWMServoDriver pwm(0x40);
WebServer http(80);
WebSocketsServer ws(81);

const float FREQ = 50.0;
const int US_MIN = 500, US_MAX = 2500;

// ---- 校准表（沿用 v1 实测值）----
float OPEN_A[6]  = {  10,   5,    5,    5,    5,   45 };
float CLOSE_A[6] = {  70,  85,   85,   85,   80,  135 };
bool  INVERT[6]  = {false,false,false,false,false,false};

float cur[6], tgt[6];
bool  live[6] = {false,false,false,false,false,false};
int   stepDeg = 3;
String curGesture = "rest";

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
float degToPct(int i, float d){
  float p = (d - OPEN_A[i]) / (CLOSE_A[i]-OPEN_A[i]) * 100.0;
  if (INVERT[i]) p = 100 - p;
  return constrain(p, 0, 100);
}
void setFinger(int i, float pct){ tgt[i]=pctToDeg(i,pct); live[i]=true; }
void setHand(float pct){ for(int i=0;i<5;i++) setFinger(i,pct); }
void restAll(){ for(int c=0;c<6;c++){ pwm.setPWM(c,0,0); live[c]=false; } curGesture="rest"; }

const float G_OPEN[5]={0,0,0,0,0}, G_FIST[5]={100,100,100,100,100},
            G_POINT[5]={100,0,100,100,100}, G_OK[5]={80,80,0,0,0},
            G_PEACE[5]={100,0,0,100,100}, G_THREE[5]={100,0,0,0,100};
void gesture(const float g[5], const char* name){ for(int i=0;i<5;i++) setFinger(i,g[i]); curGesture=name; }

// ---- 统一指令处理：串口和 WebSocket 共用 ----
String handleCommand(String s){
  s.trim(); if(!s.length()) return "";
  if     (s=="open" ) gesture(G_OPEN ,"open");
  else if(s=="fist" ) gesture(G_FIST ,"fist");
  else if(s=="point") gesture(G_POINT,"point");
  else if(s=="ok"   ) gesture(G_OK   ,"ok");
  else if(s=="peace") gesture(G_PEACE,"peace");
  else if(s=="three") gesture(G_THREE,"three");
  else if(s=="rest" ) restAll();
  else {
    char c = s.charAt(0); int a=-1,b=-1;
    sscanf(s.c_str()+1, "%d %d", &a, &b);
    if     (c=='f' && a>=0 && a<5 && b>=0){ setFinger(a,b); curGesture="custom"; }
    else if(c=='h' && a>=0){ setHand(a); curGesture="custom"; }
    else if(c=='w' && a>=0){ setFinger(5,a); }
    else if(s.startsWith("raw")){ int ch,dg; if(sscanf(s.c_str()+3,"%d %d",&ch,&dg)==2&&ch>=0&&ch<6){cur[ch]=dg;tgt[ch]=dg;live[ch]=true;writeDeg(ch,dg);} }
    else if(s.startsWith("speed")){ int v; if(sscanf(s.c_str()+5,"%d",&v)==1) stepDeg=constrain(v,1,10); }
    else return "ERR unknown: " + s;
  }
  return "OK " + s;
}

// ---- 内嵌网页（滑杆+手势+语音）----
const char PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang=zh><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>InMoov Hand Console</title><style>
body{font-family:-apple-system,'PingFang SC',sans-serif;background:#10233f;color:#eaf1fb;margin:0;padding:18px;max-width:560px;margin:auto}
h2{margin:8px 0 14px} .g{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:12px 0}
button{background:#0f62fe;color:#fff;border:0;border-radius:10px;padding:12px;font-size:15px}
button:active{opacity:.7} .rest{background:#5a6a7e} .voice{background:#2f9e5f;grid-column:span 3}
.sl{margin:10px 0} .sl label{display:flex;justify-content:space-between;font-size:14px;color:#a9bcd8}
input[type=range]{width:100%} #log{font:12px Menlo;color:#7fd08f;background:#0a1830;border-radius:8px;padding:8px;height:70px;overflow:auto;white-space:pre-wrap}
#st{font-size:13px;color:#a9bcd8;margin:6px 0}
</style></head><body>
<h2>🖐 InMoov Hand Console</h2><div id=st>连接中…</div>
<div class=g>
<button onclick="tx('open')">张开</button><button onclick="tx('fist')">握拳</button><button onclick="tx('point')">指点</button>
<button onclick="tx('ok')">OK</button><button onclick="tx('peace')">✌️</button><button onclick="tx('three')">三</button>
<button class=voice id=vb onclick="voice()">🎤 语音（中文）</button>
<button class=rest onclick="tx('rest')" style="grid-column:span 3">释放全部</button>
</div>
<div id=sliders></div>
<div class=sl><label>手腕 <span id=vw>50</span></label><input type=range min=0 max=100 value=50 oninput="wr(this.value)"></div>
<div id=log></div>
<script>
const names=['拇指','食指','中指','无名指','小指'];
const box=document.getElementById('sliders');
names.forEach((n,i)=>{box.insertAdjacentHTML('beforeend',
`<div class=sl><label>${n} <span id=v${i}>0</span></label><input type=range min=0 max=100 value=0 oninput="fr(${i},this.value)"></div>`)});
let ws,t={};function conn(){ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onopen=()=>st('已连接 ✓');ws.onclose=()=>{st('已断开，3秒重连…');setTimeout(conn,3000)};
ws.onmessage=e=>log(e.data);}conn();
function st(s){document.getElementById('st').textContent=s}
function log(s){const l=document.getElementById('log');l.textContent=(s+'\n'+l.textContent).slice(0,800)}
function tx(c){if(ws&&ws.readyState==1){ws.send(c);}}
function fr(i,v){document.getElementById('v'+i).textContent=v;clearTimeout(t[i]);t[i]=setTimeout(()=>tx('f '+i+' '+v),60)}
function wr(v){document.getElementById('vw').textContent=v;clearTimeout(t.w);t.w=setTimeout(()=>tx('w '+v),60)}
function voice(){const R=window.SpeechRecognition||window.webkitSpeechRecognition;
if(!R){log('此浏览器不支持语音，请用 Chrome');return}
const r=new R();r.lang='zh-CN';r.onresult=e=>{const said=e.results[0][0].transcript;log('🎤 '+said);
const map={'张开':'open','打开':'open','握拳':'fist','拳头':'fist','指':'point','OK':'ok','耶':'peace','剪刀':'peace','三':'three','放松':'rest'};
for(const k in map){if(said.includes(k)){tx(map[k]);return}}
log('（本地词表未命中——接入 LLM 中继后这里交给模型理解）')};
r.start();}
</script></body></html>
)HTML";

unsigned long lastTick=0, lastPush=0;
void setup(){
  Serial.begin(115200);
  Wire.begin(21,22);
  pwm.begin(); pwm.setPWMFreq(FREQ);
  for(int i=0;i<6;i++){ cur[i]=OPEN_A[i]; tgt[i]=OPEN_A[i]; }
  restAll();

  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status()!=WL_CONNECTED){ delay(400); Serial.print("."); }
  Serial.printf("\nIP: %s  （浏览器打开 http://%s/ ）\n",
    WiFi.localIP().toString().c_str(), WiFi.localIP().toString().c_str());

  http.on("/", [](){ http.send_P(200, "text/html", PAGE); });
  http.begin();
  ws.begin();
  ws.onEvent([](uint8_t n, WStype_t t, uint8_t* p, size_t l){
    if (t==WStype_TEXT){ String r = handleCommand(String((char*)p).substring(0,l)); ws.sendTXT(n, r); }
  });
}

void loop(){
  http.handleClient(); ws.loop();
  if (millis()-lastTick >= 20){
    lastTick = millis();
    for(int i=0;i<6;i++){
      if(!live[i]) continue;
      float d = tgt[i]-cur[i];
      if (fabs(d) > 0.5){ cur[i] += constrain(d, -stepDeg, stepDeg); writeDeg(i, cur[i]); }
    }
  }
  if (millis()-lastPush >= 500){       // 状态广播：网页/未来OLED共用
    lastPush = millis();
    String j = "{\"g\":\""+curGesture+"\",\"p\":[";
    for(int i=0;i<6;i++){ j += String((int)degToPct(i,cur[i])); if(i<5) j+=","; }
    j += "]}";
    ws.broadcastTXT(j);
  }
  if (Serial.available()){
    String r = handleCommand(Serial.readStringUntil('\n'));
    if (r.length()) Serial.println(r);
  }
}
