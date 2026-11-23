# Wiring

## Power topology (the part that breaks first)

```
USB ──► ESP32 (logic only)
                 │ SDA GPIO21 ─────────┐
                 │ SCL GPIO22 ─────────┤
                 │ GND ────────┐       │
                               ▼       ▼
5–6 V / ≥5 A ──► V+  ──► PCA9685 (addr 0x40) ──► CH0..CH5 servos
supply GND  ──► GND ─────┘
```

Three rules, in order of how often they are violated:

1. **Common ground.** ESP32 GND, PCA9685 GND and the servo supply GND must be one net. Symptom of a missing bond: servos twitch randomly, I2C errors, "haunted hand".
2. **Servo power goes to V+ on the screw terminal, never from USB.** Six MG996R stall well above 2 A combined; USB browns out and the ESP32 reboots mid-grip.
3. **VCC (logic pin) on the PCA9685 comes from the ESP32 3V3** — it powers the chip's logic, not the servos. Do not bridge VCC and V+.

## Channel map

| Channel | Joint |
|---|---|
| CH0 | Thumb |
| CH1 | Index |
| CH2 | Middle |
| CH3 | Ring |
| CH4 | Pinky |
| CH5 | Wrist (RotaWrist gear drive) |
| CH6 | Reserved (gripper / future) |

## Signal parameters

- PWM frequency: **50 Hz**
- Pulse range: **500–2500 µs** mapped to 0–180°
- Tick conversion: `ticks = µs × 50 × 4096 / 1e6`

## Debug order

电 → 信号 → 代码. Power first (measure V+ under load), then signals (I2C scan at 0x40, pulse on channel), and only then suspect the firmware.
