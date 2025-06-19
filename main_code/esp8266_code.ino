#include <ESP8266WiFi.h>         // ESP8266 Wi-Fi kütüphanesi, Wi-Fi özelliklerini kullanabilmek için
#include <ESP8266WebServer.h>     // Web sunucu için ESP8266 web sunucu kütüphanesi
#include <Servo.h>                // Servo motoru kontrol etmek için Servo kütüphanesi

const char* ssid = "Wİ-Fİ name";  // Wi-Fi ağ adı
const char* password = "Wİ-Fİ password";       // Wi-Fi ağ şifresi

// Pin atamaları
const int trigPin = 4;     // D2 pini mesafe sensörü trig girişine bağlı
const int echoPin = 5;     // D1 pini mesafe sensörü echo çıkışına bağlı
const int fanPin = 12;     // D6 pini fan kontrolü için bağlı
const int buzzerPin = 13;  // D7 pini buzzer kontrolü için bağlı
const int servoPin = 14;   // D5 pini servo motor kontrolü için bağlı

Servo myservo;                     // Servo motoru için servo nesnesi oluşturuluyor
ESP8266WebServer server(80);       // Web sunucu nesnesi oluşturuluyor, port 80 kullanılıyor

long duration;                 // Mesafe ölçümü için sürenin tutulacağı değişken
float distanceCm;              // Ölçülen mesafeyi santimetre cinsinden tutacak değişken

// Durum değişkenleri
String fanDurumu = "Kapalı";      // Fanın durumu
String buzzerDurumu = "Kapalı";  // Buzzer'ın durumu
String servoDurumu = "Ortada";   // Servo motorun durumu
bool manuelMod = false;          // Manuel mod kontrol değişkeni, varsayılan olarak otomatik mod

void setup() {
  Serial.begin(115200);      // Seri haberleşme başlatılır, 115200 baud hızında
  pinMode(trigPin, OUTPUT);  // trigPin çıkış olarak ayarlanır
  pinMode(echoPin, INPUT);   // echoPin giriş olarak ayarlanır
  pinMode(fanPin, OUTPUT);   // fanPin çıkış olarak ayarlanır
  pinMode(buzzerPin, OUTPUT);// buzzerPin çıkış olarak ayarlanır
  myservo.attach(servoPin);  // Servo motor D5 pinine bağlanır
  myservo.write(90);         // Servo motor başlangıç pozisyonuna (90 derece) getirilir

  WiFi.begin(ssid, password);  // Wi-Fi ağına bağlanmaya başla
  Serial.print("WiFi'ye bağlanılıyor...");
  while (WiFi.status() != WL_CONNECTED) {  // Wi-Fi bağlantısı kurulana kadar bekle
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi bağlandı!");
  Serial.println("IP: " + WiFi.localIP().toString()); // Bağlantı başarılıysa IP adresini yazdır

  // Web sunucusunun URL endpointlerini tanımlıyoruz
  server.on("/", handleRoot);        // Ana sayfa için handler
  server.on("/mesafe", handleMesafe);  // Mesafe ölçümü için handler

  // Fan açma kapama kontrolü
  server.on("/fan/on", []() {
    manuelMod = true;              // Manuel modda çalışıldığını belirt
    digitalWrite(fanPin, HIGH);    // Fanı aç
    fanDurumu = "Manuel Açıldı";   // Fan durumu güncellenir
    server.send(200, "text/plain", "Fan Manuel Açıldı"); // Web tarayıcısına yanıt gönder
  });

  // Fan kapama kontrolü
  server.on("/fan/off", []() {
    manuelMod = true;              // Manuel modda çalışıldığını belirt
    digitalWrite(fanPin, LOW);     // Fanı kapat
    fanDurumu = "Kapalı";          // Fan durumu güncellenir
    server.send(200, "text/plain", "Fan Kapandı"); // Web tarayıcısına yanıt gönder
  });

  // Buzzer açma kontrolü
  server.on("/buzzer/on", []() {
    manuelMod = true;              // Manuel modda çalışıldığını belirt
    digitalWrite(buzzerPin, HIGH); // Buzzer'ı aç
    buzzerDurumu = "Manuel Açıldı";// Buzzer durumu güncellenir
    server.send(200, "text/plain", "Buzzer Açıldı"); // Web tarayıcısına yanıt gönder
  });

  // Buzzer kapama kontrolü
  server.on("/buzzer/off", []() {
    manuelMod = true;              // Manuel modda çalışıldığını belirt
    digitalWrite(buzzerPin, LOW);  // Buzzer'ı kapat
    buzzerDurumu = "Kapalı";       // Buzzer durumu güncellenir
    server.send(200, "text/plain", "Buzzer Kapandı"); // Web tarayıcısına yanıt gönder
  });

  // Servo sağa döndürme kontrolü
  server.on("/servo/right", []() {
    manuelMod = true;             // Manuel modda çalışıldığını belirt
    myservo.write(180);           // Servo motoru 180 derece sağa döndür
    servoDurumu = "Sağa Döndü";   // Servo durumu güncellenir
    server.send(200, "text/plain", "Servo Sağa Döndü"); // Web tarayıcısına yanıt gönder
  });

  // Servo sola döndürme kontrolü
  server.on("/servo/left", []() {
    manuelMod = true;             // Manuel modda çalışıldığını belirt
    myservo.write(0);             // Servo motoru 0 derece sola döndür
    servoDurumu = "Sola Döndü";   // Servo durumu güncellenir
    server.send(200, "text/plain", "Servo Sola Döndü"); // Web tarayıcısına yanıt gönder
  });

  // Otomatik moda geçiş kontrolü
  server.on("/otomatik", []() {
    manuelMod = false;            // Otomatik moda geçildi
    server.send(200, "text/plain", "Otomatik Moda Geçildi"); // Web tarayıcısına yanıt gönder
  });

  // Manuel moda geçiş kontrolü
  server.on("/manuel", []() {
    manuelMod = true;             // Manuel moda geçildi
    server.send(200, "text/plain", "Manuel Moda Geçildi"); // Web tarayıcısına yanıt gönder
  });

  server.begin();                // Web sunucusunu başlat
  Serial.println("Web sunucu başlatıldı.");
}

void loop() {
  server.handleClient();         // Web sunucusunun istemci isteklerini işleme alması
}

void handleRoot() {
  server.send(200, "text/plain", "ESP8266 Kontrol Paneli Aktif!");  // Ana sayfa için mesaj gönder
}

void handleMesafe() {
  digitalWrite(trigPin, LOW);            // Trig pinini düşük yaparak mesafe ölçümüne başla
  delayMicroseconds(2);                  // Kısa bir gecikme
  digitalWrite(trigPin, HIGH);           // Trig pinini yüksek yaparak ultrasonik ses dalgası gönder
  delayMicroseconds(10);                 // Kısa bir gecikme
  digitalWrite(trigPin, LOW);            // Trig pinini tekrar düşük yap

  duration = pulseIn(echoPin, HIGH, 30000); // Echo pininden gelen geri yansıyan sinyalin süresini ölç
  if (duration == 0) {                   // Eğer sinyal alınmazsa (mesafe ölçülememişse)
    server.send(200, "application/json", "{\"mesafe\":\"0\", \"fan\":\"" + fanDurumu + "\", \"servo\":\"" + servoDurumu + "\", \"buzzer\":\"" + buzzerDurumu + "\"}"); 
    return; // Hiçbir mesafe verisi gelmediyse sıfır dönülür
  }

  distanceCm = duration * 0.034 / 2;     // Ses dalgasının mesafeye dönüştürülmesi (ses hızını dikkate alarak)

  if (!manuelMod) { // Eğer manuel modda değilse (otomatik mod)
    // FAN kontrolü
    if (distanceCm > 0 && distanceCm <= 10) {      // Eğer mesafe 10 cm'ye kadar ise
      digitalWrite(fanPin, HIGH);    // Fanı aç
      fanDurumu = "Hızlı";           // Fan durumu güncellenir
    } else if (distanceCm > 10 && distanceCm <= 20) {  // Eğer mesafe 10 cm ile 20 cm arasında ise
      digitalWrite(fanPin, HIGH);    // Fanı aç
      fanDurumu = "Yavaş";           // Fan durumu güncellenir
    } else {
      digitalWrite(fanPin, LOW);     // Fanı kapat
      fanDurumu = "Kapalı";          // Fan durumu güncellenir
    }

    // BUZZER kontrolü
    if (distanceCm > 0 && distanceCm <= 10) {    // Eğer mesafe 10 cm'ye kadar ise
      digitalWrite(buzzerPin, HIGH);  // Buzzer'ı aç
      buzzerDurumu = "Açık";          // Buzzer durumu güncellenir
    } else {
      digitalWrite(buzzerPin, LOW);   // Buzzer'ı kapat
      buzzerDurumu = "Kapalı";        // Buzzer durumu güncellenir
    }

    // SERVO kontrolü
    if (distanceCm > 0 && distanceCm <= 10) {  // Eğer mesafe 10 cm'ye kadar ise
      myservo.write(180);                // Servo motoru sağa döndür
      servoDurumu = "Sağa Döndü";       // Servo durumu güncellenir
    } else if (distanceCm > 10 && distanceCm <= 20) {  // Eğer mesafe 10 cm ile 20 cm arasında ise
      myservo.write(0);                  // Servo motoru sola döndür
      servoDurumu = "Sola Döndü";       // Servo durumu güncellenir
    } else {
      myservo.write(90);                 // Servo motoru ortada tut
      servoDurumu = "Ortada";           // Servo durumu güncellenir
    }
  }

  // Mesafe, fan, buzzer ve servo durumları JSON formatında gönderilir
  String json = "{\"mesafe\":\"" + String(distanceCm, 2) + "\", \"fan\":\"" + fanDurumu + "\", \"servo\":\"" + servoDurumu + "\", \"buzzer\":\"" + buzzerDurumu + "\"}";
  server.send(200, "application/json", json);  // JSON yanıtı gönder
}

