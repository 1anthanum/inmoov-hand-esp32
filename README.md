# InMoov Hand on ESP32 — Serial · Web · Voice Control

Firmware for a tendon-driven 3D-printed [InMoov](https://inmoov.fr) hand, running on an **ESP32 + PCA9685** servo stack. Five fingers plus a geared rotating wrist, smooth eased motion, gesture presets, and a self-hosted web console with Chinese (zh-CN) voice control — no cloud, no app, just a browser.

> **Photo:** _待补 (to be added) — one real photo of the assembled hand._

> Built as the first public artifact of a longer robotics journey (InMoov hands → SO-101 → imitation learning → LLM orchestration). Both a left and a mirrored right hand run this firmware.

## Features

- **6-channel servo control** — 5 fingers + wrist via PCA9685, 50 Hz, 500–2500 µs pulse mapping
- **Eased motion** — 20 ms update loop interpolates every move; no servo snap
- **Per-finger calibration arrays** — `OPEN_A[6]` / `CLOSE_A[6]` / `INVERT[6]`: tune once, use everywhere
- **Gesture presets** — open / fist / point / ok / peace / three, table-driven poses (plus a `demo` loop in v1)
- **Serial protocol** (v1) — drive any finger by percent, trigger gestures from any host
- **Web console** (v2) — WiFi + HTTP (port 80, single-file UI served from PROGMEM) + WebSocket (port 81); sliders throttled at 60 ms
- **Voice control** (v2) — browser `webkitSpeechRecognition`, zh-CN, local word map → gestures
- **State broadcast** (v2) — 500 ms JSON status over WebSocket (OLED / dashboard hook ready)
- **Servo centering tool** — standalone sketch for the assembly-critical "center at 90° before rigging" step

## Hardware

| Part | Notes |
|---|---|
| ESP32 dev board | Any common devkit |
| PCA9685 16-ch PWM board | I2C `0x40`, SDA=GPIO21, SCL=GPIO22 |
| 6× MG996R (or similar 180°) | 5 fingers + wrist |
| External 5–6 V supply, ≥5 A | To PCA9685 **V+**. Never power servos from USB |
| Braided PE line (~200 lb / 0.8 mm) | Tendons, ~75 cm per finger run |

**Channel map:** CH0–CH4 = thumb → pinky, CH5 = wrist. **Common ground between ESP32, PCA9685 and the servo supply is mandatory.** See [`docs/wiring.md`](docs/wiring.md).

## Repo layout

```
firmware/
  servo_center_tool.ino      # flash FIRST — center servos before mechanical assembly
  inmoov_hand_firmware.ino   # v1: serial control, easing, gestures
  inmoov_hand_firmware_v2.ino# v2: v1 + WiFi web console + WebSocket + voice
docs/
  wiring.md                  # power + I2C wiring, channel map
  calibration.md             # the 90° → rig → 0° iron rule, per-finger tuning
```

## Quick start

1. **Flash `servo_center_tool.ino`** and center every servo at 90° (`c` over serial) **before** mounting horns and rigging tendons. This single habit prevents most broken fingers.
2. Assemble and rig per the InMoov instructions, then calibrate: full-open = 0 %, tune `OPEN_A` / `CLOSE_A` per finger, flip `INVERT` where the tendon runs opposite. Procedure in [`docs/calibration.md`](docs/calibration.md).
3. Flash v1, verify serial control. Move to v2, set WiFi credentials, open the printed IP in a browser — sliders, gestures, and the mic button are all there.

Pulse-to-tick conversion used throughout:

```cpp
int toTicks(long us) { return (int)(us * FREQ * 4096.0 / 1000000.0); }  // FREQ = 50
```

## 3D-printed parts

STLs are **not** redistributed here. The InMoov hand is designed by **Gael Langevin** and published under **CC BY-NC** — download the parts from [inmoov.fr](https://inmoov.fr). A right hand is the same 15 side-specific files mirrored in your slicer; fingers, printed bolts, pulleys and the RotaWrist gear set are side-neutral.

## Safety notes

- Servo power never comes from USB; supply ≥5 A or fingers will brown-out mid-grip.
- Pulling the plug is the e-stop. Keep the supply within reach during tests.
- First power-up: one finger at a time, watch for a servo fighting its end stop (hot servo = wrong calibration).

## Known limitations

- _待补 (to be added)._

## License

Firmware and docs: **MIT** (see `LICENSE`). InMoov mechanical design: CC BY-NC by Gael Langevin — respect the non-commercial terms for the printed parts.

---

### 中文摘要

InMoov 腱驱打印手的 ESP32 + PCA9685 固件：五指+转腕、六通道缓动控制、每指校准数组、手势预设；v2 提供自托管网页控制台（HTTP 80 / WebSocket 81）与浏览器中文语音控制，无需云端。装配铁律：**先烧录归中工具、舵机 90° 中位再上舵盘穿线，全开位定 0%**。STL 请从 inmoov.fr 下载（CC BY-NC，勿在本仓库再分发）；右手为切片软件镜像打印。
