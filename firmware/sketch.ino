// =====================================================================
// Global Solution 2026/1 - FIAP - Disruptive Architectures
// PROJETO HYDRATA - Integração Completa: Sensores, LCD e MQTT/TLS
// =====================================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ---------- PINAGEM ----------
#define DHTPIN 15
#define DHTTYPE DHT22
#define LDRPIN 34
#define LED_BOMBA 2    // LED Azul
#define LED_ALERTA 4   // LED Vermelho

// ---------- CREDENCIAIS DE REDE E MQTT ----------
const char* ssid = "Wokwi-GUEST";
const char* password = ""; // Sem senha no simulador Wokwi

const char* mqtt_server = "2585265c020e418ba116d2601017a480.s1.eu.hivemq.cloud";
const char* mqtt_user = "vitalis";
const char* mqtt_pass = "Vitalis1"; 
const int mqtt_port = 8883; // Porta TLS Criptografada

// Tópico para publicação dos dados em JSON
const char* mqtt_topic_clima = "FIAP/HYDRATA/CLIMA";       // Tópico 1 (JSON de Temp/Umid)
const char* mqtt_topic_luminosidade = "FIAP/HYDRATA/LUZ";  // Tópico 2 (JSON do LDR)
const char* mqtt_topic_status = "FIAP/HYDRATA/STATUS";     // Tópico 3 (JSON dos Atuadores)

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ---------- VARIÁVEIS GLOBAIS DE TIMING ----------
unsigned long tempoAnteriorCarrossel = 0;
unsigned long tempoAnteriorMQTT = 0;

const long intervaloCarrossel = 3000; // Alterna o LCD a cada 3 segundos
const long intervaloMQTT = 5000;      // Envia dados ao broker a cada 5 segundos

int telaAtual = 0;

// Variáveis de controle de estado
bool bombaLigada = false;
bool alertaLigado = false;

void setup_wifi() {
  delay(10);
  Serial.print("\nConectando ao WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    lcd.setCursor(0, 0);
    lcd.print("Conectando WiFi ");
  }

  Serial.println("\nWiFi Conectado com sucesso!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: OK");
  delay(1000);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    lcd.setCursor(0, 1);
    lcd.print("MQTT: Conectando");

    String clientId = "ESP32Client-HyDrata-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" Conectado ao HiveMQ!");
      lcd.setCursor(0, 1);
      lcd.print("MQTT: Conectado ");
      delay(1000);
      lcd.clear();
    } else {
      Serial.printf(" Falhou, rc=%d. Tentando novamente em 2s\n", client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando Sistema Completo HyDrata...");

  pinMode(LED_BOMBA, OUTPUT);
  pinMode(LED_ALERTA, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("PROJETO HYDRATA");
  lcd.setCursor(0, 1);
  lcd.print("   FIAP 2026   ");
  delay(2000);
  lcd.clear();

  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float umidadeAr = dht.readHumidity();
  float temperatura = dht.readTemperature();
  int leituraLDR = analogRead(LDRPIN);
  
  float luminosidade = map(leituraLDR, 0, 4095, 0, 100);


  if (isnan(umidadeAr) || isnan(temperatura)) {
    Serial.println("Erro crítico: Falha na leitura do sensor DHT22!");
    return;
  }

  // 2. REGRAS DE NEGÓCIO LOCAIS (Gatilhos de Atuadores)
  // Regra da Bomba de Irrigação (LED Azul)
  if (umidadeAr < 40.0 && luminosidade > 70) {
    bombaLigada = true;
    digitalWrite(LED_BOMBA, HIGH);
  } else {
    bombaLigada = false;
    digitalWrite(LED_BOMBA, LOW);
  }

  // Regra de Alerta Crítico Climático (LED Vermelho)
  if (temperatura >= 38.0) {
    alertaLigado = true;
    digitalWrite(LED_ALERTA, HIGH);
  } else {
    alertaLigado = false;
    digitalWrite(LED_ALERTA, LOW);
  }

  unsigned long tempoAtual = millis();

// 3. ENVIO DOS DADOS VIA MQTT (Executado a cada 5 segundos em 3 tópicos distintos)
  if (tempoAtual - tempoAnteriorMQTT >= intervaloMQTT) {
    tempoAnteriorMQTT = tempoAtual;

    char jsonBuffer[150];

    // Tópico 1: Dados de Microclima (DHT22)
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"temperatura\":%.1f,\"umidade_ar\":%.1f}", temperatura, umidadeAr);
    client.publish(mqtt_topic_clima, jsonBuffer);

    // Tópico 2: Dados de Radiação Solar (LDR)
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"luminosidade\":%.0f}", luminosidade);
    client.publish(mqtt_topic_luminosidade, jsonBuffer);

    // Tópico 3: Status de Operação Física (LEDs / Atuadores)
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"bomba_ativa\":%s,\"alerta_critico\":%s}", 
             bombaLigada ? "true" : "false", 
             alertaLigado ? "true" : "false");
    client.publish(mqtt_topic_status, jsonBuffer);

    Serial.println("-> Telemetria distribuída com sucesso nos 3 tópicos MQTT obrigatórios.");
  }

  // 4. MÁQUINA DE ESTADOS DO DISPLAY LCD (Carrossel a cada 3 segundos - Não bloqueante)
  if (tempoAtual - tempoAnteriorCarrossel >= intervaloCarrossel) {
    tempoAnteriorCarrossel = tempoAtual;
    telaAtual = (telaAtual + 1) % 2; 
    lcd.clear();
  }

  if (telaAtual == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatura, 1);
    lcd.print(" C");
    
    lcd.setCursor(0, 1);
    lcd.print("Umid Ar: ");
    lcd.print(umidadeAr, 1);
    lcd.print("%");
  } 
  else if (telaAtual == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Sol/Luz: ");
    lcd.print(luminosidade, 0);
    lcd.print("%");
    
    lcd.setCursor(0, 1);
    if (bombaLigada) { 
       lcd.print("Bomba: ATIVADA  ");
    } else if (alertaLigado) {
       lcd.print("[!] CALOR CRITICO");
    } else {
       lcd.print("Status: SEGURO  ");
    }
  }

  delay(50); // Delay mínimo apenas para estabilizar o ciclo do processador no ambiente simulado
}