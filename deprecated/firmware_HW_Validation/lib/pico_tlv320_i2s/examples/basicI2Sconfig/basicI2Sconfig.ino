#include <Adafruit_TLV320DAC3100.h>

Adafruit_TLV320DAC3100 codec; // Create codec object
#define TLV_RESET 5

void halt(const char *message) {
  Serial.println(message);
  while (1)
    yield(); // Function to halt on critical errors
}

void setup() {
  Serial.begin(115200);

  pinMode(TLV_RESET, OUTPUT);
  digitalWrite(TLV_RESET, LOW);
  delay(100);
  digitalWrite(TLV_RESET, HIGH);

  Serial.println("Init TLV DAC");
  if (!codec.begin()) {
    halt("Failed to initialize codec!");
  }
  delay(10);

  // Interface Control
  if (!codec.setCodecInterface(TLV320DAC3100_FORMAT_I2S,     // Format: I2S
                               TLV320DAC3100_DATA_LEN_16)) { // Length: 16 bits
    halt("Failed to configure codec interface!");
  }

  // Clock MUX and PLL settings
  if (!codec.setCodecClockInput(TLV320DAC3100_CODEC_CLKIN_PLL) ||
      !codec.setPLLClockInput(TLV320DAC3100_PLL_CLKIN_BCLK)) {
    halt("Failed to configure codec clocks!");
  }

  if (!codec.setPLLValues(1, 2, 32, 0)) { // P=2, R=2, J=32, D=0
    halt("Failed to configure PLL values!");
  }

  // DAC/ADC Config
  if (!codec.setNDAC(true, 8) || // Enable NDAC with value 8
      !codec.setMDAC(true, 2)) { // Enable MDAC with value 2
    halt("Failed to configure DAC dividers!");
  }

  if (!codec.powerPLL(true)) { // Power up the PLL
    halt("Failed to power up PLL!");
  }

  // DAC Setup
  if (!codec.setDACDataPath(true, true,                    // Power up both DACs
                            TLV320_DAC_PATH_NORMAL,        // Normal left path
                            TLV320_DAC_PATH_NORMAL,        // Normal right path
                            TLV320_VOLUME_STEP_1SAMPLE)) { // Step: 1 per sample
    halt("Failed to configure DAC data path!");
  }

  if (!codec.configureAnalogInputs(TLV320_DAC_ROUTE_MIXER, // Left DAC to mixer
                                   TLV320_DAC_ROUTE_MIXER, // Right DAC to mixer
                                   false, false, false,    // No AIN routing
                                   false)) {               // No HPL->HPR
    halt("Failed to configure DAC routing!");
  }

  // DAC Volume Control
  if (!codec.setDACVolumeControl(
          false, false, TLV320_VOL_INDEPENDENT) || // Unmute both channels
      !codec.setChannelVolume(false, 18) ||        // Left DAC +0dB
      !codec.setChannelVolume(true, 18)) {         // Right DAC +0dB
    halt("Failed to configure DAC volumes!");
  }

  // Headphone and Speaker Setup
  if (!codec.configureHeadphoneDriver(
          true, true,                     // Power up both drivers
          TLV320_HP_COMMON_1_35V,         // Default common mode
          false) ||                       // Don't power down on SCD
      !codec.configureHPL_PGA(0, true) || // Set HPL gain, unmute
      !codec.configureHPR_PGA(0, true) || // Set HPR gain, unmute
      !codec.setHPLVolume(true, 6) ||     // Enable and set HPL volume
      !codec.setHPRVolume(true, 6)) {     // Enable and set HPR volume
    halt("Failed to configure headphone outputs!");
  }

  if (!codec.enableSpeaker(true) ||                // Dis/Enable speaker amp
      !codec.configureSPK_PGA(TLV320_SPK_GAIN_6DB, // Set gain to 6dB
                              true) ||             // Unmute
      !codec.setSPKVolume(true, 0)) { // Enable and set volume to 0dB
    halt("Failed to configure speaker output!");
  }

  if (!codec.configMicBias(false, true, TLV320_MICBIAS_AVDD) ||
      !codec.setHeadsetDetect(true) ||
      !codec.setInt1Source(true, true, false, false, false,
                           false) || // GPIO1 is detect headset or button press
      !codec.setGPIO1Mode(TLV320_GPIO1_INT1)) {
    halt("Failed to configure headset detect");
  }
  Serial.println("TLV config done!");
}

void loop() {
  static tlv320_headset_status_t last_status = TLV320_HEADSET_NONE;

  tlv320_headset_status_t status = codec.getHeadsetStatus();

  if (last_status != status) {
    switch (status) {
    case TLV320_HEADSET_NONE:
      Serial.println("Headset removed");
      break;
    case TLV320_HEADSET_WITHOUT_MIC:
      Serial.println("Headphones detected");
      break;
    case TLV320_HEADSET_WITH_MIC:
      Serial.println("Headset with mic detected");
      break;
    }
    last_status = status;
  }

  // Read the sticky IRQ flags
  uint8_t flags = codec.readIRQflags(true);

  // Only print if there are flags set
  if (flags) {
    Serial.println(F("IRQ Flags detected:"));

    if (flags & TLV320DAC3100_IRQ_HPL_SHORT) {
      Serial.println(
          F("- Short circuit detected at HPL / left class-D driver"));
    }

    if (flags & TLV320DAC3100_IRQ_HPR_SHORT) {
      Serial.println(
          F("- Short circuit detected at HPR / right class-D driver"));
    }

    if (flags & TLV320DAC3100_IRQ_BUTTON_PRESS) {
      Serial.println(F("- Headset button pressed"));
    }

    if (flags & TLV320DAC3100_IRQ_HEADSET_DETECT) {
      Serial.println(F("- Headset insertion detected"));
    } else if (flags & 0x10) { // Check bit but with different meaning
      Serial.println(F("- Headset removal detected"));
    }

    if (flags & TLV320DAC3100_IRQ_LEFT_DRC) {
      Serial.println(F("- Left DAC signal power greater than DRC threshold"));
    }

    if (flags & TLV320DAC3100_IRQ_RIGHT_DRC) {
      Serial.println(F("- Right DAC signal power greater than DRC threshold"));
    }

    Serial.print(F("Raw flag value: 0x"));
    Serial.println(flags, HEX);
    Serial.println();
  }

  delay(100);
}