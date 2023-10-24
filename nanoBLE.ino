#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <ArduinoBLE.h>

MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;

// BLE Heart Rate Service and Characteristic
BLEService heartRateService("180D");
BLEUnsignedCharCharacteristic heartRateChar("2A37", BLERead | BLENotify);

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }

  // Initialize BLE
  if (!BLE.begin()) 
  {
    Serial.println("Starting BLE failed!");
    while(1);
  }
  BLE.setLocalName("HeartRateSensor");
  BLE.setAdvertisedService(heartRateService);
  heartRateService.addCharacteristic(heartRateChar);
  BLE.addService(heartRateService);
  heartRateChar.writeValue(0);  // Initial heart rate value
  BLE.advertise();
  
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop()
{
  BLEDevice central = BLE.central();  // Wait for a BLE central to connect

  // If device is connected, read the sensor and send data
  if (central) 
  {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) 
    {
      long irValue = particleSensor.getIR();
      if (checkForBeat(irValue) == true)
      {
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20)
        {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;

          beatAvg = 0;
          for (byte x = 0; x < RATE_SIZE; x++)
            beatAvg += rates[x];
          beatAvg /= RATE_SIZE;

          heartRateChar.writeValue(beatAvg);  // Send heart rate over BLE
        }
      }

      Serial.print("IR=");
      Serial.print(irValue);
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM=");
      Serial.println(beatAvg);
      delay(2000);  // Update every 2 seconds
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}
