#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define RST_PIN 9
#define SS_PIN 10
#define BlueLED 7
#define GreenLED 8
#define RedLED 2
#define Buzzer 5
#define SensorPalmas 4
#define Rele 3
#define SensorAgua A0 // Sensor de nível de água

MFRC522 mfrc522(SS_PIN, RST_PIN);
String MasterTag = "93 26 4C 19";
String UIDCard = "";
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

int palmas = 0;
bool contaPalma = true;
unsigned long tempo = 0;
bool luzLigada = false; // Estado da lâmpada

void restartSistema() {
  digitalWrite(BlueLED, HIGH);
  digitalWrite(RedLED, LOW);
  digitalWrite(GreenLED, LOW);
  noTone(Buzzer);
  servo.write(10);
  lcd.clear();
  lcd.print("Controle Acesso");
  lcd.setCursor(0, 1);
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  servo.attach(6);
  pinMode(GreenLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(SensorPalmas, INPUT);
  pinMode(SensorAgua, INPUT);
  pinMode(Rele, OUTPUT);
  digitalWrite(Rele, HIGH);
  restartSistema();
}

void loop() {
  if (getUID()) {
    Serial.print("UID: ");
    Serial.println(UIDCard);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Permissao");
    lcd.setCursor(0, 1);

    if (UIDCard == MasterTag) {
      lcd.print("Acesso Concedido");
      digitalWrite(GreenLED, HIGH);
      digitalWrite(BlueLED, LOW);
      digitalWrite(RedLED, LOW);
      servo.write(100);
      for (int i = 0; i < 2; i++) {
        tone(Buzzer, 2000);
        delay(250);
        noTone(Buzzer);
        delay(250);
      }
      monitorarPalmas();
      delay(500); // Espera para estabilizar antes de verificar o nível de água
    } else {
      lcd.print("Acesso Negado");
      digitalWrite(BlueLED, LOW);
      digitalWrite(GreenLED, LOW);
      digitalWrite(RedLED, HIGH);
      tone(Buzzer, 2000);
      for (int i = 0; i < 3; i++) {
        digitalWrite(RedLED, LOW);
        delay(300);
        digitalWrite(RedLED, HIGH);
        delay(300);
      }
      noTone(Buzzer);
      delay(1000);
    }
    restartSistema();
  }
  verificarAgua();
}

boolean getUID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return false;
  UIDCard = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    char buf[3];
    sprintf(buf, "%02X", mfrc522.uid.uidByte[i]);
    UIDCard += String(buf) + " ";
  }
  UIDCard.trim();
  Serial.println("Cartão detectado: " + UIDCard);
  mfrc522.PICC_HaltA();
  return true;
}

void monitorarPalmas() {
  unsigned long tempoInicio = millis();
  palmas = 0;
  bool estadoAnterior = HIGH; // Assume que o sensor inicia sem detectar som

  while (millis() - tempoInicio < 10000) { // 10 segundos para detectar palmas
    bool estadoAtual = digitalRead(SensorPalmas);

    if (estadoAnterior == HIGH && estadoAtual == LOW) { // Detecta transição
      palmas++;
      Serial.print("Palmas detectadas: ");
      Serial.println(palmas);
      delay(800); // Pequeno delay para evitar leituras múltiplas
    }
    estadoAnterior = estadoAtual;

    if (palmas == 1 && !luzLigada) {
      Serial.println("Lâmpada LIGADA!");
      digitalWrite(Rele, LOW);
      luzLigada = true;
    } else if (palmas == 2 && luzLigada) {
      Serial.println("Lâmpada DESLIGADA!");
      digitalWrite(Rele, HIGH);
      luzLigada = false;
      palmas = 0;
    }
  }
}

int leituraAguaSuavizada() {
  long soma = 0;
  for (int i = 0; i < 10; i++) {
    soma += analogRead(SensorAgua);
    delay(10); // Pequeno atraso entre as leituras
  }
  return soma / 10;
}

void verificarAgua() {
  int valorSensor = leituraAguaSuavizada();
  int nivelAgua = map(valorSensor, 0, 500, 0, 100);
  Serial.print("Nível de água: ");
  Serial.print(nivelAgua);
  Serial.println("%");

  lcd.setCursor(0, 1);
  lcd.print("Nivel: ");
  lcd.print(nivelAgua);
  lcd.print("%   ");

  if (nivelAgua >= 30) {
    digitalWrite(RedLED, HIGH);
    for (int i = 0; i < 5; i++) {
      tone(Buzzer, 300, 300);
      delay(500);
      tone(Buzzer, 100, 300);
      delay(500);
    }
    digitalWrite(RedLED, LOW);
  }
  delay(1000);
}
