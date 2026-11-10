#include <driver/i2s.h>
#include <math.h>

// --- Pins ---
const int trigPin = 25;
const int echoPin = 26;

#define I2S_BCK_PIN 27
#define I2S_DIN_PIN 22
#define I2S_LCK_PIN 23

// --- Sensor Variablen ---
float smoothedDistance = 0;
float alpha = 0.2; // Glättungsfaktor

// --- Audio Variablen ---
const int sampleRate = 44100;
volatile float filterFreq = 10000.0; // Wird vom Sensor gesteuert
const float sawFreq = 200.0;         // Feste Oszillator-Frequenz
float phase = 0.0;
float filterState = 0.0;

TaskHandle_t SensorTask;

void setup() {
  Serial.begin(115200);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // I2S Konfiguration
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

  // Starte den Sensor auf Kern 0
  xTaskCreatePinnedToCore(
    readSensor,   // Funktion
    "SensorTask", // Name
    10000,        // Stack
    NULL,         // Parameter
    1,            // Priorität
    &SensorTask,  // Handle
    0             // Kern (0)
  );
}

// ================================================================
// KERN 0: Kümmert sich nur um den Ultraschallsensor
// ================================================================
void readSensor(void * parameter) {
  for(;;) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, 30000); 
    
    if (duration > 0) {
      float currentDistance = duration * 0.034 / 2;
      
      if(smoothedDistance == 0) {
        smoothedDistance = currentDistance;
      } else {
        // Formel korrigiert
        smoothedDistance = (alpha * currentDistance) + ((1.0 - alpha) * smoothedDistance);
      }
      
      // Fehlerwerte filtern
      if(smoothedDistance > 5 && smoothedDistance < 50) {
        // Mapping: 5cm -> 200 Hz (geschlossen), 50cm -> 10000 Hz (offen)
        // Das ersetzt dein SuperCollider linexp()
        float norm = (smoothedDistance - 5.0) / (50.0 - 5.0); // 0.0 bis 1.0
        filterFreq = 200.0 + (norm * (10000.0 - 200.0));
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // 50ms Pause
  }
}

// ================================================================
// KERN 1: Generiert das Audio (Sawtooth + LowPass Filter)
// ================================================================
void loop() {
  size_t bytes_written;
  int16_t sample_buffer[64 * 2]; // Stereo Buffer

  // Füll den Audio-Buffer
  for(int i = 0; i < 64; i++) {
    
    // 1. Sawtooth Oszillator erzeugen (Werte von -1.0 bis 1.0)
    phase += sawFreq / sampleRate;
    if(phase > 1.0) phase -= 2.0; 
    float sig = phase; 

    // 2. Low Pass Filter (One-Pole IIR Filter)
    float filterAlpha = (2.0 * PI * filterFreq) / sampleRate;
    if(filterAlpha > 1.0) filterAlpha = 1.0;
    
    filterState = filterState + filterAlpha * (sig - filterState);
    sig = filterState;

    // 3. Float in 16-Bit Integer wandeln und Lautstärke anpassen
    int16_t audio_out = (int16_t)(sig * 16000.0); // 16000 verhindert Übersteuerung

    // Links und Rechts
    sample_buffer[i * 2] = audio_out;     
    sample_buffer[i * 2 + 1] = audio_out; 
  }

  // Sende den fertigen Buffer an den DAC
  i2s_write(I2S_NUM_0, &sample_buffer, sizeof(sample_buffer), &bytes_written, portMAX_DELAY);
}