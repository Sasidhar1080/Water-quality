#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 27 // temp pin
#define tdssensorPin 35 // tds pin

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

int tdssensorValue = 0;
float tdsValue = 0;
float Voltage = 0;
float Temp = 0;

void setup() {
  Serial.begin(115200);
  tempSensor.begin();
}

void loop() {
  // TDS calibration
  tdssensorValue = analogRead(tdssensorPin);
  Voltage = tdssensorValue * 3.3 / 1024.0; // Convert analog reading to Voltage
  Serial.print("Voltage: ");
  Serial.println(Voltage);

  // Temperature reading
  Temp = tempSensor.requestTemperaturesByIndex(0);
  Serial.print("Temperature: ");
  Serial.print(Temp);
  Serial.println(" °C");

  // Temperature compensation
  float compensationCoefficient = 1.0 + 0.02 * (Temp - 25.0);
  float compensationVoltage = Voltage / compensationCoefficient;

  // TDS with Temperature Compensation
  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
    - 255.86 * compensationVoltage * compensationVoltage
    + 857.39 * compensationVoltage) * 0.5;

  Serial.print("TDS Value (with Temp Compensation) = ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  // TDS without Temperature Compensation
  float compensationCoefficient_without_temp = 1.0 + 0.02 * (25.0 - 25.0);
  float compensationVoltage_without_temp = Voltage / compensationCoefficient_without_temp;
  float tdsValue_without_temp = (133.42 * compensationVoltage_without_temp * compensationVoltage_without_temp
    - 255.86 * compensationVoltage_without_temp * compensationVoltage_without_temp
    + 857.39 * compensationVoltage_without_temp) * 0.5;

  Serial.print("TDS Value (without Temp Compensation) = ");
  Serial.print(tdsValue_without_temp);
  Serial.println(" ppm");
  
  delay(1000);
}
