# Arduino Coding Style Guide

Reference for maintaining consistent coding style across Arduino / ESP8266 projects.

---

## File Structure & Sections

Organise code into clearly labelled sections using **asterisk block dividers**. Each section is wrapped with a matching pair of comment lines:

```cpp
//************* SECTION NAME *****************
// Brief description of what this section contains
int myVariable = 10;
//********************************************
```

- Dividers are `//` followed by asterisks, padded to roughly equal width (~42 chars)
- The section title is UPPERCASE, centred between the asterisks
- A blank line separates each section from the next
- One blank line between sections, no extra blank lines within a section

### Standard Section Order

1. Header files (includes)
2. Pin setup (defines & pin variables)
3. Structs
4. Data tables / constant arrays
5. Configuration values (offsets, timing, etc.)
6. Global instances (servo, sensor objects)
7. Helper / utility functions
8. Action functions (hardware operations)
9. `setup()`
10. `loop()`

---

## Naming Conventions

| Element | Convention | Example |
|---|---|---|
| Functions | PascalCase, verb-first | `CollectCandy()`, `MoveSlideTo()` |
| Global variables | PascalCase | `TopServoPin`, `SlideServoOffset` |
| Local variables | camelCase | `colorAngle`, `minDistance` |
| Constants / defines | UPPER_SNAKE_CASE | `SDA_PIN`, `SCL_PIN` |
| LED pin defines | PascalCase prefix + channel | `LedR`, `LedG`, `LedB` |
| Structs | PascalCase | `Color`, `LedColor` |
| Struct fields | camelCase | `name`, `angle` |

---

## Comments

### Section Headers
Use the asterisk block divider pattern (see above). Include a `// FN` suffix for function sections:

```cpp
//************ COLLECT CANDY FN **************
void CollectCandy() {
  ...
}
//********************************************
```

### Inline Comments
- Use `//` with a space after the slashes
- Place on the same line for pin mappings and short notes
- Place above the line for explanations of logic

```cpp
#define SDA_PIN 5  // D1 - Blue
// Normalize RGB values (scale to 255 based on the highest value)
uint16_t maxVal = max(r, max(g, b));
```

### Serial Debug Banners
Use `#` characters for section headers, menus, and status banners:

```cpp
Serial.println("############# Menu ##############");
Serial.println("Press 'S' to start");
Serial.println("#################################");
```

Use `-` characters for data output separators within a section:

```cpp
Serial.println("--------------------------------");
Serial.print("Red: ");
Serial.println(normR);
Serial.println("--------------------------------");
```

Banner widths do not need to be exact, but keep them consistently ~33 chars.

### `[TAG]` Log-Level Messages
Use bracketed uppercase tags for operational messages that run in a loop or
during automated operation. Tags make it easy to grep serial output:

| Tag | Use |
|---|---|
| `[WARN]` | Recoverable errors (sensor glitch, I2C reinit) |
| `[AUTO]` | AutoRange gain changes |
| `[SCAN]` | Per-cycle scan results |
| `[FADE]` | LED transition start |

```cpp
Serial.println("[WARN] Sensor glitch — re-initializing I2C");
Serial.println("[AUTO] Gain 4X (dark/medium surface)");
```

### Structured Single-Line Output
When a line contains multiple fields, build it with `sprintf` into a char
buffer and print once. Avoids multiple UART flushes and keeps output atomic:

```cpp
char buf[56];
sprintf(buf, "[SCAN] R=%d G=%d B=%d  #%02X%02X%02X  dist=%.1f  -> %s",
        r, g, b, r, g, b, dist, status);
Serial.println(buf);
```

### Key-Value Pairs
For single key-value pairs use separate `Serial.print` / `Serial.println` calls:

```cpp
Serial.print("Colour is ");
Serial.println(closestColor);
```

### Prompt Messages
Interactive prompts use plain sentences without banner wrappers:

```cpp
Serial.println("Press 'S' to scan a colour");
```

---

## Formatting

- **Indentation**: 2 spaces (no tabs)
- **Braces**: opening brace on the same line as the statement
- **Blank lines**: one blank line between sections; no trailing blank lines inside functions except to separate logical groups
- **Line length**: no strict limit, but keep reasonable (~80–100 chars)

```cpp
if (angle >= 0) {
  SlideServo.attach(SlideServoPin, 544, 2400);
  SlideServo.write(angle + SlideServoOffset);
  delay(slideDelay);
  SlideServo.detach();
}
```

---

## Hardware Patterns

### Servo Usage
Always attach before writing, detach after the delay:

```cpp
void SomeServoAction() {
  MyServo.attach(pin, 544, 2400);
  MyServo.write(targetAngle + offset);
  Serial.println("Action description");
  delay(actionDelay);
  MyServo.detach();
}
```

### Timing
Use named delay variables rather than magic numbers:

```cpp
int collectDelay = 800;
int moveDelay = 1000;
```

### Pin Definitions
Group by peripheral, annotate with board label and wire colour.

Standard pin assignments shared across all projects:

```cpp
// I2C colour sensor
#define SDA_PIN 5  // D1 - Blue
#define SCL_PIN 4  // D2 - Green

// Servo pins
#define TopServoPin   14  // D5 - Yellow
#define SlideServoPin 12  // D6 - Purple

// RGB LED (common-cathode) — all PWM-capable, no conflicts
#define LedR 13  // D7
#define LedG 15  // D8
#define LedB 0   // D3
```

> D7/D8/D3 are the only free PWM-capable pins after I2C and servo pins are reserved. D0 (GPIO16) does **not** support PWM and must not be used for LED channels.

---

## Hardware Tuning & Calibration

### RGB LED & Sensor Color Accuracy

When mixing colors on common-cathode RGB LEDs powered at 3.3V, pure PWM mappings often look skewed. The Red and Green LED dies have lower forward voltages than Blue, resulting in overpowering brightness on those channels. Furthermore, the TCS34725 color sensor illuminator LED can have a cool spectral bias (producing a green/blue tint).

To achieve accurate, "true" colors (such as a pure yellow without a green tint, or a pure red without a pink/magenta cast), use standard **scaling multipliers** applied via a `SetLED` wrapper function, and scale raw inputs from the sensor before normalizing them.

**TCS34725 Sensor Scaling Multipliers:**
```cpp
float SENSOR_SCALE_R = 1.00f;
float SENSOR_SCALE_G = 0.70f; // Mitigates green-tint bias
float SENSOR_SCALE_B = 0.90f; // Mitigates blue-ambient amplification
```

**RGB LED Scaling Multipliers:**
```cpp
const float LED_SCALE_R = 1.00f; // Red scale 
const float LED_SCALE_G = 0.55f; // Drops green power to match red
const float LED_SCALE_B = 0.85f; // Drops blue power to stop it overpowering red
```

**Standard SetLED Wrapper Function:**
Include this function in your sketch and only ever call `SetLED()`, avoiding raw `analogWrite(LedR...)` calls directly so colors are always balanced.
```cpp
// analogWriteRange(255) must be called in setup()
void SetLED(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t rScaled = (uint8_t)min(r * LED_SCALE_R, 255.0f);
  uint8_t gScaled = (uint8_t)min(g * LED_SCALE_G, 255.0f);
  uint8_t bScaled = (uint8_t)min(b * LED_SCALE_B, 255.0f);

  analogWrite(LedR, rScaled);
  analogWrite(LedG, gScaled);
  analogWrite(LedB, bScaled);
}
```

---

## Data Tables

Use arrays of structs for lookup tables. Initialise inline with brace syntax:

```cpp
struct Color {
  const char *name;
  uint16_t r, g, b;
  int angle;
};

Color colors[] = {
  {"Red", 255, 109, 91, 70},
  {"Blue", 144, 237, 255, 128}
};
```

---

## Control Flow

- Use `else if` chains for mutually exclusive serial commands
- Reprint the menu after each command completes
- Use `do...while` for loops that must run at least once

```cpp
if (input.equalsIgnoreCase("s")) {
  // start action
} else if (input.equalsIgnoreCase("c")) {
  // calibrate
} else if (input.equalsIgnoreCase("r")) {
  // read sensor
}
```
