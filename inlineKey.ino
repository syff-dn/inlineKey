#include <ESP8266WiFi.h>           // Board ESP8266
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>  // Library Universal Telegram Bot
#include <ArduinoJson.h>           // Library Arduino Json
#include <Servo.h>                 // Library Servo

/*******************************************************************/
/* Ganti Nama SSID, Password WiFi, Bot Token Dan Chat ID           */
/*******************************************************************/
#define WIFI_SSID "Queen za"                                       //
#define WIFI_PASSWORD "aang.ajah"                                      //
#define BOT_TOKEN "bot_token"  //
#define CHAT_ID "chat_id"                                        //
//-----------------------------------------------------------------//

X509List cert(TELEGRAM_CERTIFICATE_ROOT);

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
Servo myservo;


#define trigDepan D1  // Trigger Pin Depan
#define echoDepan D2  // Echo Pin Depan

#define trigCek D8  // Trigger Pin Cek Kapasitas
#define echoCek D7  // Echo Pin Cek Kapasitas

#define servoPin D4

#define Output_Relay1 LED_BUILTIN  // Atur Pin led builtin

#define Hidup LOW
#define Mati HIGH


/*******************************************************************/
/*               deklarasi variabel yg digunalan                   */
/*******************************************************************/
bool sampahPenuh = true;
bool notifikasi = true;


int delayKirim = 600000;
unsigned long waktuKirim;

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "User Tidak Terdaftar", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name = bot.messages[i].from_name;

    if (from_name == "") {
      from_name = "Guest";
    }

    if (bot.messages[i].type == F("callback_query")) {
      String text = bot.messages[i].text;
      Serial.print("Call back button pressed with : ");
      Serial.println(text);

      if (text == F("Aktif")) {
        notifikasi = true;
        digitalWrite(Output_Relay1, Hidup);
        bot.sendMessage(chat_id, "Notifikasi diaktifkan", "");
      }

      else if (text == F("Nonaktif")) {
        notifikasi = false;
        digitalWrite(Output_Relay1, Mati);
        bot.sendMessage(chat_id, "Notifikasi dimatikan", "");
      }

      else if (text == F("Batal")) {
        bot.sendMessage(chat_id, "Pengaturan dibatalkan", "");
      }
    }

    else {
      if (text == F("Setting")) {
        String welcome;
        if (sampahPenuh == true) {
          welcome += "Status notifikasi Sa'at ini dalam keadaan AKTIF.\n\nNotifikasi sampah penuh akan dikirim dalam selang waktu 5 menit.";
        } else {
          welcome += "Status notifikasi Sa'at ini dalam keadaan NONAKTIF.\n\nNotifikasi sampah penuh tidak akan dikirim.";
        }

        String Kirim_Perintah = F("[[{ \"text\" : \"AKTIF\", \"callback_data\" : \"Aktif\" },{ \"text\" : \"NONAKTIF\", \"callback_data\" : \"Nonaktif\" }],");
        Kirim_Perintah += F("[{ \"text\" : \"                   BATAL                   \", \"callback_data\" : \"Batal\" }]]");
        bot.sendMessageWithInlineKeyboard(chat_id, welcome, "", Kirim_Perintah);
      }

      else if (text == F("Cek Sampah")) {
        if (sampahPenuh == true) {
          bot.sendMessage(chat_id, "Tempat sampah Penuh.", "");
        }

        else {
          String kapasitas = "Ketinggian sampah saat ini " + String(30) + " cm";
          bot.sendMessage(chat_id, kapasitas, "");
        }
      }

      else if (text == F("/start")) {
        String welcome = "Halo " + from_name + "..\n\nSelamat datang di Dashboard Monitoring Smart Trash Bin berbasis IoT.\n\n";
        welcome += "Untuk melihat ketinggian sampah, silahkan klik tombol Cek Sampah\n\n";
        welcome += "Untuk aktifkan dan nonaktifkan notifikasi klik tombol **Setting**";

        String keyboardJson = "[[\"Cek Sampah\"],[\"Setting\"]]";
        bot.sendMessageWithReplyKeyboard(chat_id, welcome, "", keyboardJson, true);
      } else {
        bot.sendMessage(chat_id, "Perintah tidak sesuai.", "");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("Menggunakan Board ESP38266");
  configTime(0, 0, "pool.ntp.org");
  client.setTrustAnchors(&cert);


  myservo.attach(servoPin);

  pinMode(trigDepan, OUTPUT);
  pinMode(echoDepan, INPUT);

  pinMode(trigCek, OUTPUT);
  pinMode(echoCek, INPUT);

  pinMode(Output_Relay1, OUTPUT);
  digitalWrite(Output_Relay1, Mati);


  //Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi -> ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Bot Siap..");

  // bot.sendMessage(CHAT_ID, "Bot siap..\nTekan : /start", "");
}

void loop() {
  if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }


  if (sampahPenuh == true && notifikasi == true) {
    if (millis() - waktuKirim > delayKirim) {
      bot.sendMessage(CHAT_ID, "Bot siap..\nTekan : /start", "");

    }
  }
}



int bukaSampah() {
  // Mengirim sinyal ultrasonik
  digitalWrite(trigDepan, LOW);
  delayMicroseconds(2);
  digitalWrite(trigDepan, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigDepan, LOW);

  // Menerima sinyal pantul
  long duration = pulseIn(echoDepan, HIGH);

  // Menghitung jarak
  int distance = duration * 0.034 / 2;

  return distance;
}


int cekSampah() {
  // Mengirim sinyal ultrasonik
  digitalWrite(trigCek, LOW);
  delayMicroseconds(2);
  digitalWrite(trigCek, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigCek, LOW);

  // Menerima sinyal pantul
  long duration = pulseIn(echoCek, HIGH);

  // Menghitung jarak
  int distance = duration * 0.034 / 2;

  return distance;
}