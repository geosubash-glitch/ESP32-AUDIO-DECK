![IMG_20260223_141614](https://github.com/user-attachments/assets/fd115ac1-483a-47d7-8157-12c854c5a1f6)
![IMG_20260223_234533](https://github.com/user-attachments/assets/a194c5ed-702f-4c98-89ec-c3ae5e980f00)

# ESP32 Tactile Audio Deck

A standalone, mixed-signal digital audio player (DAP) and Bluetooth receiver built with a focus on tactile controls and raw, cassette-futurism aesthetics. 

Designed specifically to drive high-sensitivity in-ear monitors cleanly, this deck relies entirely on physical hardware buttons for navigation instead of a touchscreen. It uses a custom Digital Signal Processing (DSP) pipeline to seamlessly switch between streaming Bluetooth audio and parsing local lossless WAV files from an SD card, all running on a standard ESP32.

## ✨ Features
* **Dual-Mode Audio:** Switch instantly between a Bluetooth A2DP Sink (receiver) and a local SD Card WAV player.
* **Tactile Interface:** A 16-key mechanical matrix mapped as a hierarchical D-pad, complete with a hardware-level active buzzer for zero-latency auditory feedback on every press.
* **Real-Time Visualizer:** 128x64 OLED display featuring a live audio visualizer, scrolling track metadata, and an auto-sleep timeout to reduce eye strain in the dark.
* **Custom DSP Pipeline:** Utilizes software volume scaling and mono-to-stereo format conversion to ensure clean audio output even when reading raw files.
* **Modular "Sandwich" Chassis:** Built using M3 hex standoffs and dual perfboards to physically isolate the high-speed digital audio signals from the matrix scanning noise.

---

## 🛠️ Hardware Bill of Materials (BOM)
* **Microcontroller:** Standard ESP32 Dev Module (38-pin)
* **DAC:** I2S Audio DAC (PCM5102A or UDA1334A)
* **Display:** 0.96" SSD1306 OLED (I2C)
* **Input:** 4x4 Matrix Keypad
* **Storage:** MicroSD Card Module (SPI)
* **Feedback:** 5V Active Piezo Buzzer
* **Mounting:** M3 x 12mm Male-to-Female Nylon Standoffs, M3 Nuts, M3 8mm CSK Screws
* **Wiring:** Flexible stranded wire (critical for the service loop)

---

## 🔌 Pinout & Wiring Directory
**Critical Note:** To avoid Guru Meditation Errors and memory crashes, the OLED and SD Card modules are mapped to non-standard pins. This physically dodges the Keypad matrix and protects the I2S audio buffer. 

### 1. I2S DAC (Audio Path)
| DAC Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| **BCLK** | 26 | Bit Clock |
| **WSEL** | 25 | Word Select (L/R) |
| **DIN** | 22 | Data In |
| **VIN / GND** | 5V / GND | Shared Ground. Tie `SCK` to GND if using PCM5102A. |

### 2. SSD1306 OLED (Visual Path)
| OLED Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| **SCL** | 21 | I2C Clock (Running at 400kHz) |
| **SDA** | 17 | I2C Data |

### 3. MicroSD Module (Storage Path)
| SD Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| **CS** | 5 | Chip Select |
| **SCK** | 18 | SPI Clock |
| **MISO** | 34 | Master In Slave Out (Input only pin) |
| **MOSI** | 32 | Master Out Slave In |

### 4. 4x4 Matrix Keypad (Control Path)
| Keypad Pin | ESP32 GPIO | Note |
| :--- | :--- | :--- |
| **Rows 1, 2, 3, 4** | 13, 12, 14, 27 | General Purpose |
| **Cols 1, 2, 3, 4** | 33, 16, 23, 19 | Standardized to avoid SPI conflict |

### 5. Buzzer
**Positive (+)** to **GPIO 4**, **Negative (-)** to **GND**.

---

## 💻 Software Dependencies
You must install the following libraries in your Arduino IDE to compile this firmware:

**Available via Arduino Library Manager:**
* `Keypad` by Mark Stanley, Alexander Brevig
* `Adafruit GFX Library` by Adafruit
* `Adafruit SSD1306` by Adafruit

**Require Manual Installation (GitHub ZIP):**
* `ESP32-A2DP` by Phil Schatzmann
* `arduino-audio-tools` by Phil Schatzmann

---

## 🚀 Flashing Instructions
Because the Bluetooth stack and DSP libraries are quite large, a standard ESP32 partition will fail to compile. 

1. Open the `.ino` file in the Arduino IDE.
2. Go to **Tools** -> **Partition Scheme**.
3. Select **"Huge APP (3MB No OTA/1MB SPIFFS)"**.
4. Compile and Upload.

---

## 🎮 Controls (D-Pad Layout)
* `2` : Navigate Up
* `8` : Navigate Down
* `4` : Navigate Left / Previous Track
* `6` : Navigate Right / Next Track
* `5` : Confirm / Play / Pause
* `A` : Volume Up (+10%)
* `B` : Volume Down (-10%)
* `D` : Hardware Mode Switch (Toggle BT / SD)
* `*` : Open System Menu
* `1` : Toggle Live Audio Visualizer
