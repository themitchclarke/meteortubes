/*
  Meteor Tubes - Arduino IoT Cloud LED Controller
  https://create.arduino.cc/cloud/things/1d5abbc7-2f4a-45d4-9790-b728a967581f

  Arduino IoT Cloud Variables:

  int activeEffect;           // Effect selector (0=off, 1-7=effects)
  int cloudPaletteSetIndex;   // Palette set selector for Pacifica effect
  int cloudSelectedPalette;   // Palette selector for Juggle effect
  int effectColor;            // Color selector for compatible effects

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/

#include "thingProperties.h"
#include <FastLED.h>

// ============================================================================
// EFFECT DEFINITIONS
// ============================================================================
// Effect IDs for activeEffect cloud variable
#define EFFECT_OFF              0
#define EFFECT_PACIFICA         1
#define EFFECT_GLITTER          2
#define EFFECT_RAINBOW          3
#define EFFECT_DRIP             4
#define EFFECT_MATRIX           5
#define EFFECT_COUNTDOWN        6
#define EFFECT_JUGGLE           7
// Christmas Effects
#define EFFECT_CANDY_CANE       8
#define EFFECT_CHRISTMAS_SPARKLE 9
#define EFFECT_CHRISTMAS_CHASE  10
#define EFFECT_CHRISTMAS_TWINKLE 11
#define EFFECT_HOLLY            12

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define NUM_LEDS_PER_TUBE 60
#define NUM_TUBES 5
#define TOTAL_LEDS (NUM_LEDS_PER_TUBE * NUM_TUBES)
#define DATA_PIN 5

// Power supply constraints
#define POWER_SUPPLY_VOLTAGE 5
#define MAX_CURRENT_MILLIAMPS 10000

// ============================================================================
// GLOBAL STATE
// ============================================================================
CRGB leds[TOTAL_LEDS];
CRGB effectColorCRGB = CRGB::Blue;
CRGB FullWhite = CRGB(255, 255, 255);
bool countdownComplete = false;

// ============================================================================
// COLOR PALETTES
// ============================================================================

// Christmas Colors
#define CHRISTMAS_RED    CRGB(220, 20, 20)
#define CHRISTMAS_GREEN  CRGB(20, 150, 20)
#define CHRISTMAS_GOLD   CRGB(255, 200, 50)
#define CHRISTMAS_WHITE  CRGB(255, 255, 255)

CRGBPalette16 pacifica_palette_1 =
    {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
     0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0xb53dff, 0x8d00c4};
CRGBPalette16 pacifica_palette_2 =
    {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
     0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F};
CRGBPalette16 pacifica_palette_3 =
    {0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
     0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF};
CRGBPalette16 aurora_palette_1 =
    {0x001814, 0x003E32, 0x006646, 0x00A259, 0x00CF76, 0x1CF89E, 0x64F7C8, 0xB1FEFF,
      0x78A5FF, 0x5C70F2, 0x7348D4, 0xA536CC, 0xCC35A3, 0xFF71A8, 0xFFC3D6, 0xFFC3D6};
CRGBPalette16 aurora_palette_2 =
    {0x020009, 0x09000F, 0x0F0015, 0x15001C, 0x1C0023, 0x22002A, 0x290031, 0x2F0038,
      0x36003F, 0x3D0046, 0x0C5F52, 0x19BE5F, 0x19BE5F, 0x19BE5F, 0x19BE5F, 0x19BE5F};  // Duplicated the last color to fill the 16 slots
CRGBPalette16 aurora_palette_3 =
    {0x060016, 0x08001F, 0x12002B, 0x1C0037, 0x260043, 0x30004F, 0x3A005B, 0x440067,
      0x4E0073, 0x600080, 0x4010FF, 0x6020FF, 0x6020FF, 0x6020FF, 0x6020FF, 0x6020FF};

DEFINE_GRADIENT_PALETTE( pal_Aurora ){
    /* Edit this gradient at https://eltos.github.io/gradient/#pal_Aurora=0:000967-11.4:550081-27.2:42027B-37.8:000967-50:9A182B-84.8:000967-100:000967 */
      0,    0,   9, 103,
     29,   85,   0, 129,
     69,   66,   2, 123,
     96,    0,   9, 103,
    127,  154,  24,  43,
    216,    0,   9, 103,
    255,    0,   9, 103
      };

DEFINE_GRADIENT_PALETTE(pal_Rainbow){
    0,   255,   0,   0,   // Red
   85,     0, 255,   0,   // Green
  170,     0,   0, 255,   // Blue
  255,   255,   0,   0    // Red again
      };

DEFINE_GRADIENT_PALETTE(pal_Forest){
    0,     0, 128,   0,   // Dark Green
   85,    34, 139,  34,   // Forest Green
  170,    50, 205,  50,   // Lime Green
  255,     0, 128,   0    // Dark Green again
 };

DEFINE_GRADIENT_PALETTE(pal_Valentine){
    0,   128,   0,   0,   // Deep Red
   85,   220,  20,  60,   // Crimson
  170,   255, 105, 180,   // Hot Pink
  255,   255,  182, 193    // Light Pink
 };

DEFINE_GRADIENT_PALETTE(pal_Valentine_alt){
    0,   128,   0,   0,   // Deep Red
   85,   180,  30,  60,   // Rose Red
  170,   220,  70, 130,   // Fuchsia/Magenta
  255,   160,  80, 180    // Soft Purple (instead of light pink)
};

const TProgmemRGBGradientPaletteRef palettes[] = {
  pal_Aurora,
  pal_Rainbow,
  pal_Forest,
  pal_Valentine,
  pal_Valentine_alt
};

// Define sets of palettes
CRGBPalette16 pacifica_palette_set_1[] = {
    pacifica_palette_1,
    pacifica_palette_2,
    pacifica_palette_3
};

CRGBPalette16 pacifica_palette_set_2[] = {
    aurora_palette_1,
    aurora_palette_2,
    aurora_palette_3
};

CRGBPalette16 pacifica_palette_set_3[] = {
    LavaColors_p,
    PartyColors_p,
    HeatColors_p
};

// Array of palette sets
CRGBPalette16 *palette_sets[] = {
    pacifica_palette_set_1,
    pacifica_palette_set_2,
    pacifica_palette_set_3
};

// Total number of sets
const int NUM_PALETTE_SETS = sizeof(palette_sets) / sizeof(palette_sets[0]);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Function to mirror the LEDs in a single tube
void setMirroredLED(int tube, int ledIndex, CRGB color) {
  int tubeStart = tube * NUM_LEDS_PER_TUBE; // Start index of the tube
  leds[tubeStart + ledIndex] = color;       // Set color for the LED
  leds[tubeStart + NUM_LEDS_PER_TUBE - 1 - ledIndex] = color; // Mirror on the opposite side
}

// ============================================================================
// EFFECT IMPLEMENTATIONS
// ============================================================================

// Glitter helper function
void addGlitter(fract8 chanceOfGlitter){
    if (random8() < chanceOfGlitter)
    {
        leds[random16(TOTAL_LEDS)] += FullWhite;
    }
}

// Pacifica effect helper functions
// Add one layer of waves into the led array
void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
    uint16_t ci = cistart;
    uint16_t waveangle = ioff;
    uint16_t wavescale_half = (wavescale / 2) + 20;
    for (uint16_t i = 0; i < TOTAL_LEDS; i++)
    {
        waveangle += 250;
        uint16_t s16 = sin16(waveangle) + 32768;
        uint16_t cs = scale16(s16, wavescale_half) + wavescale_half;
        ci += cs;
        uint16_t sindex16 = sin16(ci) + 32768;
        uint8_t sindex8 = scale16(sindex16, 240);
        CRGB c = ColorFromPalette(p, sindex8, bri, LINEARBLEND);
        leds[i] += c;
    }
}
// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
    uint8_t basethreshold = beatsin8(9, 55, 65);
    uint8_t wave = beat8(7);

    for (uint16_t i = 0; i < TOTAL_LEDS; i++)
    {
        uint8_t threshold = scale8(sin8(wave), 20) + basethreshold;
        wave += 7;
        uint8_t l = leds[i].getAverageLight();
        if (l > threshold)
        {
            uint8_t overage = l - threshold;
            uint8_t overage2 = qadd8(overage, overage);
            leds[i] += CRGB(overage, overage2, qadd8(overage2, overage2));
        }
    }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
    for (uint16_t i = 0; i < TOTAL_LEDS; i++)
    {
        leds[i].blue = scale8(leds[i].blue, 145);
        leds[i].green = scale8(leds[i].green, 200);
        leds[i] |= CRGB(2, 5, 7);
    }
}

void effectPacifica() {

      // Ensure the index is valid
    int selectedSetIndex = constrain(cloudPaletteSetIndex, 0, NUM_PALETTE_SETS - 1);

    // Get the selected set of palettes
    CRGBPalette16 *selectedSet = palette_sets[selectedSetIndex];
  
    // Increment the four "color index start" counters, one for each wave layer.
    // Each is incremented at a different speed, and the speeds vary over time.
    static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
    static uint32_t sLastms = 0;
    uint32_t ms = GET_MILLIS();
    uint32_t deltams = ms - sLastms;
    sLastms = ms;
    uint16_t speedfactor1 = beatsin16(3, 179, 269);
    uint16_t speedfactor2 = beatsin16(4, 179, 269);
    uint32_t deltams1 = (deltams * speedfactor1) / 256;
    uint32_t deltams2 = (deltams * speedfactor2) / 256;
    uint32_t deltams21 = (deltams1 + deltams2) / 2;
    sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
    sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
    sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
    sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

    // Clear out the LED array to a dim background blue-green
    fill_solid(leds, TOTAL_LEDS, CRGB(10, 2, 10));

    // Render each of four layers, with different scales and speeds, that vary over time
    pacifica_one_layer(selectedSet[0], sCIStart1, beatsin16(3, 11 * 256, 14 * 256), beatsin8(10, 70, 130), 0 - beat16(301));
    pacifica_one_layer(selectedSet[1], sCIStart2, beatsin16(4, 6 * 256, 9 * 256), beatsin8(17, 40, 80), beat16(401));
    pacifica_one_layer(selectedSet[2], sCIStart3, 6 * 256, beatsin8(9, 10, 38), 0 - beat16(503));
    pacifica_one_layer(selectedSet[2], sCIStart4, 5 * 256, beatsin8(8, 10, 28), beat16(601));

    // Add brighter 'whitecaps' where the waves lines up more
    pacifica_add_whitecaps();

    // Deepen the blues and greens a bit
    pacifica_deepen_colors();

  FastLED.show();
}

void effectCountdown() {
  // Countdown phase: Flash all LEDs for 10 seconds
  for (int seconds = 10; seconds > 0; seconds--) {
    // Debug: print the remaining seconds
    Serial.print("Countdown: ");
    Serial.println(seconds);

    // Flash all LEDs with a bright white color
    fill_solid(leds, TOTAL_LEDS, CRGB::Red);
    FastLED.show();
    delay(500); // Half a second on

    // Turn off all LEDs
    fill_solid(leds, TOTAL_LEDS, CRGB::Black);
    FastLED.show();
    delay(500); // Half a second off
  }

  // Ensure the countdown ends with LEDs off before starting juggle
  fill_solid(leds, TOTAL_LEDS, CRGB::Black);
  FastLED.show();

  // Debug: indicate transition to juggle
  Serial.println("Starting effectJuggle...");
}


void effectAurora() {
#define UPDATES_PER_SECOND 125
static uint8_t startIndex = 0; // Palette scrolling index
  startIndex++;                  // Increment to create motion

  // Iterate over each tube
  for (int tube = 0; tube < NUM_TUBES; tube++) {
    // Iterate over half of the LEDs per tube (mirrored LEDs will be handled by setMirroredLED)
    for (int i = 0; i < NUM_LEDS_PER_TUBE / 2; i++) {
      // Calculate color index for the aurora effect based on position and scrolling index
      uint8_t colorIndex = (startIndex + (i * 255 / (NUM_LEDS_PER_TUBE / 2))) % 255; // Ensure the index stays within bounds

      // Use CRGBPalette16 type and cast pal_Aurora
      CRGB color = ColorFromPalette((CRGBPalette16)pal_Aurora, colorIndex, 255, LINEARBLEND);
      
      // Set mirrored LEDs
      setMirroredLED(tube, i, color);
    }
  }
  
  // Add a small delay to control update speed
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void effectChase() {
  // Chase up
  for (int i = 0; i < NUM_LEDS_PER_TUBE / 2; i++) { // Only loop over half LEDs for mirroring
    for (int tube = 0; tube < NUM_TUBES; tube++) {
      setMirroredLED(tube, i, effectColorCRGB); // Light up mirrored LED pair
    }
    FastLED.show();
    delay(30);

    // Clear the LEDs as the chase moves up
    for (int tube = 0; tube < NUM_TUBES; tube++) {
      setMirroredLED(tube, i, CRGB::Black); // Turn off mirrored LED pair
    }
  }

  // Chase down
  for (int i = (NUM_LEDS_PER_TUBE / 2) - 1; i >= 0; i--) { // Reverse loop for chase down
    for (int tube = 0; tube < NUM_TUBES; tube++) {
      setMirroredLED(tube, i, effectColorCRGB); // Light up mirrored LED pair
    }
    FastLED.show();
    delay(30);

    // Clear the LEDs as the chase moves down
    for (int tube = 0; tube < NUM_TUBES; tube++) {
      setMirroredLED(tube, i, CRGB::Black); // Turn off mirrored LED pair
    }
  }
}

// Blink effect with mirrored LEDs across all tubes
void effectGlitter() {
  fill_solid(leds, TOTAL_LEDS, effectColorCRGB);
  addGlitter(200);
  FastLED.show();
}

void effectRedWhite() {
  static uint8_t saturation = 0;      // Tracks the current saturation
  static int8_t delta = 10;           // Controls the direction of the transition (increase or decrease)

  for (int tube = 0; tube < NUM_TUBES; tube++) {
    for (int i = 0; i < NUM_LEDS_PER_TUBE / 2; i++) {
      // Set mirrored LEDs transitioning between red and white
      setMirroredLED(tube, i, CHSV(0, saturation, 255)); // Hue=0 (red), animate saturation
    }
  }
  FastLED.show();

  // Update the saturation for the next frame
  saturation += delta;
  if (saturation >= 255 || saturation <= 0) {
    delta = -delta; // Reverse the direction of the transition
  }

  delay(50); // Adjust delay for the desired animation speed
}

// Rainbow effect with mirrored LEDs in each tube
void effectRainbow() {
  static uint8_t hue = 0;
  for (int tube = 0; tube < NUM_TUBES; tube++) {
    for (int i = 0; i < NUM_LEDS_PER_TUBE / 2; i++) {
      setMirroredLED(tube, i, CHSV(hue + (i * 10), 255, 255)); // Set mirrored pair to rainbow
    }
  }
  FastLED.show();
  hue += 5; // Shift hue for animation
  delay(50);
}

void effectDrip() {
  // Static variables to keep track of each tube's state
  static int positions[NUM_TUBES];          // Current drip position for each tube
  static int delays[NUM_TUBES];             // Current delay (gravity effect) for each tube
  static unsigned long timers[NUM_TUBES];   // Next update time for each tube
  static bool initialized = false;          // Initialization flag

  // Initialization: set random starting states for each tube
  if (!initialized) {
    for (int tube = 0; tube < NUM_TUBES; tube++) {
      positions[tube] = random(0, NUM_LEDS_PER_TUBE / 2); // Random start position
      delays[tube] = random(30, 100);                    // Random delay
      timers[tube] = millis() + delays[tube];            // Initial timers
    }
    initialized = true;
  }

  FastLED.clear(); // Clear LEDs for a new frame

  // Process each tube independently
  for (int tube = 0; tube < NUM_TUBES; tube++) {
    if (millis() >= timers[tube]) { // Time to update this tube
      setMirroredLED(tube, positions[tube], effectColorCRGB); // Light up the current position

      // Clear the previous position (only if it's not the start)
      if (positions[tube] > 0) {
        setMirroredLED(tube, positions[tube] - 1, CRGB::Black);
      }

      positions[tube]++; // Move the drip down

      // Apply gravity-like acceleration by reducing the delay
      delays[tube] = max(20, delays[tube] - 5); // Minimum delay of 10ms
      timers[tube] = millis() + delays[tube];   // Set next update time

      // Reset if the drip reaches the end of the tube
      if (positions[tube] >= NUM_LEDS_PER_TUBE / 2) {
        positions[tube] = 0;                    // Restart at the top
        delays[tube] = random(30, 100);         // Reset delay randomly
        setMirroredLED(tube, NUM_LEDS_PER_TUBE / 2 - 1, CRGB::Black); // Clear last LED
      }
    }
  }

  FastLED.show(); // Update LEDs
}

void effectMatrix() {
  // Twinkle logic
  static unsigned long lastUpdate = 0;  // Track last update time
  static int twinkleCount = 0;          // Count of active twinkles
  unsigned long now = millis();

  // Density logic: Determine max number of LEDs twinkling based on twinkleDensity
  int twinkleDensity = 2;
  int twinkleSpeed = 1;
  int maxTwinkles = (TOTAL_LEDS * twinkleDensity) / 100;

  // Update twinkling lights periodically
  if (now - lastUpdate > twinkleSpeed) {
    lastUpdate = now;

    // Randomly activate new twinkles if below density limit
    while (twinkleCount < maxTwinkles) {
      int index = random(0, TOTAL_LEDS);  // Pick a random LED
      if (leds[index] == CRGB::Black) {   // Ensure the LED is currently off
        leds[index] = effectColorCRGB;       // Set to the selected color
        twinkleCount++;
      }
    }

    // Fade all LEDs slightly (simulating twinkle fade-out)
    for (int i = 0; i < TOTAL_LEDS; i++) {
      if (leds[i] != CRGB::Black) {  // If an LED is lit
        leds[i].nscale8(65);        // Scale down brightness (adjust the value as needed)
        if (leds[i].getAverageLight() < 10) {  // Check if brightness is very low
          leds[i] = CRGB::Black;    // Turn off completely
          twinkleCount--;           // Decrement active twinkle count
        }
      }
    }

    FastLED.show();  // Update LEDs
  }
}

void effectJuggle() {
   // Fade the LEDs over time to create the trailing effect
  for (int tube = 0; tube < NUM_TUBES; tube++) {
    fadeToBlackBy(&leds[tube * NUM_LEDS_PER_TUBE], NUM_LEDS_PER_TUBE, 20);
  }

  uint8_t dothue = 0; // Initialize the hue for the dots
  const TProgmemRGBGradientPaletteRef currentPalette = palettes[cloudSelectedPalette];

  // Create 8 bouncing dots
  for (int i = 0; i < 8; i++) {
    // Calculate the LED position using beatsin16
    int position = beatsin16(i + 4, 0, NUM_LEDS_PER_TUBE / 2 - 1);

    // Generate the color for this dot based on the selected palette
    CRGB color = ColorFromPalette((CRGBPalette16)currentPalette, dothue, 255, LINEARBLEND);

    // Set the mirrored LEDs in all tubes
    for (int tube = 0; tube < NUM_TUBES; tube++) {
      setMirroredLED(tube, position, color);
    }

    // Increment the hue for the next dot
    dothue += 32;
  }
  FastLED.show();    // Update the LEDs
  delay(20);         // Adjust the delay for smoother motion
}

// ============================================================================
// CHRISTMAS EFFECTS
// ============================================================================

// Candy Cane: Red and white striped pattern that rotates
void effectCandyCane() {
  static uint8_t rotation = 0;

  for (int tube = 0; tube < NUM_TUBES; tube++) {
    for (int i = 0; i < NUM_LEDS_PER_TUBE / 2; i++) {
      // Create stripes with rotation
      int position = (i + rotation / 4) % 8;
      CRGB color = (position < 4) ? CHRISTMAS_RED : CHRISTMAS_WHITE;
      setMirroredLED(tube, i, color);
    }
  }

  FastLED.show();
  rotation++;
  delay(50);
}

// Christmas Sparkle: Random sparkles of red, green, white, and gold
void effectChristmasSparkle() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();

  // Fade all LEDs
  for (int i = 0; i < TOTAL_LEDS; i++) {
    leds[i].nscale8(240);
  }

  if (now - lastUpdate > 30) {
    lastUpdate = now;

    // Add random sparkles
    for (int i = 0; i < 5; i++) {
      int index = random(0, TOTAL_LEDS);
      int colorChoice = random(0, 4);

      switch(colorChoice) {
        case 0: leds[index] = CHRISTMAS_RED; break;
        case 1: leds[index] = CHRISTMAS_GREEN; break;
        case 2: leds[index] = CHRISTMAS_WHITE; break;
        case 3: leds[index] = CHRISTMAS_GOLD; break;
      }
    }
  }

  FastLED.show();
}

// Christmas Chase: Alternating red and green chase effect
void effectChristmasChase() {
  static uint8_t offset = 0;

  for (int tube = 0; tube < NUM_TUBES; tube++) {
    for (int i = 0; i < NUM_LEDS_PER_TUBE / 2; i++) {
      int position = (i + offset) % 6;
      CRGB color;

      if (position < 2) {
        color = CHRISTMAS_RED;
      } else if (position < 4) {
        color = CHRISTMAS_GREEN;
      } else {
        color = CRGB::Black;
      }

      setMirroredLED(tube, i, color);
    }
  }

  FastLED.show();
  offset++;
  delay(60);
}

// Christmas Twinkle: Twinkling lights like on a Christmas tree
void effectChristmasTwinkle() {
  static unsigned long lastUpdate = 0;
  static int twinkleCount = 0;
  unsigned long now = millis();

  int maxTwinkles = TOTAL_LEDS / 4;

  if (now - lastUpdate > 50) {
    lastUpdate = now;

    // Add new twinkles
    while (twinkleCount < maxTwinkles) {
      int index = random(0, TOTAL_LEDS);
      if (leds[index].getAverageLight() < 50) {
        int colorChoice = random(0, 4);
        switch(colorChoice) {
          case 0: leds[index] = CHRISTMAS_RED; break;
          case 1: leds[index] = CHRISTMAS_GREEN; break;
          case 2: leds[index] = CHRISTMAS_WHITE; break;
          case 3: leds[index] = CHRISTMAS_GOLD; break;
        }
        twinkleCount++;
      }
    }

    // Fade existing lights
    for (int i = 0; i < TOTAL_LEDS; i++) {
      if (leds[i] != CRGB::Black) {
        leds[i].nscale8(200);
        if (leds[i].getAverageLight() < 10) {
          leds[i] = CRGB::Black;
          twinkleCount--;
        }
      }
    }

    FastLED.show();
  }
}

// Holly: Green background with occasional red "berries"
void effectHolly() {
  static unsigned long lastBerry = 0;
  static int berryPositions[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  static uint8_t berryBrightness[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned long now = millis();

  // Fill with green
  fill_solid(leds, TOTAL_LEDS, CHRISTMAS_GREEN);

  // Add red berries that fade in and out
  if (now - lastBerry > 200) {
    lastBerry = now;

    // Add new berry
    for (int i = 0; i < 10; i++) {
      if (berryPositions[i] == -1) {
        berryPositions[i] = random(0, TOTAL_LEDS);
        berryBrightness[i] = 0;
        break;
      }
    }

    // Update existing berries
    for (int i = 0; i < 10; i++) {
      if (berryPositions[i] != -1) {
        // Fade in then out
        if (berryBrightness[i] < 128) {
          berryBrightness[i] += 8;
        } else if (berryBrightness[i] < 255) {
          berryBrightness[i] += 4;
        } else {
          // Remove berry
          berryPositions[i] = -1;
          berryBrightness[i] = 0;
        }

        if (berryPositions[i] != -1) {
          uint8_t brightness = (berryBrightness[i] <= 128) ? berryBrightness[i] * 2 : (255 - berryBrightness[i]) * 2;
          leds[berryPositions[i]] = CRGB(brightness, 0, 0);
        }
      }
    }
  }

  FastLED.show();
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

    // Initialize the FastLED library
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, TOTAL_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(POWER_SUPPLY_VOLTAGE, MAX_CURRENT_MILLIAMPS);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  ArduinoCloud.update();

  // Execute the selected effect
  switch (activeEffect) {
    case EFFECT_PACIFICA:
      effectPacifica();
      break;
    case EFFECT_GLITTER:
      effectGlitter();
      break;
    case EFFECT_RAINBOW:
      effectRainbow();
      break;
    case EFFECT_DRIP:
      effectDrip();
      break;
    case EFFECT_MATRIX:
      effectMatrix();
      break;
    case EFFECT_COUNTDOWN:
      if (!countdownComplete) {
        effectCountdown();
        countdownComplete = true;
      } else {
        effectJuggle();
      }
      break;
    case EFFECT_JUGGLE:
      effectJuggle();
      break;
    case EFFECT_CANDY_CANE:
      effectCandyCane();
      break;
    case EFFECT_CHRISTMAS_SPARKLE:
      effectChristmasSparkle();
      break;
    case EFFECT_CHRISTMAS_CHASE:
      effectChristmasChase();
      break;
    case EFFECT_CHRISTMAS_TWINKLE:
      effectChristmasTwinkle();
      break;
    case EFFECT_HOLLY:
      effectHolly();
      break;
    case EFFECT_OFF:
    default:
      FastLED.clear();
      FastLED.show();
      break;
  }
}

// ============================================================================
// ARDUINO CLOUD CALLBACKS
// ============================================================================

void onActiveEffectChange() {
  // Reset countdown state when switching to countdown effect
  if (activeEffect == EFFECT_COUNTDOWN) {
    countdownComplete = false;
  }
}

void onEffectColorChange() {
  switch (effectColor) {
    case 1: effectColorCRGB = CRGB::Red; break;
    case 2: effectColorCRGB = CRGB::Green; break;
    case 3: effectColorCRGB = CRGB::Blue; break;
    case 4: effectColorCRGB = CRGB::DarkTurquoise; break;
    case 5: effectColorCRGB = CRGB::Cyan; break;
    case 6: effectColorCRGB = CRGB::Magenta; break;
    case 7: effectColorCRGB = CRGB::White; break;
    default: effectColorCRGB = CRGB::Black; break;
  }
}

void onCloudSelectedPaletteChange() {
  // Palette selection changed for Juggle effect
}

void onCloudPaletteSetIndexChange() {
  // Palette set selection changed for Pacifica effect
}

// ============================================================================
// HOW TO ADD NEW EFFECTS
// ============================================================================
/*
 * To add a new LED effect to this project:
 *
 * 1. Define a new effect ID constant at the top of the file (e.g., #define EFFECT_AURORA 13)
 * 2. Implement your effect function (e.g., void effectAurora() { ... })
 * 3. Add a new case to the switch statement in loop() (e.g., case EFFECT_AURORA: effectAurora(); break;)
 * 4. Set activeEffect to your new effect ID from the Arduino Cloud dashboard
 *
 * That's it! No need to add new cloud variables.
 *
 * CURRENT EFFECTS:
 * 0  = Off
 * 1  = Pacifica (ocean waves)
 * 2  = Glitter
 * 3  = Rainbow
 * 4  = Drip
 * 5  = Matrix
 * 6  = Countdown
 * 7  = Juggle
 * 8  = Candy Cane (rotating red/white stripes)
 * 9  = Christmas Sparkle (random red/green/white/gold sparkles)
 * 10 = Christmas Chase (alternating red/green chase)
 * 11 = Christmas Twinkle (twinkling tree lights)
 * 12 = Holly (green with red berry accents)
 *
 * Available effect functions ready to use:
 * - effectAurora() - Aurora-like wave pattern
 * - effectChase() - Chase up/down effect
 * - effectRedWhite() - Red to white fade transition
 *
 * Example: To activate Aurora effect, add this to the definitions and switch statement:
 *   #define EFFECT_AURORA 13
 *   case EFFECT_AURORA: effectAurora(); break;
 */
