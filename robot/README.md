# Desk critter

A palm-sized robot with two glowing eyes. It reacts when you poke it or blow on it, and you can type to it from your phone. Claude decides what it says and how it looks, and it keeps notes on what it learns about you.

This folder is separate from the website in the rest of the repo.

```
you / phone ──type──▶ relay (your computer) ──▶ Claude
                        ▲   │
              poke/puff │   ▼ what to say + which face
                      critter (ESP32-S3 + OLED)
```

- `firmware/` — the program that runs on the robot.
- `relay/` — a small program on your computer that connects the robot to Claude and holds its memory.

## Shopping list (version 1: new parts, no soldering)

| Part | What to look for | Roughly |
|---|---|---|
| Brain | ESP32-S3 board with a built-in LiPo charger and battery socket. Reference: **Adafruit Feather ESP32-S3** | $18 |
| Screen | 128x64 SSD1306 OLED, I2C. Reference: **Adafruit 0.96" OLED with STEMMA QT** | $13 |
| Screen cable | STEMMA QT / Qwiic cable, 50–100 mm | $1 |
| Battery | 3.7 V LiPo, 400–1200 mAh, **with protection circuit**, JST-PH 2-pin plug | $7–10 |
| USB-C cable | Must carry data, not charge-only | — |
| Enclosure | ABS project box with a **clear lid**, about 100 x 68 x 50 mm | $8 |
| Poke button | 12 mm panel-mount momentary push button with wire leads | $2 |
| Mounting | Double-sided foam tape | $3 |

Buy the battery from the same seller as the board. The plug's plus and minus are not standardised between sellers, and a reversed battery destroys the board.

Worth adding to the same order for the voice stage later: an INMP441 microphone, a MAX98357A amplifier, and a small 4–8 ohm speaker. These need soldering.

A cheaper generic ESP32-S3 dev board and a four-wire OLED also work with this firmware (build with `-e devkit_s3`); they just need jumper wires and have no battery socket.

## Build stages

### 1. Eyes (no Wi-Fi, no battery)

1. Plug the screen into the board with the STEMMA QT cable.
2. Install [VS Code](https://code.visualstudio.com) and its **PlatformIO IDE** extension.
3. Open the `robot/firmware` folder. Copy `critter/config.example.h` to `critter/config.h`.
4. Plug the board into your computer and click **Upload** (the arrow in the bottom bar).

The eyes appear, blink, and glance around. Press the board's BOOT button to tour the nine faces.

### 2. Claude

1. Get an API key from [platform.claude.com](https://platform.claude.com) and install [Node.js](https://nodejs.org) 22.18 or newer.
2. Start the relay on your computer:

   ```bash
   cd robot/relay
   npm install
   export ANTHROPIC_API_KEY=your-key-here
   npm start
   ```

   It prints an address such as `192.168.1.50`.
3. Put your Wi-Fi name, password, and that address into `critter/config.h`, then upload again. The Wi-Fi must be 2.4 GHz.
4. Open `http://<that address>:8787` on your phone and type to it.

The BOOT button is now a poke that Claude reacts to. What it learns is saved in `relay/memory.json`.

To try the relay without a key or a robot, run `npm run fake`.

### 3. Battery

Plug the battery into the board's socket. It charges whenever USB is connected.

### 4. Enclosure

The clear lid means the screen needs no cutout: tape it face-up against the inside of the lid. Tape the battery to the floor of the box and the board on top of it.

Two holes are needed, both in the box wall:

- A slot for the USB-C plug, about 12 x 7 mm, lined up with the board's socket.
- A 12 mm round hole for the poke button, since the board's own button is now out of reach. Wire the button between any free GPIO pin and GND, and set `BUTTON_PIN` in `config.h` to that pin.

The button is the one place this build needs a connection to the board's pins. Either order the board with headers already soldered and use push-on jumper wires, or solder the two wires.

### 5. Later

- Puff sensor: set `PUFF_PIN` in `config.h`. Set `PUFF_DEBUG` to 1 first to see what your sensor reads.
- Voice, a custom printed shell, and the rover base.
- The eco version: salvaged vape cells behind a protection board.

## Relay settings

| Variable | Meaning | Default |
|---|---|---|
| `ANTHROPIC_API_KEY` | Your Claude API key | required |
| `CRITTER_TOKEN` | Shared password; set the same value as `RELAY_TOKEN` in `config.h` | none |
| `CRITTER_MODEL` | Which Claude model to use | `claude-opus-5-5` |
| `CRITTER_MEMORY` | Where the memory file lives | `memory.json` |
| `PORT` | Port to listen on | `8787` |

Each poke, puff, or message is one Claude API request and is billed to your key.

## Battery safety

- Use only cells with a protection circuit.
- Never charge a cell that is puffy, dented, or punctured.
- Charge on a hard, non-flammable surface, and not unattended while you are still getting to know the build.
