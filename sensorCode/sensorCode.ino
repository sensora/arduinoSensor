/* BMP085 Extended Example Code
  by: Jim Lindblom
  SparkFun Electronics
  date: 1/18/11
  updated: 2/26/13
  license: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/
  
  Get pressure and temperature from the BMP085 and calculate 
  altitude. Serial.print it out at 9600 baud to serial monitor.

  Update (7/19/11): I've heard folks may be encountering issues
  with this code, who're running an Arduino at 8MHz. If you're 
  using an Arduino Pro 3.3V/8MHz, or the like, you may need to 
  increase some of the delays in the bmp085ReadUP and 
  bmp085ReadUT functions.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
SSD1306 display(OLED_RESET);

int totalSent = 0;

#define BMP085_ADDRESS 0x77  // I2C address of BMP085

const unsigned char OSS = 0;  // Oversampling Setting

// Calibration values
int ac1;
int ac2; 
int ac3; 
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1; 
int b2;
int mb;
int mc;
int md;

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 

short temperature;
long pressure;

// Use these for altitude conversions
const float p0 = 101325;     // Pressure at sea level (Pa)
float altitude;


#include <dht11.h>
dht11 DHT11;
/*Pin setup*/
int lightSensor = A0;
//int soundSensor = A1;
#define DHT11PIN A1
//int vibrationSensor = A3;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  bmp085Calibration();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); 
}


void loop() {
  String fullPrint = "";
  /*Light sensor*/
  int lightValue = analogRead(lightSensor);
  String lightString = "Light:";
  lightString += lightValue;
  lightString += "|";
  fullPrint.concat(lightString);
  
  /*Sound sensor
  int soundValue = analogRead(soundSensor);
  String soundString = "Sound:";
  soundString += soundValue;
  soundString += "|";
  fullPrint.concat(soundString);
  */
  
  /* Temperature and humidity */
  int chk = DHT11.read(DHT11PIN);
  String temperatureString = "Temp:";
  char temp[10];
  temperatureString += dtostrf((float)DHT11.temperature, 2, 1, temp);
  temperatureString += "|";
  fullPrint.concat(temperatureString);
  String humidityString = "Hum:";
  char hum[10];
  humidityString += dtostrf((float)DHT11.humidity, 2, 1, hum);
  humidityString += "|";
  fullPrint.concat(humidityString);
  
  /* Vibration 
  int vibrationValue = analogRead(vibrationSensor);
  String vibrationString = "Vibration:";
  vibrationString += vibrationValue;
  vibrationString += "|";
  fullPrint.concat(vibrationString);*/
  
  /* Pressure */
  temperature = bmp085GetTemperature(bmp085ReadUT());
  String temperature2 = "Temperature2:";
  temperature2 += temperature, DEC;
  temperature2 += "|";
  fullPrint.concat(temperature2);
  pressure = bmp085GetPressure(bmp085ReadUP());
  String pressureString = "Pressure:";
  pressureString += pressure, DEC;
  pressureString += "|";
  fullPrint.concat(pressureString);
  char alt[10];
  altitude = (float)44330 * (1 - pow(((float) pressure/p0), 0.190295));
  String altitudeString = "Altitude:";
  altitudeString += dtostrf(altitude, 2, 1, alt);
  altitudeString += "|";
  fullPrint.concat(altitudeString);
  
  Serial.println(fullPrint);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  display.setCursor(0,0);
  String temperatureDisplay = "Temperature: ";
  temperatureDisplay.concat(temp);
  display.println(temperatureDisplay);
  
  display.setCursor(0,10);
  String moistureDisplay = "Moisture: ";
  moistureDisplay.concat(hum);
  display.println(moistureDisplay);
  
  display.setCursor(0,20);
  String lightDisplay = "Light: ";
  lightDisplay.concat(lightValue);
  display.println(lightDisplay);
  
  display.setCursor(0,30);
  String pressureDisplay = "Pressure: ";
  pressureDisplay.concat(pressure);
  display.println(pressureDisplay);
  
  display.setCursor(0,40);
  String temperatureDisplay2 = "Temperature 2: ";
  temperatureDisplay2 += temperature, DEC;
  display.println(temperatureDisplay2);
  
  display.setCursor(0,50);
  String altitudeDisplay = "Altitude: ";
  altitudeDisplay.concat(alt);
  display.println(altitudeDisplay);
  
  totalSent ++;
  display.setCursor(85, 10);
   display.setTextSize(2);
  display.println(totalSent);
  
  
  display.display();
  
  delay(30000);
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1 = bmp085ReadInt(0xB6);
  b2 = bmp085ReadInt(0xB8);
  mb = bmp085ReadInt(0xBA);
  mc = bmp085ReadInt(0xBC);
  md = bmp085ReadInt(0xBE);
}

// Calculate temperature given ut.
// Value returned will be in units of 0.1 deg C
short bmp085GetTemperature(unsigned int ut)
{
  long x1, x2;
  
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return ((b5 + 8)>>4);  
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;
  
  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;
  
  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
  
  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
    
  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  
  return p;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;
  
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available())
    ;
    
  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;
  
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  
  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT()
{
  unsigned int ut;
  
  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();
  
  // Wait at least 4.5ms
  delay(5);
  
  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;
  
  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();
  
  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));
  
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom(BMP085_ADDRESS, 3);
  
  // Wait for data to become available
  while(Wire.available() < 3)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  xlsb = Wire.read();
  
  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);
  
  return up;
}


