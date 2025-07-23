#include <Arduino.h>


/***************************************************************************
 *
 *
 * Max & Marcel Displayprogrammierung based on Example of INA219_WE
 *
 * Further information can be found on:
 * https://wolles-elektronikkiste.de/ina219_1 (German)
 * https://wolles-elektronikkiste.de/en/ina219_1-current-and-power-sensor (English)
 *
 ***************************************************************************/
#include <Wire.h>
#include <INA219_WE.h>
#include <LiquidCrystal.h>

#define I2C_ADDRESS_1 0x40
#define I2C_ADDRESS_2 0x45

#define ONLYONE false // falls nur ein INA verwendet wird
#define LCDBright A1
#define MAXLINES 4 // Anzahl verschiedener Anzeigen auf LCD


#define DEBUG_ohne_HARDWARE true

unsigned long previousMillis = 0;
const long interval = 200; // 200ms

/* There are several ways to create your ina219_1 object:
 * ina219_1_WE ina219_1 = ina219_1_WE(); -> uses Wire / I2C Address = 0x40
 * ina219_1_WE ina219_1 = ina219_1_WE(I2C_ADDRESS); -> uses Wire / I2C_ADDRESS
 * ina219_1_WE ina219_1 = ina219_1_WE(&Wire); -> you can pass any TwoWire object
 * ina219_1_WE ina219_1 = ina219_1_WE(&Wire, I2C_ADDRESS); -> all together
 */
 // Adresszuweisung
INA219_WE ina219_1 = INA219_WE(I2C_ADDRESS_1);

INA219_WE ina219_2 = INA219_WE(I2C_ADDRESS_2);

//const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;  // marcis Board
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// globale Variablen
int x = 0; // Zeilen variable
int zustand = 1; //Mosfet zustand: 0 entspricht nicht leitend 1 entspricht leitend
int key = 0; // Tastendruck

// Variablen für Ausgabe
float Uges = 0;
float P_V = 0; // V für Verbraucher
float I_V = 0;
float Q_S = 10; // S für Speicher
float P_S = 0;

void setup()
{

  Serial.begin(9600);
  Wire.begin();
  if (DEBUG_ohne_HARDWARE)
  {
    Serial.println("Debugmode ohne INA!");
  }
  if (!ina219_1.init() && !DEBUG_ohne_HARDWARE)
  {
    Serial.println("ina219_1 not connected!");
    while (1)
      ;
  }
  if (!ina219_2.init() && !DEBUG_ohne_HARDWARE)
  {
    Serial.println("ina219_2 not connected!");
    // while(1);
  }
  // set up the LCD's number of columns and rows:
  analogWrite(LCDBright, 78);
  lcd.begin(16, 2);

  // Print a message to the LCD.

  lcd.print("3XM");
  lcd.setCursor(0,1);
  lcd.print("Maxi Max Marcel");
  /* Set ADC Mode for Bus and ShuntVoltage
  * Mode *            * Res / Samples *       * Conversion Time *
  BIT_MODE_9        9 Bit Resolution             84 µs
  BIT_MODE_10       10 Bit Resolution            148 µs
  BIT_MODE_11       11 Bit Resolution            276 µs
  BIT_MODE_12       12 Bit Resolution            532 µs  (DEFAULT)
  SAMPLE_MODE_2     Mean Value 2 samples         1.06 ms
  SAMPLE_MODE_4     Mean Value 4 samples         2.13 ms
  SAMPLE_MODE_8     Mean Value 8 samples         4.26 ms
  SAMPLE_MODE_16    Mean Value 16 samples        8.51 ms
  SAMPLE_MODE_32    Mean Value 32 samples        17.02 ms
  SAMPLE_MODE_64    Mean Value 64 samples        34.05 ms
  SAMPLE_MODE_128   Mean Value 128 samples       68.10 ms
  */
  // ina219_1.setADCMode(SAMPLE_MODE_32); // choose mode and uncomment for change of default

  /* Set measure mode
  POWER_DOWN - ina219_1 switched off
  TRIGGERED  - measurement on demand
  ADC_OFF    - Analog/Digital Converter switched off
  CONTINUOUS  - Continuous measurements (DEFAULT)
  */
  // ina219_1.setMeasureMode(CONTINUOUS); // choose mode and uncomment for change of default

  /* Set PGain
  * Gain *  * Shunt Voltage Range *   * Max Current (if shunt is 0.1 ohms) *
   PG_40       40 mV                    0.4 A
   PG_80       80 mV                    0.8 A
   PG_160      160 mV                   1.6 A
   PG_320      320 mV                   3.2 A (DEFAULT)
  */
  // Kalibrierung
  ina219_1.setPGain(PG_80); // choose gain and uncomment for change of default

  /* Set Bus Voltage Range
   BRNG_16   -> 16 V
   BRNG_32   -> 32 V (DEFAULT)
  */
  ina219_1.setBusRange(BRNG_16); // choose range and uncomment for change of default

  Serial.println("ina219_1 Current Sensor Example Sketch - Continuous");

  /* If the current values delivered by the ina219_1 differ by a constant factor
     from values obtained with calibrated equipment you can define a correction factor.
     Correction factor = current delivered from calibrated equipment / current delivered by ina219_1
  */
  ina219_1.setCorrectionFactor(0.84); // insert your correction factor if necessary
  ina219_2.setCorrectionFactor(0.84); // insert your correction factor if necessary

  /* If you experience a shunt voltage offset, that means you detect a shunt voltage which is not
     zero, although the current should be zero, you can apply a correction. For this, uncomment the
     following function and apply the offset you have detected.
  */
  ina219_1.setShuntVoltOffset_mV(0);     // insert the shunt voltage (millivolts) you detect at zero current
  ina219_2.setShuntVoltOffset_mV(-0.02); // insert the shunt voltage (millivolts) you detect at zero current
// MOSFET zu Beginn durchlässig machen
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
}


// Ausgabe wechselder Zeilen LCD
void LCD_OUTPUT()
{
  char buffer[16], floatStr[16];
  lcd.clear();
  lcd.setCursor(0, 0);
  if (x == 0)
  {

    dtostrf(Uges, 4, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "Spannung: %s V", floatStr);
    lcd.print(buffer);
    lcd.setCursor(0, 1);
    dtostrf(P_V, 5, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "P am V: %s mW", floatStr);
    lcd.print(buffer);
  }
  else if (x == 1)
  {
    dtostrf(P_V, 5, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "P am V: %s mW", floatStr);
    lcd.print(buffer);
    lcd.setCursor(0, 1);
    dtostrf(I_V, 5, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "I am V: %s mA", floatStr);
    lcd.print(buffer);
  }
  else if (x == 2)
  {
    dtostrf(I_V, 5, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "I am V: %s mA", floatStr);
    lcd.print(buffer);
    lcd.setCursor(0, 1);
    dtostrf(Q_S, 7, 1, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "Ladung:%smC", floatStr);
    lcd.print(buffer);
  }
  else if (x == 3)
  {
    dtostrf(Q_S, 7, 1, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "Ladung:%smC", floatStr);
    lcd.print(buffer);
    lcd.setCursor(0, 1);
    dtostrf(P_S, 5, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "Lade P: %s mW", floatStr);
    lcd.print(buffer);
  }
   else if (x == 4)
  {
    dtostrf(P_S, 5, 2, floatStr); // (Wert, Breite, Nachkommastellen, Zielpuffer)
    sprintf(buffer, "Lade P: %s mW", floatStr);
    lcd.print(buffer);
    lcd.setCursor(0, 1);
    lcd.print("MOSFET: "+String(zustand));
  }
}

//  Tastendruckerfassung
void getTaste()
{
  int analogWert = analogRead(A0);
  Serial.print(analogWert);
  Serial.print("  ;z "+zustand);
  Serial.println("  ;x "+x);
  if (key==0){
  // UP
  if (analogWert > 50 && analogWert < 150 && x > 0)
  {
    x--;
    LCD_OUTPUT();
    key = 1;
  }
  // DOWN
  if (analogWert > 200 && analogWert < 300 && x < MAXLINES)
  {
    x++;
    LCD_OUTPUT();
    key = 1;
  }
  // Änderung des MOSFETs
  // Selest wenn MOSFET nicht durchlässig
  if (analogWert > 600 && analogWert < 750 && zustand == 0)
  {
    zustand = 1;
    digitalWrite(0, HIGH);
    LCD_OUTPUT();
    key = 1;
  }
  // Selest wenn MOSFET durchlässig
  else if (analogWert > 600 && analogWert < 750 && zustand == 1)
  {
    zustand = 0;
    digitalWrite(0, LOW);
    LCD_OUTPUT();
    key = 1;
  }
}
  if (analogWert > 1020 && analogWert < 1030 && key == 1)
  {
    key = 0;
  }
}


// Zusätzliche Ausgabe für DEBUGGING (;
void Serial_OUTPUT()
{
  /* Serial.print("Spannung Zelle [V]: ");
  Serial.println(Uges);
  Serial.print("Ausgangsstromstärke I [mA]: ");
  Serial.println(I_V);
  Serial.print("Ausgangsleistung P [mW]: ");
  Serial.println(P_V);
  Serial.print("Ladung Kondensator Q [mC]:");
  Serial.println(Q_S);
  Serial.print("LadeLeistung P [mW]: ");
  Serial.println(P_S); */
}
void measurementFunction()
{
  float busVoltage_V_1 = 0.0; // benutzen
  float current_mA_1 = 0.0;   // benutzen
  float power_mW_1 = 0.0;     // fragen sonst rechnen
  bool ina219_1_overflow = false;

  float busVoltage_V_2 = 0.0; // benutzen
  float current_mA_2 = 0.0;   // benutzen
  float power_mW_2 = 0.0;     // fragen sonst rechnen
  bool ina219_2_overflow = false;

  busVoltage_V_1 = ina219_1.getBusVoltage_V();
  current_mA_1 = ina219_1.getCurrent_mA();
  power_mW_1 = ina219_1.getBusPower();
  ina219_1_overflow = ina219_1.getOverflow();

  if (!ONLYONE)
  {
    busVoltage_V_2 = ina219_2.getBusVoltage_V();
    current_mA_2 = ina219_2.getCurrent_mA();
    power_mW_2 = ina219_2.getBusPower();
    ina219_2_overflow = ina219_2.getOverflow();
  }
  // Umrechnung der Werte auf Ausgabevariablen
  Uges = busVoltage_V_1;
  P_V = power_mW_2;
  I_V = current_mA_2;
  float Qsalt = Q_S;
  Q_S = Qsalt + (current_mA_1 - current_mA_2) / 1000 * interval; // integration for charge
  P_S = power_mW_1 - power_mW_2;
}

void loop()
{
  // timemeasurement for integration
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you measured something
    previousMillis = currentMillis;

    // Do something here
    measurementFunction();
    LCD_OUTPUT();
    Serial_OUTPUT();
  }
  getTaste();
}






