#include <Wire.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // RS, E, D4, D5, D6, D7
SoftwareSerial gsm(9, 10);          // RX, TX for GSM SIM800A

const int MPU = 0x68;  // I2C address of MPU6050
int16_t AcX, AcY, AcZ;
const int ACCIDENT_THRESHOLD = 30000; // Tilt/impact threshold

const int buzzerPin = 8;      // Buzzer pin (D8)
const int ledPin = 12;         // LED pin (D12)
const int gsmPowerPin = 11;   // GSM power control pin

bool gsmInitialized = false;
unsigned long lastCheckTime = 0;

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  lcd.begin(16, 2);

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(gsmPowerPin, OUTPUT);

  // Gyro init
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  
  Wire.write(0);     
  Wire.endTransmission(true);

  lcd.print("System Initializing");
  delay(2000);
  lcd.clear();
  lcd.print("Calibrating...");
  delay(2000);
  lcd.clear();
  lcd.print("System Ready");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastCheckTime > 1000) {
    lastCheckTime = currentTime;

    // Read accelerometer values
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();

    int absX = abs(AcX);

    lcd.setCursor(0, 0);
    lcd.print("Tilt: "); lcd.print(absX);
    lcd.setCursor(0, 1);
    lcd.print("Monitoring...    ");

    if (absX > ACCIDENT_THRESHOLD) {
      lcd.clear();
      lcd.print("Accident Detected!");
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(ledPin, HIGH);
      delay(1000);
      checkAndSendLocation();
      digitalWrite(buzzerPin, LOW);
      digitalWrite(ledPin, LOW);
    }
  }
}

void checkAndSendLocation() {
  if (!gsmInitialized) {
    lcd.clear();
    lcd.print("Init GSM...");
    digitalWrite(gsmPowerPin, HIGH);
    initGSM();
  }

  lcd.clear();
  lcd.print("Locating...");

  gsm.println("AT+CIPGSMLOC=1,1");
  delay(5000);

  String location = "";
  while (gsm.available()) {
    String line = gsm.readStringUntil('\n');
    if (line.startsWith("+CIPGSMLOC")) {
      location = line;
    }
  }

  lcd.clear();
  lcd.print("Sending SMS...");
  sendSMS(location);
  delay(2000);
  lcd.clear();
  lcd.print("Alert Sent!");
  delay(1500);
}

void initGSM() {
  gsm.println("AT");
  delay(1000);

  gsm.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(1000);

  gsm.println("AT+SAPBR=3,1,\"APN\",\"internet\""); // Replace APN if needed
  delay(1000);

  gsm.println("AT+SAPBR=1,1");
  delay(3000);

  gsmInitialized = true;
}

void sendSMS(String msg) {
  gsm.println("AT+CMGF=1");
  delay(1000);
  gsm.println("AT+CMGS=\"+91XXXXXXXXXX\""); // Replace with emergency contact
  delay(1000);
  gsm.print("Accident Alert! Location: ");
  gsm.print(msg);
  gsm.write(26); // CTRL+Z to send
  delay(5000);
}
