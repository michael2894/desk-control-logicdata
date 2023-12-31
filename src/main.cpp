#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


#define RxD 10
#define TxD 11
#define HS1 2 // UP
#define HS2 3 // DOWN
#define HS3 4
#define HS4 5
#define lowerMaximum 65
#define upperMaximum 130
#define ledPin 13


// Define the virtual serial port characteristics
const int baudRate = 1000;
const int dataBits = 8;
const int stopBits = 1;
const int inverted = true; // Set this to true if the bits are inverted
const int bufferSize = 3;
const int lowerMaximumAddress = 0x00; 
const int upperMaximumAddress = 0x02; 


// Define timing variables
const long interval = 300; // ms, interval in which the height change should be checked
const int startupDelay = 2000; // amount of time which should be waited after begin driving the desk

volatile bool buttonStateUp = 0;
volatile bool buttonStateDown = 0;
bool hasWaited = false;
volatile int drivingDirection = 0; // 0=standstill, 1=down, 2=up
unsigned long previousMillis = 0;
unsigned long startTime = 0;
unsigned long timestamp = 0;

int circularBuffer[bufferSize];
int deskHeight = 90;
int previousHeight = 0;
int upperLimit = 0;
int lowerLimit = 0;

void edgeChangeUp();
void edgeChangeDown();
// void stopDriving();
void stopDriving();
void driveDown();
void driveUp();
void reprogramLimit();
SoftwareSerial mySerial(RxD, TxD, inverted); // RX, TX

void setup()
{
  pinMode(RxD, INPUT_PULLUP);// Start the virtual serial port
  pinMode(HS1, INPUT);
  pinMode(HS2, INPUT);
  pinMode(ledPin, OUTPUT);
  mySerial.begin(baudRate);
  Serial.begin(115200); // Start the hardware serial port for debugging

  EEPROM.get(upperMaximumAddress, upperLimit);
  EEPROM.get(lowerMaximumAddress, lowerLimit);
  Serial.print("Upper limit: ");
  Serial.println(upperLimit);
  Serial.print("Lower limit: ");
  Serial.println(lowerLimit);
  if(upperLimit>upperMaximum||upperLimit<lowerLimit||lowerLimit<lowerMaximum||lowerLimit==upperLimit) // validity check 
  {
    upperLimit = 115;
    lowerLimit = 78;
    EEPROM.put(lowerMaximumAddress, lowerLimit);
    EEPROM.put(upperMaximumAddress, upperLimit);
    Serial.println("Data written to EEPROM: ");
  }
  // pinMode(HS1, OUTPUT);
  // digitalWrite(HS1, HIGH);
  // delay(50);
  // digitalWrite(HS1, LOW);
  // pinMode(HS1, INPUT);
  attachInterrupt(digitalPinToInterrupt(HS1), edgeChangeUp, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HS2), edgeChangeDown, CHANGE);
  Serial.println("Boot complete!");
}

void loop()
{
  if (mySerial.available() > 0) // Read data from the virtual serial port
  {
    for (int i = 0; i < bufferSize - 1; i++) // Left shift buffer
    {
      circularBuffer[i] = circularBuffer[i + 1];
    }
    circularBuffer[bufferSize - 1] = mySerial.read();
    if (circularBuffer[0] == 1 && circularBuffer[1] == 0)
    {
      if (circularBuffer[2] >= lowerMaximum && circularBuffer[2] <= upperMaximum)
      {
        deskHeight = circularBuffer[2];
      }
      Serial.print(millis());
      Serial.print(": ");
      Serial.println(circularBuffer[2]);
    }
  }

  if (deskHeight >= (upperLimit - 1) && drivingDirection == 2) // upper height limit
  {
    stopDriving();
  }
  if (deskHeight <= (lowerLimit + 1) && drivingDirection == 1) // lower height limit
  {
    stopDriving();
  }

  if (drivingDirection != 0)
  {
    if (!hasWaited)
    {
      Serial.print(millis());
      Serial.print(": ");
      Serial.println("Waiting!");
      startTime = millis(); // Initialize the start time when waiting begins
      hasWaited = true;     // Set the flag to indicate that waiting has occurred
      drivingDirection == 1 ? previousHeight=upperMaximum : previousHeight=0;
      // if (drivingDirection == 1){
      //     previousHeight=upperMaximum;
      // }

    }
    if (millis() - startTime >= startupDelay) // wait after starting driving, as height info takes time to arrive and motor has slow start
    { 
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval)
      {
      int heightChange = deskHeight - previousHeight; // Calculate height change during the interval
      Serial.print(millis());
      Serial.print(": HC: ");
      Serial.println(heightChange);
        previousMillis = currentMillis;                      // Update the previousMillis for the next interval
        previousHeight = deskHeight;                         // Update the previousSpeed for the next interval
        if ((heightChange >= 0 && drivingDirection == 1)||(heightChange <= 0 && drivingDirection == 2)) // 0=standstill, 1=down, 2=up
        {
            digitalWrite(ledPin, HIGH);
            Serial.print("Height Change ");
            Serial.print(((drivingDirection==2) ? "UP" : "DOWN"));
            Serial.println(" triggered!");
            stopDriving();
          }
          // else if (heightChange <= 0 && drivingDirection == 2)
          // {
          //   digitalWrite(ledPin, HIGH);
          //   Serial.println("Height Change Up triggered!");
          //   stopDriving();
          // }
        }
      }
    // }
  }
}

void edgeChangeUp()
{
  // Serial.print(millis());
  // Serial.print(": ");
  // Serial.print("INT up triggered!: ");
  int currentState = digitalRead(HS1); // Read the current state of the pin
  // Serial.println(currentState);
  if (currentState == HIGH && buttonStateUp == 0)
  {
    buttonStateUp = 1;
    digitalWrite(ledPin, LOW);
    if (buttonStateDown == 1)
    {
      reprogramLimit();
      buttonStateUp = 0;
    }
  }
  else if (currentState == LOW && buttonStateUp == 1 && ((millis() - timestamp) < 500))
  {
    buttonStateUp = 0;
    Serial.print(millis());
    Serial.print(": ");
    Serial.println("Button UP doubleclicked!");
    driveUp();
  }
  else if (currentState == LOW && buttonStateUp == 1)
  {
    buttonStateUp = 0;
    timestamp = millis();
  }
  delay(10);
}

void edgeChangeDown()
{  
  // Serial.print("INT down triggered!: ");
  int currentState = digitalRead(HS2); // Read the current state of the pin
  // Serial.println(currentState);
  if (currentState == HIGH && buttonStateDown == 0)
  {
    buttonStateDown = 1;
    digitalWrite(ledPin, LOW);
    if (buttonStateUp == 1)
    {
      reprogramLimit();
      buttonStateDown = 0;
    }
  }
  else if (currentState == LOW && buttonStateDown == 1 && ((millis() - timestamp) < 500))
  {
    buttonStateDown = 0;
    Serial.print(millis());
    Serial.print(": ");
    Serial.println("Button DOWN doubleclicked!");
    driveDown();
  }
  else if (currentState == LOW && buttonStateDown == 1)
  {
    buttonStateDown = 0;
    timestamp = millis();
  }
  delay(10);
}

void stopDriving()
{
  pinMode(HS1, OUTPUT);
  pinMode(HS2, OUTPUT);
  digitalWrite(HS1, LOW);
  digitalWrite(HS2, LOW);
  Serial.print(millis());
  Serial.print(": ");
  Serial.print("Driving ");
  Serial.print(((drivingDirection==2) ? "UP" : "DOWN"));
  Serial.println(" stopped!");
  pinMode(HS1, INPUT);
  pinMode(HS2, INPUT);
  // delay(100);
  attachInterrupt(digitalPinToInterrupt(HS1), edgeChangeUp, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HS2), edgeChangeDown, CHANGE);
  hasWaited = false;
  drivingDirection = 0; // 0=standstill, 1=down, 2=up
}
// void stopDriving()
// {
//   // pinMode(HS1, OUTPUT);
//   // pinMode(HS2, OUTPUT);
//   // digitalWrite(HS1, LOW);
//   // digitalWrite(HS2, LOW);
//   // Serial.println("Driving UP stopped!");
//   // pinMode(HS1, INPUT);
//   // pinMode(HS2, INPUT);
//   // attachInterrupt(digitalPinToInterrupt(HS1), edgeChangeUp, CHANGE);
//   // attachInterrupt(digitalPinToInterrupt(HS2), edgeChangeDown, CHANGE);
//   // hasWaited = false;
//   // drivingDirection = 0; // 0=standstill, 1=down, 2=up
//   // doubleClickUpDetected = false;
//   // doubleClickDownDetected = false;
// }
void driveUp()
{
    drivingDirection = 2;
    detachInterrupt(digitalPinToInterrupt(HS1));
    detachInterrupt(digitalPinToInterrupt(HS2));
    Serial.print(millis());
    Serial.print(": ");
    Serial.println("Driving Up!");
    pinMode(HS1, OUTPUT);
    digitalWrite(HS1, HIGH);
}
void driveDown()
{
    drivingDirection = 1; // 0=standstill, 1=down, 2=up
    detachInterrupt(digitalPinToInterrupt(HS1));
    detachInterrupt(digitalPinToInterrupt(HS2));
    Serial.print(millis());
    Serial.print(": ");
    Serial.println("Driving Down!");
    pinMode(HS2, OUTPUT);
    digitalWrite(HS2, HIGH);
}
void reprogramLimit()
{
  if (deskHeight>=((upperLimit+lowerLimit)/2))
  {
    upperLimit=deskHeight;
    EEPROM.put(upperMaximumAddress, upperLimit);
    Serial.print(millis());
    Serial.print(": ");
    Serial.print("New upper limit: ");
    Serial.println(upperLimit);
  }
  else if(deskHeight<((upperLimit+lowerLimit)/2))
  {
    lowerLimit=deskHeight;
    EEPROM.put(lowerMaximumAddress, lowerLimit);
    Serial.print(millis());
    Serial.print(": ");
    Serial.print("New lower limit: ");
    Serial.println(lowerLimit);
  }
}