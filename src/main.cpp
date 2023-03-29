#include <Arduino.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <FirebaseESP32.h>

#define trigPin 14            // Trigger pin
#define echoPin 12            // Echo pin
#define servoPin 13           // Servo pin
#define distance_threshold 10 // Distance threshold in cm

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Apek"
#define WIFI_PASSWORD "kelvin108"
#define API_KEY "AIzaSyD-q4nQwkG8Uj5I1Fbz0BtTs9huzhQRwsA"
#define DATABASE_URL "https://smartbin888-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servo;    // Create servo object
float distance; // Distance variable
float duration; // Duration variable

// define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

byte progressBar[8] = {
    0b00000,
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000};

void start_wifi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void start_firebase()
{
    /* Assign the API key (required) */
    config.api_key = API_KEY;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Sign up */
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("Sign up successfully!");
        signupOK = true;
    }
    else
    {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void start_lcd()
{
    lcd.begin(16, 2);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("IoT Smart Bin  ");
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("       ApekTahua");
    delay(2000);
    lcd.clear();
}

void write_to_firebase()
{
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1500 || sendDataPrevMillis == 0))
    {
        sendDataPrevMillis = millis();
    }
    // Write distance to Firebase and servo motor status
    if (Firebase.RTDB.setFloat(&fbdo, "/Jarak ", distance))
    {
        count++;
        Serial.println("Distance data updated!");
        Serial.println("Path: ");
        Serial.println(fbdo.dataPath());
        Serial.println("Type: ");
        Serial.println(fbdo.dataType());
        Serial.println("Value: ");
        Serial.println(fbdo.floatData());
        Serial.println("ETag: ");
        Serial.println(fbdo.ETag());
        Serial.println("Data sent: ");
        Serial.println(count);
    }
    else
    {
        Serial.println("Distance data update failed!");
        Serial.println("Error: ");
        Serial.println(fbdo.errorReason());
    }
}

// Make a progressive bar at a max 10 bars, so 1 bar = 5cm
void bar()
{
    lcd.createChar(0, progressBar);
    lcd.setCursor(0, 1);
    for (int i = 0; i < 10; i++)
    {
        if (distance > i * 5)
        {
            lcd.write(byte(0));
        }
        else
        {
            lcd.print(" ");
        }
    }
}

void move_servo()
{
    if (distance < distance_threshold)
    {
        servo.write(360);
        delay(500);
    }
}

void setup()
{
    Serial.begin(9600); // Starts the serial communication
    start_wifi();
    start_firebase();
    start_lcd();

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    servo.attach(servoPin);
    servo.write(0);
}

void loop()
{
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * SOUND_SPEED / 2;

    if (distance < distance_threshold)
    {
        move_servo();
    }
    else
    {
        servo.write(0);
    }

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    lcd.clear();
    lcd.setCursor(0, 0); // start to print at the first row
    lcd.print("Jarak: ");
    lcd.print(distance);
    lcd.print(" cm");

    bar();
    write_to_firebase();
    delay(100);
}
