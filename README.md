
<img width="1344" height="768" alt="Whisk_daea1c44551e25e8cdf473055c31af51dr" src="https://github.com/user-attachments/assets/f1b9a575-8dff-436e-a986-72afbd0411c1" />

<img width="1344" height="768" alt="Whisk_5f7d61608cdf3d4a9bd4aeee038e435ddr" src="https://github.com/user-attachments/assets/d89846af-86da-438d-8da9-67006435365c" />

# ESP32 TACTILE AUDIO DECK

I built this standalone digital audio player (DAP) and Bluetooth receiver because I wanted a dedicated, single-purpose gadget with actual tactile controls. No touchscreens, no bloat—just physical mechanical buttons and a raw, cassette-futurism vibe. 

I specifically designed the audio pipeline to cleanly drive high-sensitivity gear like my CCA Polaris IEMs. It uses a custom DSP pipeline on a standard ESP32 to seamlessly switch between streaming Bluetooth audio and parsing local lossless WAV files from an SD card.

---

## SYSTEM FEATURES

* [ Dual-Mode Audio ] Instantly toggle between a Bluetooth A2DP Sink (receiver) and a local SD Card WAV player.
* [ 100% Tactile Interface ] A 16-key mechanical matrix mapped as a hierarchical D-pad. I wired in a hardware-level active buzzer so every button press has zero-latency click feedback.
* [ OLED & Visualizer ] 128x64 display featuring a live audio visualizer and scrolling track metadata. I set the screen to auto-sleep after a timeout to save battery and cut out the glare when listening in a dark room.
* [ Clean Audio Pipeline ] Uses software volume scaling and mono-to-stereo format conversion to ensure clean output even when parsing raw files.
* [ Modular "Sandwich" Chassis ] Built using dual perfboards and M3 hex standoffs. This isn't just for aesthetics; it physically isolates the high-speed digital audio signals from the matrix scanning noise.

---

## HARDWARE BOM (Bill of Materials)

* Microcontroller: Standard ESP32 Dev Module (38-pin)
* Audio: I2S Audio DAC (PCM5102A or UDA1334A)
* Display: 0.96" SSD1306 OLED (I2C)
* Input: 4x4 Matrix Keypad
* Storage: MicroSD Card Module (SPI)
* Power (Portable Mod): TP4056 Charging Module + MT3608 Boost Converter (tuned to 5V) + Mechanical Switch
* Feedback: 5V Active Piezo Buzzer
* Hardware: M3 x 12mm Male-to-Female Nylon Standoffs, stranded wire for the service loops.

---

## PINOUT & WIRING DIRECTORY

Figuring out the pinout without causing SPI conflicts or Guru Meditation memory crashes took some trial and error. To protect the I2S audio buffer from the matrix keypad, I mapped the OLED and SD Card modules to these specific alternative pins:

### 1. I2S DAC (Audio Path)
| DAC Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| BCLK | 26 | Bit Clock |
| WSEL | 25 | Word Select (L/R) |
| DIN | 22 | Data In |
| VIN / GND | 5V / GND | Shared Ground. (Tie SCK to GND if using PCM5102A). |

### 2. SSD1306 OLED (Visual Path)
| OLED Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| SCL | 21 | I2C Clock (Running fast at 400kHz to stop audio stutter) |
| SDA | 17 | I2C Data |

### 3. MicroSD Module (Storage Path)
| SD Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| CS | 5 | Chip Select |
| SCK | 18 | SPI Clock |
| MISO | 34 | Master In Slave Out (Input only pin) |
| MOSI | 32 | Master Out Slave In |

### 4. 4x4 Matrix Keypad (Control Path)
| Keypad Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| Rows 1-4 | 13, 12, 14, 27 | Standardized to avoid SPI conflict |
| Cols 1-4 | 33, 16, 23, 19 | Standardized to avoid SPI conflict |

### 5. Buzzer
Positive (+) to GPIO 4, Negative (-) to GND.

---

## SOFTWARE DEPENDENCIES

You need these libraries installed in your Arduino IDE to compile the firmware:

> Arduino Library Manager:
* Keypad by Mark Stanley, Alexander Brevig
* Adafruit GFX Library & Adafruit SSD1306

> Manual GitHub ZIP Install:
* ESP32-A2DP by Phil Schatzmann
* arduino-audio-tools by Phil Schatzmann

---

## FLASHING THE ESP32

Because the Bluetooth stack and DSP libraries are massive, a standard ESP32 partition scheme will fail to compile. 

1. Open the .ino file in the Arduino IDE.
2. Go to Tools -> Partition Scheme.
3. Select "Huge APP (3MB No OTA/1MB SPIFFS)".
4. Compile and Upload.

---

## INTERFACE & MATRIX CONTROLS

* [ 2 ] : Up
* [ 8 ] : Down
* [ 4 ] : Left / Previous Track
* [ 6 ] : Right / Next Track
* [ 5 ] : Confirm / Play / Pause
* [ A ] : Volume Up (+10%)
* [ B ] : Volume Down (-10%)
* [ D ] : Hardware Mode Switch (Toggle BT / SD)
* [ * ] : Open System Menu
* [ 1 ] : Toggle Live Audio Visualizer
---

## DEVELOPMENT ROADMAP

* [ Power Architecture ] Integrate the TP4056 charging module and MT3608 boost converter to support a standalone 3.7V Li-Po battery without audio interference.
* [ DSP Synthesizer ] Program an isolated, monophonic synthesizer mode using the ESP32 to generate raw waveforms, turning the 4x4 matrix into a playable instrument.
* [ Hardware Enclosure ] Design and 3D-print a custom, snap-fit chassis that leans heavily into the cassette-futurism aesthetic to hide the internal PCB sandwich.

---

## LICENSE

This project is licensed under the MIT License.

Copyright (c) 2026 [geosubash]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
