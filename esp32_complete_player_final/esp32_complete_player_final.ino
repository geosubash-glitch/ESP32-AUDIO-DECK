/**
 * ============================================================
 *  ESP32 D-Pad Player + SD Card + Bluetooth
 *  Keypad + OLED + SD Module + DAC all working together
 * ============================================================
 *
 *  COMPLETE WIRING:
 *  
 *  UDA1334A DAC:
 *    BCLK → GPIO 26
 *    WSEL → GPIO 25
 *    DIN  → GPIO 22
 *    VIN  → 3.3V
 *    GND  → GND
 *  
 *  OLED Display:
 *    SDA → GPIO 17
 *    SCL → GPIO 21
 *    VCC → 3.3V
 *    GND → GND
 *  
 *  Keypad (WORKING - DON'T CHANGE):
 *    Rows: GPIO 13, 12, 14, 27
 *    Cols: GPIO 33, 16, 23, 19
 *  
 *  SD Card Module (USING ALTERNATIVE PINS):
 *    CS   → GPIO 5
 *    SCK  → GPIO 18
 *    MISO → GPIO 34  ← NEW: avoids conflict with keypad GPIO 19
 *    MOSI → GPIO 32  ← NEW: avoids conflict with keypad GPIO 23
 *    VCC  → 5V
 *    GND  → GND
 *  
 *  Buzzer:
 *    + → GPIO 4
 *    - → GND
 *
 *  MODES:
 *  [1] Bluetooth (default)
 *  [2] SD WAV Player (press D to switch)
 *  
 *  Note: SD can only play WAV files (no MP3/FLAC to save flash space)
 * ============================================================
 */

#include "Arduino.h"
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include "SD.h"
#include "SPI.h"
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Pin Definitions ──────────────────────────────────────────
// I2S DAC
#define I2S_BCLK     26
#define I2S_WSEL     25
#define I2S_DOUT     22

// OLED
#define OLED_SDA     17
#define OLED_SCL     21

// Buzzer
#define BUZZER_PIN   4

// SD Card (alternative SPI pins to avoid keypad conflict)
#define SD_CS        5
#define SD_SCK       18
#define SD_MISO      34   // Changed from 19 (keypad col 4)
#define SD_MOSI      32   // Changed from 23 (keypad col 3)

// ── Keypad Configuration (WORKING - unchanged) ───────────────
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {33, 16, 23, 19};  // KEEP AS-IS
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ── Hardware Objects ─────────────────────────────────────────
Adafruit_SSD1306 display(128, 64, &Wire, -1);
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

// SD playback objects
WAVDecoder wav_decoder;
EncodedAudioStream decoder_stream(&i2s, &wav_decoder);
File audioFile;
File root;
StreamCopy copier(decoder_stream, audioFile);

// ── State Variables ──────────────────────────────────────────
enum PlayMode { MODE_BLUETOOTH, MODE_SD };
enum ScreenMode { SCREEN_PLAYER, SCREEN_VISUALIZER, SCREEN_MENU };

PlayMode currentMode = MODE_BLUETOOTH;
ScreenMode currentScreen = SCREEN_PLAYER;

// Menu items
const int menuItems = 5;
String menuList[] = {
    "Sleep Screen",
    "Disconnect BT",
    "Reconnect BT",
    "Switch to SD",
    "Exit Menu"
};
int menuCursor = 0;

// Player state
String currentTrack = "Waiting...";
int currentVolume = 70;
bool isPlaying = false;
bool isConnected = false;
bool sdAvailable = false;

// Visualizer
int barHeights[16] = {0};
unsigned long lastVisualizerUpdate = 0;

// Scrolling text
int scrollOffset = 0;
unsigned long lastScrollTime = 0;
const int scrollSpeed = 150;

// ── Feedback Functions ───────────────────────────────────────
void click() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(30);
    digitalWrite(BUZZER_PIN, LOW);
}

void longBeep() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
}

// ── Display Functions ────────────────────────────────────────
void drawPlayer() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Status bar
    display.setCursor(0, 0);
    if (currentMode == MODE_BLUETOOTH) {
        display.print(isConnected ? "BT:ON" : "BT:OFF");
    } else {
        display.print("SD:WAV");
    }
    display.print(" | ");
    display.print(isPlaying ? "PLAY" : "STOP");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    // Track name with FIXED scrolling
    display.setCursor(0, 12);
    display.print("Track:");
    
    // Clear the track display area to prevent overlap
    display.fillRect(0, 20, 128, 20, SSD1306_BLACK);
    
    display.setTextSize(2);
    int charWidth = 12;  // Size 2 font = ~12 pixels per char
    int displayWidth = 128;
    String displayTrack = currentTrack;
    
    // Only scroll if text is actually longer than screen
    if (displayTrack.length() * charWidth > displayWidth) {
        // Smooth continuous scrolling
        if (millis() - lastScrollTime > 100) {  // Update every 100ms
            scrollOffset++;
            // Reset when scrolled past the end
            if (scrollOffset > displayTrack.length() * charWidth + displayWidth) {
                scrollOffset = 0;
            }
            lastScrollTime = millis();
        }
        // Draw scrolling text
        display.setCursor(displayWidth - scrollOffset, 22);
        display.print(displayTrack);
        display.print("  ");  // Add space before loop
        // Draw second copy for seamless loop
        display.setCursor(displayWidth - scrollOffset + (displayTrack.length() + 2) * charWidth, 22);
        display.print(displayTrack);
    } else {
        // Short title - center it
        scrollOffset = 0;  // Reset scroll for next long title
        int textWidth = displayTrack.length() * charWidth;
        int xPos = (displayWidth - textWidth) / 2;
        if (xPos < 0) xPos = 0;
        display.setCursor(xPos, 22);
        display.print(displayTrack);
    }
    
    // Volume section - clear area first
    display.fillRect(0, 42, 128, 22, SSD1306_BLACK);
    
    display.setTextSize(1);
    display.setCursor(0, 44);
    int volPercent = (currentVolume * 100) / 127;
    display.print("VOL: ");
    display.print(volPercent);
    display.print("%");
    
    // Volume bar
    int barWidth = (volPercent * 90) / 100;
    display.drawRect(0, 54, 92, 9, SSD1306_WHITE);
    display.fillRect(1, 55, barWidth, 7, SSD1306_WHITE);
    
    // Controls hint
    display.setCursor(94, 54);
    display.print("A/B");
    
    display.display();
}

void drawVisualizer() {
    display.clearDisplay();
    
    // Header - clear area first
    display.fillRect(0, 0, 128, 10, SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("VISUALIZER");
    if (isPlaying) display.print(" LIVE");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    unsigned long t = millis() / 20;
    
    // Clear bar area
    display.fillRect(0, 10, 128, 45, SSD1306_BLACK);
    
    for (int i = 0; i < 16; i++) {
        float phase = (t + i * 20) / 10.0;
        int targetHeight = 0;
        
        if (isPlaying) {
            float wave1 = sin(phase * 0.1 + i * 0.3) * 15;
            float wave2 = sin(phase * 0.05 + i * 0.5) * 10;
            float wave3 = sin(phase * 0.15 - i * 0.2) * 8;
            targetHeight = abs((wave1 + wave2 + wave3)) * (currentVolume / 127.0);
            targetHeight = constrain(targetHeight + 5, 5, 45);  // Reduced max height
        } else {
            targetHeight = 5 + abs(sin(phase * 0.05 + i * 0.2)) * 5;
        }
        
        if (barHeights[i] < targetHeight) {
            barHeights[i] += 4;
        } else if (barHeights[i] > targetHeight) {
            barHeights[i] -= 2;
        }
        
        barHeights[i] = constrain(barHeights[i], 0, 45);
        
        int x = i * 8;
        int h = barHeights[i];
        
        // Draw bar from bottom up
        int barY = 55 - h;  // Start from y=55
        display.fillRect(x, barY, 6, h, SSD1306_WHITE);
        
        // Peak line
        if (h > 3) {
            display.drawLine(x, barY - 1, x + 5, barY - 1, SSD1306_WHITE);
        }
    }
    
    // Footer - clear area first
    display.fillRect(0, 56, 128, 8, SSD1306_BLACK);
    display.drawLine(0, 55, 128, 55, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("VOL:");
    display.print((currentVolume * 100) / 127);
    display.print("%");
    
    // Mode indicator
    display.setCursor(70, 56);
    display.print(currentMode == MODE_BLUETOOTH ? "BT" : "SD");
    
    display.display();
}

void drawMenu() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Header
    display.setCursor(0, 0);
    display.println("=== MENU ===");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    
    // Menu items with proper spacing
    int startY = 14;
    int itemHeight = 9;  // Consistent spacing
    
    for (int i = 0; i < menuItems; i++) {
        int yPos = startY + (i * itemHeight);
        
        // Clear the line first
        display.fillRect(0, yPos, 128, itemHeight, SSD1306_BLACK);
        
        display.setCursor(0, yPos);
        
        if (i == menuCursor) {
            display.print(">");
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            display.print(" ");
        } else {
            display.print("  ");
            display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }
        
        // Truncate long menu items
        String item = menuList[i];
        if (item.length() > 18) {
            item = item.substring(0, 17) + "~";
        }
        display.println(item);
        display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    
    // Footer with clear background
    display.fillRect(0, 56, 128, 8, SSD1306_BLACK);
    display.drawLine(0, 55, 128, 55, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print("2/8:Nav 5:OK 4:Back");
    
    display.display();
}

// ── Bluetooth Callbacks ──────────────────────────────────────
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
    if (id == 1 && currentMode == MODE_BLUETOOTH) {
        currentTrack = String((char*)text);
        Serial.printf("[BT TRACK] %s\n", currentTrack.c_str());
        if (currentScreen == SCREEN_PLAYER) drawPlayer();
    }
}

void audio_state_callback(esp_a2d_audio_state_t state, void* ptr) {
    if (currentMode == MODE_BLUETOOTH) {
        isPlaying = (state == ESP_A2D_AUDIO_STATE_STARTED);
        if (currentScreen == SCREEN_PLAYER) drawPlayer();
    }
}

void connection_callback(esp_a2d_connection_state_t state, void* ptr) {
    isConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    if (!isConnected && currentMode == MODE_BLUETOOTH) {
        currentTrack = "Disconnected";
    }
    Serial.printf("[BT] %s\n", isConnected ? "Connected" : "Disconnected");
    if (currentScreen == SCREEN_PLAYER) drawPlayer();
}

// ── SD Player Functions ──────────────────────────────────────
void playNextWAV() {
    if (audioFile) audioFile.close();
    
    int attempts = 0;
    int maxAttempts = 50;  // Prevent infinite loop
    
    while (attempts < maxAttempts) {
        File entry = root.openNextFile();
        
        if (!entry) {
            Serial.println("[SD] End of playlist, restarting");
            root.rewindDirectory();
            attempts++;
            continue;
        }
        
        String name = String(entry.name());
        String lower = name;
        lower.toLowerCase();
        
        // Skip hidden files
        if (lower.startsWith(".") || lower.startsWith("._")) {
            entry.close();
            continue;
        }
        
        // Check if it's a WAV file
        if (lower.endsWith(".wav")) {
            Serial.printf("[SD] Trying: %s (%u bytes)\n", entry.name(), entry.size());
            
            // Validate WAV header before playing
            uint8_t header[44];
            size_t bytesRead = entry.read(header, 44);
            
            // Check RIFF signature
            bool validWAV = (bytesRead == 44 && 
                           header[0] == 'R' && header[1] == 'I' && 
                           header[2] == 'F' && header[3] == 'F' &&
                           header[8] == 'W' && header[9] == 'A' && 
                           header[10] == 'V' && header[11] == 'E');
            
            if (!validWAV) {
                Serial.printf("[SD] SKIP: Invalid WAV header - %s\n", entry.name());
                entry.close();
                currentTrack = "Bad WAV: " + name;
                drawPlayer();
                delay(1000);
                continue;
            }
            
            // Rewind to start for playback
            entry.seek(0);
            
            currentTrack = name;
            audioFile = entry;
            
            // Begin playback (returns void, can't check success directly)
            Serial.printf("[SD] ✓ Playing: %s\n", entry.name());
            copier.begin(decoder_stream, audioFile);
            isPlaying = true;
            drawPlayer();
            return;
        }
        
        entry.close();
    }
    
    // If we get here, no valid files found
    Serial.println("[SD] ERROR: No playable WAV files found!");
    currentTrack = "No valid WAV files";
    isPlaying = false;
    drawPlayer();
}

void switchToBluetooth() {
    Serial.println("[MODE] Switching to Bluetooth");
    
    // Stop SD
    copier.end();
    decoder_stream.end();
    if (audioFile) audioFile.close();
    if (root) root.close();
    i2s.end();
    delay(200);
    
    // Start Bluetooth
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = I2S_BCLK;
    cfg.pin_ws = I2S_WSEL;
    cfg.pin_data = I2S_DOUT;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    cfg.i2s_format = I2S_STD_FORMAT;
    cfg.use_apll = false;
    cfg.buffer_count = 32;
    cfg.buffer_size = 2048;
    i2s.begin(cfg);
    
    currentMode = MODE_BLUETOOTH;
    currentTrack = isConnected ? "Ready" : "Pair phone";
    isPlaying = false;
    drawPlayer();
}

void switchToSD() {
    if (!sdAvailable) {
        Serial.println("[ERROR] SD card not available");
        currentTrack = "No SD card!";
        drawPlayer();
        delay(2000);
        return;
    }
    
    Serial.println("[MODE] Switching to SD");
    
    // Stop Bluetooth
    a2dp_sink.end();
    i2s.end();
    delay(200);
    
    // Start SD playback
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = I2S_BCLK;
    cfg.pin_ws = I2S_WSEL;
    cfg.pin_data = I2S_DOUT;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    cfg.i2s_format = I2S_STD_FORMAT;
    cfg.buffer_count = 16;
    cfg.buffer_size = 1024;
    i2s.begin(cfg);
    
    decoder_stream.begin();
    root = SD.open("/");
    
    currentMode = MODE_SD;
    playNextWAV();
}

// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   ESP32 D-Pad + SD + Bluetooth       ║");
    Serial.println("╚══════════════════════════════════════╝\n");
    
    // Buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("[ERROR] OLED failed");
    } else {
        Serial.println("[OK] OLED ready");
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("ESP32");
    display.setTextSize(1);
    display.setCursor(15, 40);
    display.println("Audio Player");
    display.display();
    delay(1500);
    
    // SD Card with alternative SPI pins
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (SD.begin(SD_CS, SPI)) {
        sdAvailable = true;
        Serial.println("[OK] SD card detected");
        Serial.printf("     Size: %llu MB\n", SD.cardSize() / (1024 * 1024));
    } else {
        Serial.println("[WARN] No SD card - Bluetooth mode only");
    }
    
    // Start in Bluetooth mode
    auto cfg = i2s.defaultConfig(TX_MODE);
    cfg.pin_bck = I2S_BCLK;
    cfg.pin_ws = I2S_WSEL;
    cfg.pin_data = I2S_DOUT;
    cfg.sample_rate = 44100;
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    cfg.i2s_format = I2S_STD_FORMAT;
    cfg.use_apll = false;
    cfg.buffer_count = 32;
    cfg.buffer_size = 2048;
    i2s.begin(cfg);
    
    a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
    a2dp_sink.set_on_audio_state_changed(audio_state_callback);
    a2dp_sink.set_on_connection_state_changed(connection_callback);
    a2dp_sink.start("ESP32_Audio_Deck");
    a2dp_sink.set_volume(currentVolume);
    
    Serial.println("[OK] Bluetooth started");
    Serial.println("\n[CONTROLS]");
    Serial.println("  A=Vol+ B=Vol- 5=Play 4=Prev 6=Next");
    Serial.println("  1=Visual *=Menu D=Switch Mode\n");
    
    drawPlayer();
    longBeep();
}

// ─────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────
void loop() {
    char key = keypad.getKey();
    
    // SD playback loop
    if (currentMode == MODE_SD && isPlaying) {
        if (!copier.copy()) {
            playNextWAV();
        }
    }
    
    // Visualizer animation
    if (currentScreen == SCREEN_VISUALIZER && millis() - lastVisualizerUpdate > 30) {
        drawVisualizer();
        lastVisualizerUpdate = millis();
    }
    
    // Scrolling text - only update when needed
    if (currentScreen == SCREEN_PLAYER && 
        currentTrack.length() * 12 > 128 &&
        millis() - lastScrollTime >= 100) {
        drawPlayer();
    }
    
    if (key) {
        Serial.printf("[KEY] %c\n", key);
        
        // Global controls
        if (key == 'A') {
            click();
            currentVolume = min(currentVolume + 10, 127);
            if (currentMode == MODE_BLUETOOTH) {
                a2dp_sink.set_volume(currentVolume);
            }
            if (currentScreen == SCREEN_PLAYER) drawPlayer();
            return;
        }
        
        if (key == 'B') {
            click();
            currentVolume = max(currentVolume - 10, 0);
            if (currentMode == MODE_BLUETOOTH) {
                a2dp_sink.set_volume(currentVolume);
            }
            if (currentScreen == SCREEN_PLAYER) drawPlayer();
            return;
        }
        
        if (key == '1') {
            longBeep();
            currentScreen = (currentScreen == SCREEN_VISUALIZER) ? SCREEN_PLAYER : SCREEN_VISUALIZER;
            if (currentScreen == SCREEN_PLAYER) drawPlayer();
            return;
        }
        
        if (key == '*') {
            longBeep();
            currentScreen = (currentScreen == SCREEN_MENU) ? SCREEN_PLAYER : SCREEN_MENU;
            if (currentScreen == SCREEN_MENU) {
                menuCursor = 0;
                drawMenu();
            } else {
                drawPlayer();
            }
            return;
        }
        
        if (key == 'D') {
            longBeep();
            if (currentMode == MODE_BLUETOOTH) {
                switchToSD();
            } else {
                switchToBluetooth();
            }
            return;
        }
        
        // Player controls
        if (currentScreen == SCREEN_PLAYER) {
            click();
            
            if (key == '5') {
                if (currentMode == MODE_BLUETOOTH) {
                    if (a2dp_sink.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) {
                        a2dp_sink.pause();
                    } else {
                        a2dp_sink.play();
                    }
                } else {
                    isPlaying = !isPlaying;
                    if (isPlaying) playNextWAV();
                }
            }
            else if (key == '4' && currentMode == MODE_BLUETOOTH) {
                a2dp_sink.previous();
            }
            else if (key == '6') {
                if (currentMode == MODE_BLUETOOTH) {
                    a2dp_sink.next();
                } else {
                    playNextWAV();
                }
            }
        }
        
        // Menu controls
        else if (currentScreen == SCREEN_MENU) {
            click();
            
            if (key == '2') {
                menuCursor = (menuCursor - 1 + menuItems) % menuItems;
                drawMenu();
            }
            else if (key == '8') {
                menuCursor = (menuCursor + 1) % menuItems;
                drawMenu();
            }
            else if (key == '4') {
                currentScreen = SCREEN_PLAYER;
                drawPlayer();
            }
            else if (key == '5') {
                longBeep();
                
                switch (menuCursor) {
                    case 0:
                        display.ssd1306_command(SSD1306_DISPLAYOFF);
                        break;
                    case 1:
                        if (currentMode == MODE_BLUETOOTH) {
                            a2dp_sink.disconnect();
                        }
                        break;
                    case 2:
                        if (currentMode == MODE_BLUETOOTH) {
                            a2dp_sink.reconnect();
                        }
                        break;
                    case 3:
                        currentScreen = SCREEN_PLAYER;
                        if (currentMode == MODE_BLUETOOTH) {
                            switchToSD();
                        } else {
                            switchToBluetooth();
                        }
                        break;
                    case 4:
                        currentScreen = SCREEN_PLAYER;
                        drawPlayer();
                        break;
                }
            }
        }
    }
    
    delay(10);
}
