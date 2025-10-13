# death-poc

Collection of working proof-of-concepts for the "Death Fortune Teller" animatronic skeleton build. Each folder captures one hardware or firmware experiment that feeds into the final prop. This repository was initialized locally with `git init`; add a GitHub remote called `death-poc` when you are ready to publish.

## Proofs of Concept

### esp32-esp32-matter-link/
Matter master/slave link between a Freenove ESP32-S3-WROOM (master) and an ESP32-C3 SuperMini (Matter node) connected over UART.
- Read `START-HERE.md` first; it points to `POC-SUMMARY.md`, `QUICK-REFERENCE.md`, and `CHECKLIST.md` for deep dives and copy-ready code.
- `esp32-matter-master/esp32-matter-master.ino` exposes a CLI that drives the S3 side of the UART/Matter bridge.
- `esp32-matter-node/firmware/main/` contains the ESP-IDF + ESP-Matter project for the C3. Use `sdkconfig.defaults.*` and follow `SETUP.md` to rebuild; build output, component caches, and generated sdkconfig files are ignored.
- Proves five Matter endpoints (one trigger, four mutually exclusive modes), CRC-protected UART protocol, commissioning workflow, and all documented HomeKit edge cases.

### thermal-printer-test/
End-to-end validation of a CSN-A1X thermal receipt printer driven by an ESP32 at 9600 8N1.
- `thermal-printer-test.ino` initializes UART2 (GPIO 18/19), streams raster bitmaps with the Epson `GS v 0` command, and demonstrates line feeds and bitmap centering.
- Companion `README.md` documents wiring, ESC/POS command matrix, and troubleshooting notes collected while bringing the printer online.
- Uses the bitmap assets generated in `black-and-white-bitmap-converter/` (`PirateLogo.h`) to confirm raster printing.

### black-and-white-bitmap-converter/
Python utility for turning grayscale PNG art into 1-bit packed headers consumable by thermal printers or embedded displays.
- Run `python3 png2c.py <image.png> <max_width> <Symbol> [--style=string]` (requires Pillow) to create `<Symbol>.h` with `PROGMEM` bitmap data and width/height defines.
- Includes sample input (`pirate-crossbones.png`) and the generated assets used by the thermal printer test.

### finger-detector-test/
Simple capacitive touch experiment for finger detection on the ESP32.
- Sketch `finger-detector-test.ino` tunes the touch sensor (T9 / GPIO32) with long integration cycles, adaptive filtering, and debounce logic to flash the onboard LED when a hand approaches fast or close enough.
- Serves as the basis for future gesture or presence sensing around the prop.

## Next Steps
1. Create the remote repository (e.g., `github.com/<org>/death-poc`) and run `git remote add origin <url>`.
2. Commit the curated sources once you review the generated `.gitignore` entries below.
3. Push when you are ready to share the POCs with collaborators.
