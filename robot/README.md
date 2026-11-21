# Critter truck

A six-wheeled, pickup-shaped home robot. Its face is a screen at the very front, a small crane-style arm sits behind the cab, and the back is an open load bay. Claude decides what it says and does; a Raspberry Pi on board does the driving.

This folder is separate from the website in the rest of the repo.

## The design

| Part | Decision |
|---|---|
| Size | About 480 x 320 mm, lives on the floor |
| Frame | Bolt-together aluminium channel; 3D-printed body panels, cab and bay |
| Drive | Six identical 100 mm wheels, one geared motor with a position sensor per wheel |
| Head | 5.5" AMOLED touch screen and its own Raspberry Pi, on a pan-tilt neck, liftable |
| Body brain | Raspberry Pi 5 with an AI accelerator board |
| Senses | Laser scanner, depth camera, rear camera, bump skirt, stair-edge sensors |
| Arm | Open-source printable arm with smart servos (SO-101), on a turntable behind the cab |
| Load bay | About 300 x 250 mm, drop tailgate, accessory rails and power sockets |
| Power | 12.8 V lithium iron phosphate pack in the frame |
| Garage | Wall charger it reverses onto, with a spare head socket on the roof |
| Safety | Emergency stop button, fuses, low voltage on all exposed contacts |

## Build order

Each phase ends with something that works on its own.

### Phase 1: the head, on a desk

A talking face on wall power. No battery, no soldering.

| Part | Notes | Roughly (USD) |
|---|---|---|
| Raspberry Pi 5, 8 GB | With the official 27 W power supply and active cooler | check current price |
| microSD card, 64 GB or more | A2 rated | $10–15 |
| Waveshare 5.5" HDMI AMOLED, 1080 x 1920, capacitive touch | The face | $120–180 |
| USB speakerphone | One puck with microphone and speaker, with echo cancelling | $30–80 |
| Raspberry Pi Camera Module 3 Wide | Plus the narrow camera cable the Pi 5 needs | $35 |

The face software already exists: see "Try the face now" below.

### Phase 2: rolling chassis

Frame, six motors, three dual motor controllers (RoboClaw 2x15A, about $100 each), the battery, the body's Pi, and the emergency stop. Driven from a phone. The frame parts are ordered only after the frame is drawn, because channel lengths and hole patterns have to match the motors and wheels chosen.

### Phase 3: sight and maps

Laser scanner (RPLIDAR C1, about $70–80), depth camera, and the map of the house. Then the garage, and driving home to it.

### Phase 4: the arm

Two SO-101 arms: one mounted on the truck, and a twin you move by hand to teach it. Kits run about $190–300 per arm, less if you print the parts yourself. Reach is about 500 mm; it lifts small objects up to about 500 g.

### Phase 5: bodywork and extras

Printed panels, the neck, working lights, the head's own battery, then bay accessories.

## Try the face now

Needs [Node.js](https://nodejs.org) 22.18 or newer. No robot and no API key required.

```bash
cd robot/relay
npm install
npm run fake
```

Then open `http://localhost:8787/face` for the face and `http://localhost:8787` in a second window to type to it. Tap the face to poke it. `http://localhost:8787/face?demo` cycles through every mood.

For real replies from Claude, get an API key from [platform.claude.com](https://platform.claude.com) and run it with the key instead:

```bash
export ANTHROPIC_API_KEY=your-key-here
npm start
```

## Relay settings

| Variable | Meaning | Default |
|---|---|---|
| `ANTHROPIC_API_KEY` | Your Claude API key | required |
| `CRITTER_TOKEN` | Shared password; set the same value as `RELAY_TOKEN` in `config.h` | none |
| `CRITTER_MODEL` | Which Claude model to use | `claude-opus-5-5` |
| `CRITTER_MEMORY` | Where the memory file lives | `memory.json` |
| `PORT` | Port to listen on | `8787` |

Each poke, puff, or message is one Claude API request and is billed to your key.

## What is in this folder

- `relay/` — the program that connects the robot to Claude, keeps its memory, and serves the face and the phone page. It will move onto the truck's Pi.
- `firmware/` — an earlier small-screen prototype for an ESP32 board with a tiny OLED. Not part of the truck; kept for reference.

## Battery safety

- Use only packs with a built-in protection board, and a charger made for that battery type.
- Fit a fuse next to the battery before anything else is connected.
- Never charge a cell that is puffy, dented, or punctured.
