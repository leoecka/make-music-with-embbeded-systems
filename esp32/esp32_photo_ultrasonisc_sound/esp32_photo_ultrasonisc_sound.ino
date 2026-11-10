#include <driver/i2s.h>
#include <math.h>

// --- Pin Definitions ---
const int trigPin = 25;
const int echoPin = 26;
const int ldrPin  = 34; 

#define I2S_BCK_PIN 27
#define I2S_DIN_PIN 22
#define I2S_LCK_PIN 23

// --- Global Sensor Variables (Shared between CPU Cores) ---
volatile float sharedDistance = 20.0;
volatile float sharedLdr = 2000.0;

float smoothedDistance = 0;
float smoothedLdr = 0;
float alpha = 0.2; // Smoothing factor (0.1 = slow/smooth, 0.9 = fast/jittery)

// --- Audio Synthesizer Variables ---
const int sampleRate = 44100;
float phase = 0.0;
float filterState = 0.0;

TaskHandle_t SensorTask;

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);

  // --- I2S Hardware Configuration ---
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = sampleRate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LCK_PIN,
    .data_out_num = I2S_DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  // --- Start Sensor Task on CPU Core 0 ---
  xTaskCreatePinnedToCore(
    readSensors, "SensorTask", 10000, NULL, 1, &SensorTask, 0
  );
}

// ================================================================
// CORE 0: Read Sensors & Send Serial Data to SuperCollider
// ================================================================
void readSensors(void * parameter) {
  for(;;) {
    // 1. Trigger Ultrasonic Sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Timeout after 30ms (approx. 5 meters max distance)
    long duration = pulseIn(echoPin, HIGH, 30000); 
    
    if (duration > 0) {
      float currentDist = duration * 0.034 / 2;
      // Apply Exponential Moving Average filter
      smoothedDistance = (smoothedDistance == 0) ? currentDist : (alpha * currentDist) + ((1.0 - alpha) * smoothedDistance);
    }

    // 2. Read Light Dependent Resistor (Values: 0 to 4095)
    float rawLdr = analogRead(ldrPin);
    smoothedLdr = (smoothedLdr == 0) ? rawLdr : (alpha * rawLdr) + ((1.0 - alpha) * smoothedLdr);

    // 3. Update global variables for the audio core
    sharedDistance = smoothedDistance;
    sharedLdr = smoothedLdr;

    // 4. Send Data via USB to Laptop (Format: "Distance,LDR")
    Serial.print(smoothedDistance, 1); // 1 decimal place
    Serial.print(",");
    Serial.println(smoothedLdr, 0);    // 0 decimal places

    // Wait ~30ms before next measurement to prevent sensor echo overlap
    vTaskDelay(30 / portTICK_PERIOD_MS); 
  }
}

// ================================================================
// CORE 1: Internal Audio Synthesizer (Outputs to I2S DAC)
// ================================================================
void loop() {
  size_t bytes_written;
  int16_t sample_buffer[64 * 2]; // Stereo Buffer

  // --- Parameter Mapping ---
  // Distance (5-50cm) -> Controls Sawtooth Pitch (100 Hz - 1000 Hz)
  float normDist = constrain((sharedDistance - 5.0) / 45.0, 0.0, 1.0);
  float currentSawFreq = 100.0 + (normDist * 900.0);

  // LDR (200-3800) -> Controls Low-Pass Filter Cutoff (200 Hz - 9000 Hz)
  float normLdr = constrain((sharedLdr - 200.0) / 3600.0, 0.0, 1.0);
  float currentFilterFreq = 200.0 + (normLdr * 8800.0);

  // --- Audio Rendering ---
  for(int i = 0; i < 64; i++) {
    
    // Generate Sawtooth wave (-1.0 to 1.0)
    phase += currentSawFreq / sampleRate;
    if(phase > 1.0) phase -= 2.0; 
    float sig = phase; 

    // Apply One-Pole Low-Pass Filter
    float filterAlpha = (2.0 * M_PI * currentFilterFreq) / sampleRate;
    if(filterAlpha > 1.0) filterAlpha = 1.0;
    filterState = filterState + filterAlpha * (sig - filterState);
    sig = filterState;

    // Convert float (-1.0 to 1.0) to 16-bit integer and scale volume
    int16_t audio_out = (int16_t)(sig * 14000.0);
    
    // Write to Left and Right channel
    sample_buffer[i * 2]     = audio_out;     
    sample_buffer[i * 2 + 1] = audio_out; 
  }

  // Send the audio buffer to the PCM5102A DAC
  i2s_write(I2S_NUM_0, &sample_buffer, sizeof(sample_buffer), &bytes_written, portMAX_DELAY);
}