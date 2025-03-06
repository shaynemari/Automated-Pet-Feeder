#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>  // Core graphics library

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDR   0x3C  // Default I2C address for OLED display
#define SDA_PIN 21
#define SCL_PIN 22

// Create OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// WiFi credentials
const char* ssid = "LAPTOP-KE8SMJ1I 5187";
const char* password = "z1101Q@2";

// Telegram Bot credentials
#define BOTtoken "7759867874:AAGSjcLH0H5bEj4EdXRiOEviFYf4Ix6jBto"
#define CHAT_ID "5329360116"

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Telegram bot check interval
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

// Servo pin
#define SERVO_PIN 19  // Servo Motor

// Servo object
Servo myservo;

// Variables
unsigned long lastFeedingTime = 0;
unsigned long feedingInterval = 10 * 1000; // 10 seconds interval

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name   = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Food will be dispensed every 10 seconds.\n";
      bot.sendMessage(chat_id, welcome, "");
    }
  }
}

void setup() {
  Serial.begin(115200);

  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");
    client.setTrustAnchors(&cert);
  #endif

  // Initialize the servo
  myservo.attach(SERVO_PIN);

  // Initialize OLED display
  Wire.begin(SDA_PIN, SCL_PIN);  // Custom I2C pins
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check for Telegram messages
  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Received Telegram message");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  // Check if it's time to feed (every 10 seconds)
  if (millis() - lastFeedingTime >= feedingInterval) {
    countdownAndDispense();
    lastFeedingTime = millis(); // Update last feeding time
  }
}

void countdownAndDispense() {
  // Countdown display
  for (int i = 10; i > 0; i--) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Feeding in: ");
    display.print(i);
    display.print("s");
    display.display();
    delay(1000);
  }

  // Show "dispensing" on OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Dispensing...");
  display.display();

  myservo.write(90); // Open feeder
  delay(2000);  // Keep the feeder open for 2 seconds
  myservo.write(0); // Close feeder
  delay(500);

  Serial.println("Food has been dispensed");

  // Show "Dispensed!" message on OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Dispensed!");
  display.display();
  delay(2000);  // Show "Dispensed!" for 2 seconds

  bot.sendMessage(CHAT_ID, "Food has been dispensed! ", "");
}
