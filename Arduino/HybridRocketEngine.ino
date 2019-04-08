#include <SD.h>
#include <SPI.h>

boolean launch = LOW;
boolean record = LOW;
boolean control = LOW;
int percent = 0; // the percent valve will open

File myFile;
String fileName = "data.csv";
float calibration = 22.189; // calibration constant

// all times are in ms
unsigned long logFrequency = 25;
unsigned long streamFrequency = 500;
unsigned long ignitionDelay = 500;
unsigned long fullThrottleTime = 5000 + ignitionDelay;
unsigned long burnTime; // will be set by user input during startup

int minTime = 80; // DO NOT SET BELOW 80. Minimum valve cycle time

int ignitionPin = 2;
int valvePin = 3;
int pressurePin = A4;
int loadPin = A5;
int pinCS = 10; // chip select for the SD card

// Class for the ignition
class Ignition {
  
  unsigned long prevTime = 0;
  unsigned long startTime = 0;

  // Constructor
  public:
  Ignition() {
    pinMode(ignitionPin, OUTPUT);     
  }

  void Update() {
    unsigned long curTime = millis();
    if ((launch) && (curTime - startTime > ignitionDelay)) {
      digitalWrite(ignitionPin, HIGH);
    }
  }

  void Initialize() {
    unsigned long curTime = millis();
    startTime = curTime;
  }

};

// Class for the pressure transducer and load cell
class Sensor {
  
  unsigned long prevLogTime = 0;
  unsigned long prevStreamTime = 0;
  unsigned long startTime = 0;
  float offset; // calibration offset;

  // Constructor
  public:
  Sensor() {
    pinMode(pressurePin, INPUT);
    pinMode(loadPin, INPUT);
  }

  void Update() {
      long curTime = millis() - startTime;
      
      if ((record) && (curTime - prevLogTime >= logFrequency)) {
        int presValue = analogRead(pressurePin);
        float presVoltage = (presValue/1023.0)*5;
        float psi = (presVoltage-0.5)*500/4;  
        
        float loadValue = analogRead(loadPin);
        float loadVoltage = (loadValue/1023.0)*5;
        float weight = loadVoltage*calibration - offset; 
        
        prevLogTime += logFrequency;

        String data = String(curTime) + "," + String(psi) + "," + String(weight);
        data += ","  + String(launch) + "," + String(percent) + ",";
        myFile = SD.open(fileName, FILE_WRITE);
        myFile.println(data);
        myFile.close();
      }

      if ((record) && (curTime - prevStreamTime >= streamFrequency)) {
        int presValue = analogRead(pressurePin);
        float presVoltage = (presValue/1023.0)*5;
        float psi = (presVoltage-0.5)*500/4;  
        
        float loadValue = analogRead(loadPin);
        float loadVoltage = (loadValue/1023.0)*5;
        float weight = loadVoltage*calibration - offset; 
        
        prevStreamTime += streamFrequency;

        String data = "Time: " + String(curTime) + "     Pressure: " + String(psi);
        data += "     Load: " + String(weight) + "     launch: " + String(launch);
        data += "     Percent: " + String(percent);
        Serial.println(data);
      }
    }

    void Calibrate() {
        for (int i = 1; i <= 25; i++) {
          float loadValue = analogRead(loadPin);
          float loadVoltage = (loadValue/1023.0)*5;
          float weight = loadVoltage*calibration;
          offset += weight;
          delay(20); 
        }
        offset /= 25;
        Serial.print("Offset calculated to be: ");
        Serial.println(offset);
    }

    void Initialize() {
      unsigned long curTime = millis();
      startTime = curTime;
    }
 
};


// Class for valve
class Valve {
  
  boolean state = LOW;
  unsigned long startTime = 0;
  unsigned long prevTime = 0;

  // Constructor
  public:
  Valve() {
    pinMode(valvePin, OUTPUT);
  }

  void Update() {
    unsigned long curTime = millis();
    
    // full throttle burn
    if ((!control) && (launch) && (curTime - startTime < burnTime)) {
      digitalWrite(valvePin, HIGH);
    }
    // controlled burn, before controlled segment
    else if ((control) && (launch) && (curTime - startTime < fullThrottleTime)) {
      digitalWrite(valvePin, HIGH);
    }
    // controlled burn, during controlled segment
    else if ((control) && (launch) && (curTime - startTime < burnTime)) {
      int offTime;
      int onTime;
      if (percent >= 50) {
        offTime = minTime;
        onTime = percent*minTime/(100-percent); 
      }
      else {
        onTime = minTime;
        offTime = (100-percent)*minTime/percent; 
      }
      if ((state == LOW) && (curTime - prevTime >= offTime)) {
        state = HIGH;
        digitalWrite(valvePin, HIGH);
        prevTime = curTime;
      }
      else if ((state == HIGH) && (curTime - prevTime >= onTime)) {
        state = LOW;
        digitalWrite(valvePin, LOW);
        prevTime = curTime;
      }
    }
    else {
      digitalWrite(valvePin, LOW);
      launch = LOW;
    }
  }
    
  void Initialize() {
    unsigned long curTime = millis();
    startTime = curTime;
  }

};


// Create instances of the classes defined above
Ignition ignition;
Sensor sensors;
Valve valve; 

void setup()   { 
  Serial.begin(9600);
  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);
  SD.begin();

  if (!SD.begin(pinCS)) {
    Serial.println("Card failed, or not present");
  }

  myFile = SD.open(fileName, FILE_WRITE);
  myFile.print("TIME (mS),"); 
  myFile.print("PRESSURE (PSI),");
  myFile.print("LOAD (LBS),");
  myFile.print("STATUS,");
  myFile.println("PERCENT");
  myFile.close();
  
  Serial.println("Welcome to Kevin");
  sensors.Calibrate();
  Serial.println();
  
  // read in total burn time
  Serial.println("Enter the time in mS for the burn...");
  while(1) {
    if (Serial.available()) {
      long command = Serial.parseInt();
      burnTime = ignitionDelay + command;
      Serial.print("Burn time set to: ");
      Serial.println(command);
      Serial.println();
      break;
    }
  }

  // determine what type of burn is requested
  Serial.println(F("Enter 1 for a full throttle burn, or 2 for a controlled burn"));
  Serial.println(F("Controlled burns will start after 5 seconds of full throttle..."));
  Serial.println();
  while(1) {
    if (Serial.available()) {
      int command = Serial.parseInt();
      if (command == 1) {
        Serial.println("You have selected a full throttle burn");
        Serial.println();
        break;
      }
      else if (command == 2) {
         Serial.println("You have selected a throttled burn");
         Serial.println("Enter the control percentage...");
         while(1) {
          if (Serial.available()) {
            percent = Serial.parseInt();
            if (percent != 0) { //if the percent has been updated
              Serial.print("Percent set to: ");
              Serial.println(percent);
              Serial.println(); 
              break;
            }
          }
        }
        control = HIGH;
        break;
      }
    }
  }
  
  Serial.println(F("Commands you should know:"));
  Serial.println(F("    Enter 1 to record data."));
  Serial.println(F("    Enter 2 to end recording."));
  Serial.println(F("    Enter 3 to launch."));
  Serial.println(F("    Enter 4 to ABORT."));
  Serial.println();
}
  
void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    switch (command) {
      case '1':
        record = HIGH;
        sensors.Initialize();
        Serial.println("Recording has begun");
        Serial.println();
        break;
      case '2':
        record = LOW;
        Serial.println("Recording has ended");
        Serial.println();
        break;
      case '3':
        launch = HIGH;
        valve.Initialize();
        ignition.Initialize();
        Serial.println("LAUNCH!");
        Serial.println();
        break;
      case '4':
        launch = LOW;
        Serial.println("ABORT!");
        Serial.println();
        break;
    }
  }
  ignition.Update();
  valve.Update();
  sensors.Update();
}
