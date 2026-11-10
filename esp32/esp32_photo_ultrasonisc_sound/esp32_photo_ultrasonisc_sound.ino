#include <driver/i2s_std.h>
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
float alpha = 0.2; // Smoothing factor

// --- Audio Synthesizer Variables ---
const int sampleRate = 44100;
float phase = 0.0;
float filterState = 0.0;

// New I2S Channel Handle for Core 3.x
i2s_chan_handle_t tx_chan;

TaskHandle_t SensorTask;

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);

  // --- NEW I2S Configuration for ESP32 Core 3.x ---
  i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL);

  i2s_std_config_t tx_std_cfg = {
      .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = (gpio_num_t)I2S_BCK_PIN,
          .ws   = (gpio_num_t)I2S_LCK_PIN,
          .dout = (gpio_num_t)I2S_DIN_PIN,
          .din  = I2S_GPIO_UNUSED,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv   = false,
          },
      },
  };
  
  i2s_channel_init_std_mode(tx_chan, &tx_std_cfg);
  i2s_channel_enable(tx_chan);

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
    
    long duration = pulseIn(echoPin, HIGH, 30000); 
    
    if (duration > 0) {
      float currentDist = duration * 0.034 / 2;
      smoothedDistance = (smoothedDistance == 0) ? currentDist : (alpha * currentDist) + ((1.0 - alpha) * smoothedDistance);
    }

    // 2. Read Light Dependent Resistor 
    float rawLdr = analogRead(ldrPin);
    smoothedLdr = (smoothedLdr == 0) ? rawLdr : (alpha * rawLdr) + ((1.0 - alpha) * smoothedLdr);

    sharedDistance = smoothedDistance;
    sharedLdr = smoothedLdr;

    // 3. Send Data via USB to Laptop 
    Serial.print(smoothedDistance, 1); 
    Serial.print(",");
    Serial.println(smoothedLdr, 0);    

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
  float normDist = constrain((sharedDistance - 5.0) / 45.0, 0.0, 1.0);
  float currentSawFreq = 100.0 + (normDist * 900.0);

  float normLdr = constrain((sharedLdr - 200.0) / 3600.0, 0.0, 1.0);
  float currentFilterFreq = 200.0 + (normLdr * 8800.0);

  // --- Audio Rendering ---
  for(int i = 0; i < 64; i++) {
    phase += currentSawFreq / sampleRate;
    if(phase > 1.0) phase -= 2.0; 
    float sig = phase; 

    float filterAlpha = (2.0 * M_PI * currentFilterFreq) / sampleRate;
    if(filterAlpha > 1.0) filterAlpha = 1.0;
    filterState = filterState + filterAlpha * (sig - filterState);
    sig = filterState;

    int16_t audio_out = (int16_t)(sig * 14000.0);
    
    sample_buffer[i * 2]     = audio_out;     
    sample_buffer[i * 2 + 1] = audio_out; 
  }

  // Send the audio buffer to the PCM5102A DAC using the new V3 API
  i2s_channel_write(tx_chan, sample_buffer, sizeof(sample_buffer), &bytes_written, portMAX_DELAY);
}