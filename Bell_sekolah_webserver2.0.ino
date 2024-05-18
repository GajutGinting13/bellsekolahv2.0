#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "RTClib.h"
#include <EEPROM.h>;
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(D3, D4);
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"minggu", "senin", "selasa", "rabu", "kamis", "jumat", "sabtu"};

bool modewifi = 0, toggle, status_btn;
String feedback[10];
String datamasuk, kondisi, pelajaran, lanjut;
bool parsing = false;
int layar;
#define buzzer 13
#define tombol 14

ESP8266WebServer server(80);
void setup() {
  Serial.begin(115200);
  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("HELLO WORLD");
  pinMode(buzzer, OUTPUT);
  pinMode(tombol, INPUT);
  EEPROM.begin(512);
  mySerial.begin (9600);
  mp3_set_serial (mySerial);
  delay(1000);
  mp3_set_volume (25);
  if (digitalRead(tombol) == 1) {
    toggle = 0;
  } else {
    toggle = 1;
  }
  if (!rtc.begin())
  {
    Serial.println("Tidak Ada RTC. Periksa Sambungan");
    Serial.flush();
    while (1)
      delay(10);
  }
  server.on("/index.html", HTTP_GET, []()
  {
    String login = "";
    File file = SPIFFS.open("/index.html", "r");
    login = file.readString();
    file.close();
    server.send(200, "text/html", login);
  });
  server.on("/js.js", HTTP_GET, []()
  {
    String javascript = "";
    File file = SPIFFS.open("/js.js", "r");
    javascript = file.readString();
    file.close();
    server.send(200, "text/plain", javascript);
  });
  server.on("/style.css", HTTP_GET, []()
  {
    String css = "";
    File file = SPIFFS.open("/style.css", "r");
    css = file.readString();
    file.close();
    server.send(200, "text/css", css);
  });

  // GET Data Home
  server.on("/home.html", HTTP_GET, []()
  {
    String html = "";
    File file = SPIFFS.open("/home.html", "r");
    html = file.readString();
    file.close();
    server.send(200, "text/html", html);
    String nilai = server.arg("waktu-sekarang");
    String suara = server.arg("test");
    if (nilai != "") {
      datamasuk = nilai;
      parsing = true;
    } else if (suara == "1") {
      int a = random(1, 9);
      mp3_play(a);
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("   Test Suara   ");
      lcd.setCursor(0, 1);
      lcd.print("================");
      delay(10000);
      lcd.noBacklight();
    }
    if (parsing) {
      parsing = false;
      int x = 0;
      for (int i = 0; i < 10; i++) {
        feedback[i] = "";
      }
      for (int i = 0; i < datamasuk.length(); i++) {
        if (datamasuk[i] == ',' or datamasuk[i] == ':') {
          x++;
          feedback[x] = "";
        } else {
          feedback[x] += datamasuk[i];
        }
      }
      uint32_t tahun = strtoul(feedback[0].c_str(), NULL, 10);
      uint32_t bulan = strtoul(feedback[1].c_str(), NULL, 10);
      uint32_t tanggal = strtoul(feedback[2].c_str(), NULL, 10);
      uint32_t jam = strtoul(feedback[3].c_str(), NULL, 10);
      uint32_t menit = strtoul(feedback[4].c_str(), NULL, 10);
      uint32_t detik = strtoul(feedback[5].c_str(), NULL, 10);
      rtc.adjust(DateTime(tahun, bulan, tanggal, jam, menit, detik));
      Serial.println("Berhasil Setting Jam");
      varifikasi();
    }
  });
  server.on("/hapus.html", HTTP_GET, []() {
    String hapus = "";
    File file = SPIFFS.open("/hapus.html", "r");
    hapus = file.readString();
    file.close();
    server.send(200, "text/html", hapus);
    String hari = server.arg("hari");
    String audio = server.arg("audio");
    if (hari != "") {
      hapusjadwal(hari, audio);
      varifikasi();
      File file = SPIFFS.open("/" + hari + ".csv", "r");
      while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
      }

      file.close();
    }
  });
  server.on("/senin.html", HTTP_GET, []() {
    loadhalamanjadwal("senin");
  });
  server.on("/selasa.html", HTTP_GET, []() {
    loadhalamanjadwal("selasa");
  });
  server.on("/rabu.html", HTTP_GET, []() {
    loadhalamanjadwal("rabu");
  });
  server.on("/kamis.html", HTTP_GET, []() {
    loadhalamanjadwal("kamis");
  });
  server.on("/jumat.html", HTTP_GET, []() {
    loadhalamanjadwal("jumat");
  });
  server.on("/sabtu.html", HTTP_GET, []() {
    loadhalamanjadwal("sabtu");
  });
  server.on("/list.html", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><link rel='stylesheet' href='style.css'><title>Semua Data</title></head><body><div class='head'><a href='home.html'><button class='btn-logout'>Home</button></a></div><div class='contain' style='margin:20px;'><h1>Semua Data</h1><table style='padding: 20px;'><tr><th>Hari</th><th>Jadwal</th><th>Waktu</th><th>Audio</th></tr>";
    tampilkanData("senin", html);
    tampilkanData("selasa", html);
    tampilkanData("rabu", html);
    tampilkanData("kamis", html);
    tampilkanData("jumat", html);
    tampilkanData("sabtu", html);
    html += "</table></div><footer><p>Copyright &copy; <span id='copyright'></span> / Gajut X Elektronika</p></footer></body></html>";
    server.send(200, "text/html", html);
  });
  server.begin();
}

void tampilkanData(String hari, String &html) {
  File file = SPIFFS.open("/" + hari + ".csv", "r");
  while (file.available()) {
    String line = file.readStringUntil('\n');
    html += "<tr>";
    html += "<td>" + getValue(line, ',', 0) + "</td>";
    html += "<td>" + getValue(line, ',', 1) + "</td>";
    html += "<td>" + getValue(line, ',', 2) + ":" + getValue(line, ',', 3) + "</td>";
    html += "<td>" + getValue(line, ',', 4) + "</td>";
    html += "</tr>";
  }
  file.close();
}

void loadhalamanjadwal(String tujuan) {
  String hari_tujuan = "";
  File file = SPIFFS.open("/" + tujuan + ".html", "r");
  hari_tujuan = file.readString();
  file.close();
  server.send(200, "text/html", hari_tujuan);
  String id = server.arg("id");
  String waktu = server.arg("waktu");
  String audio = server.arg("audio");
  String hari = server.arg("hari");
  // Add
  if (id != "") {
    datamasuk = waktu;
    int x = 0;
    for (int i = 0; i < 10; i++) {
      feedback[i] = "";
    }
    for (int i = 0; i < datamasuk.length(); i++) {
      if (datamasuk[i] == ':') {
        x++;
        feedback[x] = "";
      } else {
        feedback[x] += datamasuk[i];
      }
    }
    addData(hari + ',' + id + ',' +  String(feedback[0]) + ',' +  String(feedback[1]) + ',' + audio, hari);
    varifikasi();
  }
}
void varifikasi() {
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
  delay(50);
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
  delay(50);
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void cekjadwal(String alamat, int jam, int menit, int detik) {
  File file = SPIFFS.open("/" + alamat + ".csv", "r");
  while (file.available()) {
    String line = file.readStringUntil('\n');
//    Serial.println("Baris: " + line);
    String hari = getValue(line, ',', 0);
    String kegiatan = getValue(line, ',', 1);
    String jam_csv = getValue(line, ',', 2);
    String menit_csv = getValue(line, ',', 3);
    String audio = getValue(line, ',', 4);
    if (jam_csv.toInt() == jam and menit_csv.toInt() == menit and detik >= 0 and detik <= 5) {
      mp3_play(audio.toInt());
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print(kegiatan);
      delay(10000);
      lcd.noBacklight();
    }
  }
  file.close();
}

void lihat(String tujuan) {
  Serial.println("Data dihari " + tujuan);
  File file = SPIFFS.open("/" + tujuan + ".csv", "r");
  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.println(line);
  }
  file.close();
}

void hapusjadwal(String hari, String code) {
  File file = SPIFFS.open("/" + hari + ".csv", "r");
  String tempFileName = "/temp.csv";
  File tempFile = SPIFFS.open(tempFileName, "w");
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.indexOf(code) == -1) {
      tempFile.println(line);
    }
  }
  file.close();
  tempFile.close();
  SPIFFS.remove("/" + hari + ".csv");
  SPIFFS.rename(tempFileName, "/" + hari + ".csv");
  Serial.println("Data dengan kode " + code + " berhasil dihapus");
}

void addData(String newData, String alamat) {
  File file = SPIFFS.open("/" + alamat + ".csv", "a");
  file.println(newData);
  file.close();
}
void loop() {
  //  status_btn = digitalRead(tombol);
  //  if (status_btn == 1) {
  //    while (true) {
  //      if (digitalRead(tombol) == 0) {
  //        modewifi = !modewifi;
  //        Serial.println(modewifi);
  //        break;
  //      }
  //    }
  //  }
  modewifi = digitalRead(tombol);
  if (modewifi == 1 and toggle == 0) {
    lcd.backlight();
    kondisi = "EDIT";
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("Bell Sekolah", "belsekolah23");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(">>  EDIT MODE <<");
    lcd.setCursor(0, 1);
    lcd.print("================");
    delay(2000);
    bool a = true;
    while (a)
    {
      Serial.print(".");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Menunggu....");
      if (WiFi.softAPgetStationNum() > 0) {
        lcd.setCursor(0, 1);
        lcd.print(">>  TERHUBUNG <<");
        varifikasi();
        a = false;
      }
      if (digitalRead(tombol) == 0) {
        a = false;
      }
      delay(500);
    }
    toggle = 1;
    if (SPIFFS.begin()) {
      Serial.println("SPIFFS started");
    }
    else {
      Serial.println("Error starting SPIFFS!");
    }
    Serial.println("Masuk Mode Edit Jadwal");
  }
  else if (modewifi == 0 and toggle == 1) {
    lcd.backlight();
    varifikasi();
    kondisi = "RUN";
    WiFi.mode(WIFI_OFF);
    Serial.println("Keluar Dari Mode Edit Jadwal");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(">>  RUN MODE  <<");
    lcd.setCursor(0, 1);
    lcd.print("================");
    delay(2000);
    toggle = 0;
  }
  server.handleClient();
  DateTime now = rtc.now();
  int tahun = now.year();
  int bulan = now.month();
  int tanggal = now.day();
  int jam = now.hour();
  int menit = now.minute();
  int detik = now.second();
  int suhu = rtc.getTemperature();
  String hari = daysOfTheWeek[now.dayOfTheWeek()];
  lcd.clear();
  if (layar <= 5) {
    lcd.setCursor(0, 0);
    lcd.print(hari + " " + String(jam) + ":" + String(menit) + ":" + String(detik));
    lcd.setCursor(0, 1);
    lcd.print(String(tanggal) + "/" + String(bulan) + "/" + String(tahun) + " " + kondisi);
    if (modewifi == 1) {
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("T ELEKTRONIKA");
    lcd.setCursor(0, 1);
    lcd.print("           V2.0");
    if (layar == 10) {
      layar = 1;
    }
  }
  cekjadwal(hari, jam, menit, detik);
  layar++;
  delay(1000);
  lcd.clear();
}
