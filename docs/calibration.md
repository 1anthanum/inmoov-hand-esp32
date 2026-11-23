# Calibration — the 90° → rig → 0° iron rule

Tendon hands break when servos are rigged at an unknown angle. The whole procedure exists to make the mechanical zero and the software zero the same point.

## The iron rule

1. **Before any horn or line touches a servo:** flash `firmware/servo_center_tool.ino`, send `c` — every servo drives to **90° (mid-travel)** and holds.
2. Mount horns / pulleys and rig tendons **at 90°**, fingers at mid-curl. The servo now has equal travel toward open and closed.
3. Send `z` (all to 0°) — fingers should reach **full open**. Full open is defined as **0 %** in the main firmware.
4. Leave 5–8 cm tendon tails until calibration is finished. Trim only after a full open/close cycle works.

## Per-finger tuning (main firmware)

Each finger has three calibration values:

```cpp
int  OPEN_A[6]  = { ... };  // servo angle at fully open   (0 %)
int  CLOSE_A[6] = { ... };  // servo angle at fully closed (100 %)
bool INVERT[6]  = { ... };  // true where the tendon pulls the opposite way
```

Procedure, one finger at a time:

1. Command the finger to 0 % — adjust `OPEN_A` until the finger is straight **without** the servo buzzing against the stop. A buzzing servo is stalled: it will overheat and the line will creep.
2. Command 100 % — adjust `CLOSE_A` until closed with firm (not straining) tension.
3. If the finger moves the wrong way, flip `INVERT` for that channel instead of swapping lines.
4. Cycle 0 → 100 → 0 ten times; re-check. Tendons seat and stretch slightly on the first cycles — expect one re-tune after the first day.

## Wrist note

The RotaWrist gear drive routes lines through the axis center; calibrate it last, after finger lines are tensioned, or wrist rotation will change finger tension.

## Symptoms table

| Symptom | Cause | Fix |
|---|---|---|
| Servo hums at rest | End angle past mechanical stop | Back off `OPEN_A`/`CLOSE_A` a few degrees |
| Finger droops over days | Line creep / knot slip | Re-tension; check knot, not the servo |
| One finger reversed | Tendon crossed | Flip `INVERT`, don't re-rig |
| Random twitch all fingers | Missing common ground / brown-out | See `wiring.md` rules 1–2 |
