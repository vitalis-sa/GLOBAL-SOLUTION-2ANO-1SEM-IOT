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
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
// MAC Address Fixo do Dispositivo
const String MAC_ADDRESS = "AA:BB:CC:DD:EE:FF"; 

// Strings dos Tópicos dinâmicos (serão preenchidos com o MAC Address)
String topic_clima;
String topic_luminosidade;
String topic_status;

// ---------- INICIALIZAÇÃO DE OBJETOS ----------
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ---------- VARIÁVEIS GLOBAIS DE TIMING ----------
unsigned long tempoAnteriorCarrossel = 0;
unsigned long tempoAnteriorMQTT = 0;
unsigned long tempoAnteriorOpenMeteo = 0;

const long intervaloCarrossel = 3000; // Alterna o LCD a cada 3 segundos
const long intervaloMQTT = 3000;      // Envia dados ao broker a cada 5 segundos
const long intervaloOpenMeteo = 10000; // Consulta a API externa a cada 10 segundos

float precipitacaoExterna = 0.0;
float temperaturaExterna = 0.0;

int telaAtual = 0;

// Variáveis de controle de estado
bool bombaLigada = false;
bool alertaLigado = false;

// ---------- FUNÇÃO SETUP WIFI ----------
void setup_wifi() {
  delay(10);
  Serial.print("\nConectando ao WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Feedback visual rápido no LCD durante a conexão
    lcd.setCursor(0, 0);
    lcd.print("Conectando WiFi ");
  }

  Serial.println("\nWiFi Conectado com sucesso!");
  
  // Constroi os tópicos baseados no MAC Fixo
  topic_clima = "FIAP/HYDRATA/" + MAC_ADDRESS + "/CLIMA";
  topic_luminosidade = "FIAP/HYDRATA/" + MAC_ADDRESS + "/LUZ";
  topic_status = "FIAP/HYDRATA/" + MAC_ADDRESS + "/STATUS";
  Serial.println("MAC Address Fixo: " + MAC_ADDRESS);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: OK");
  delay(1000);
}

// ---------- FUNÇÃO RECONECTAR MQTT ----------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    lcd.setCursor(0, 1);
    lcd.print("MQTT: Conectando");

    // Gerando um ID de cliente único para evitar conflitos de quedas no Broker
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

  // Configuração dos Pinos dos Atuadores
  pinMode(LED_BOMBA, OUTPUT);
  pinMode(LED_ALERTA, OUTPUT);

  // Inicializa o Sensor DHT e Display LCD
  dht.begin();
  lcd.init();
  lcd.backlight();
  
  // Tela inicial de Boot
  lcd.setCursor(0, 0);
  lcd.print("PROJETO HYDRATA");
  lcd.setCursor(0, 1);
  lcd.print("   FIAP 2026   ");
  delay(2000);
  lcd.clear();

  // Configurações de Rede
  setup_wifi();
  espClient.setInsecure(); // Permite conexão TLS na nuvem HiveMQ sem validação de certificado local estático
  client.setServer(mqtt_server, mqtt_port);

  client.setKeepAlive(60);
}

void loop() {
  // Garante conexões ativas antes de processar
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 1. LEITURA CONTÍNUA DOS SENSORES
  float umidadeAr = dht.readHumidity();
  float temperatura = dht.readTemperature();
  int leituraLDR = analogRead(LDRPIN);
  
  // Mapeamento do LDR para escala percentual (0 a 100%)
  float luminosidade = map(leituraLDR, 0, 4095, 0, 100);

  // Tratamento de segurança contra leituras corrompidas
  if (isnan(umidadeAr) || isnan(temperatura)) {
    Serial.println("Erro crítico: Falha na leitura do sensor DHT22!");
    return;
  }

  unsigned long tempoAtual = millis();

  // 1.5 DADOS EXTERNOS OPENMETEO
  if (tempoAtual - tempoAnteriorOpenMeteo >= intervaloOpenMeteo) {
    tempoAnteriorOpenMeteo = tempoAtual;
    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "https://api.open-meteo.com/v1/forecast?latitude=-23.5612&longitude=-46.6541&current=temperature_2m,precipitation";
      http.begin(url);
      int httpCode = http.GET();
      if(httpCode > 0) {
        String payload = http.getString();
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if(!error) {
          precipitacaoExterna = doc["current"]["precipitation"];
          temperaturaExterna = doc["current"]["temperature_2m"];
          Serial.printf("OpenMeteo API -> Temp Externa: %.1fC | Chuva: %.1fmm\n", temperaturaExterna, precipitacaoExterna);
        }
      }
      http.end();
    }
  }

  // 2. REGRAS DE NEGÓCIO LOCAIS (Gatilhos de Atuadores)
  // Regra da Bomba de Irrigação (LED Azul) com peso de chuva externa
  if (umidadeAr < 40.0 && luminosidade > 70 && precipitacaoExterna == 0.0) {
    bombaLigada = true;
    digitalWrite(LED_BOMBA, HIGH);
  } else {
    bombaLigada = false;
    digitalWrite(LED_BOMBA, LOW);
  }

  // Regra de Alerta Crítico Climático (LED Vermelho)
  if (temperatura >= 38.0 || temperaturaExterna >= 38.0 || umidadeAr < 30.0) {
    alertaLigado = true;
    digitalWrite(LED_ALERTA, HIGH);
  } else {
    alertaLigado = false;
    digitalWrite(LED_ALERTA, LOW);
  }

// 3. ENVIO DOS DADOS VIA MQTT (Executado a cada 5 segundos em 3 tópicos distintos)
  if (tempoAtual - tempoAnteriorMQTT >= intervaloMQTT) {
    tempoAnteriorMQTT = tempoAtual;

    char jsonBuffer[150];

    // Tópico 1: Dados de Microclima (DHT22)
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"temperatura\":%.1f,\"umidade_ar\":%.1f}", temperatura, umidadeAr);
    client.publish(topic_clima.c_str(), jsonBuffer);

    // Tópico 2: Dados de Radiação Solar (LDR)
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"luminosidade\":%.0f}", luminosidade);
    client.publish(topic_luminosidade.c_str(), jsonBuffer);

    // Tópico 3: Status de Operação Física (LEDs / Atuadores)
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"bomba_ativa\":%s,\"alerta_critico\":%s}", 
             bombaLigada ? "true" : "false", 
             alertaLigado ? "true" : "false");
    client.publish(topic_status.c_str(), jsonBuffer);

    Serial.println("-> Telemetria distribuída com sucesso nos 3 tópicos MQTT obrigatórios.");
  }

  // 4. MÁQUINA DE ESTADOS DO DISPLAY LCD (Carrossel a cada 3 segundos - Não bloqueante)
  if (tempoAtual - tempoAnteriorCarrossel >= intervaloCarrossel) {
    tempoAnteriorCarrossel = tempoAtual;
    telaAtual = (telaAtual + 1) % 2; 
    lcd.clear();
  }

  // Renderização das telas correspondentes
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
