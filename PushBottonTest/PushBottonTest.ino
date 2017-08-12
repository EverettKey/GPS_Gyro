const int ledPin = 13;
const int buttonPin = 3;
int logState = LOW;
int buttonState = LOW;
int lastButtonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lightState = LOW;
#define SerialMonitor Serial
void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  SerialMonitor.begin(9600);
}

void loop() {
  int buttonReading = digitalRead(buttonPin);
  if(buttonReading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if(debounceDelay + lastDebounceTime < millis()){
    if (buttonReading != buttonState) {
      buttonState = buttonReading;
      if(buttonState == HIGH){
        logState = !logState;
      }
    }
  }
  digitalWrite(ledPin, logState);
  Serial.print(logState);
  lastButtonState = buttonReading;
}
