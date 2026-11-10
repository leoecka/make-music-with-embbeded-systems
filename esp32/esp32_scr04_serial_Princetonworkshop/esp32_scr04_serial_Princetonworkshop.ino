const int trigPin = 25;
const int echoPin = 26;

float smoothedDistance = 0;
float alpha = 0.2; // Glättungsfaktor

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Timeout nach 30.000 Mikrosekunden (ca. 5 Meter max)
  long duration = pulseIn(echoPin, HIGH, 30000); 
  
  // Nur rechnen und senden, wenn das Signal NICHT 0 ist (also ein echtes Echo ankam)
  if (duration > 0) {
    float currentDistance = duration * 0.034 / 2;
    
    // Wenn es die erste Messung ist, setze den Startwert
    if(smoothedDistance == 0) {
      smoothedDistance = currentDistance;
    } else {
      // Hier ist die reparierte Glättungsformel:
      smoothedDistance = (alpha * currentDistance) + ((1.0 - alpha) * smoothedDistance);
    }
    
    Serial.println(smoothedDistance);
  }
  
  delay(50); 
}