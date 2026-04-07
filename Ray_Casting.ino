/*
 * Sistema de Telemetria GNSS para PC e SD com Geofencing (Ray Casting)
 * Perímetro: Definido diretamente no código (Hardcoded)
 * Feedback Visual: LEDs de Sinal GPS e Alerta de Perímetro
 */

#include <SparkFun_Unicore_GNSS_Arduino_Library.h>
#include <SD.h>
#include <SPI.h>

// ---------------------------------------------------------
// Configurações de Pinos
// ---------------------------------------------------------
// Pinos UART1 para o UM980 no ESP32
const int pin_UART1_RX = 16;
const int pin_UART1_TX = 17;

// Pino CS para o módulo SD Card
const int pin_SD_CS = 5; 

// Pinos dos LEDs
const int pin_LED_GPS = 2;       // LED onboard padrão na maioria dos ESP32
const int pin_LED_PERIMETRO = 4; // Ligue um LED externo aqui (Pino 4 -> Resistor -> LED -> GND)

// ---------------------------------------------------------
// Variáveis e Objetos
// ---------------------------------------------------------
UM980 myGNSS;
HardwareSerial SerialGNSS(1); // Usa a UART1 no ESP32

unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL_MS = 1000; 

// Controle de Estado para os LEDs
bool temSinalGPS = false;
bool foraDoPerimetro = false;

// Variáveis para piscar os LEDs sem usar delay()
unsigned long lastBlinkGPS = 0;
bool estadoLedGPS = false;
unsigned long lastBlinkPerimetro = 0;
bool estadoLedPerimetro = false;

// Estrutura para os vértices do polígono
struct Ponto {
  double lat;
  double lon;
};

// Dados extraídos da imagem inseridos diretamente no código
const Ponto poligonoPerimetro[] = {
  {-20.897, -46.9908},
  {-20.897, -46.9908},
  {-20.8971, -46.9908},
  {-20.8971, -46.9908},
  {-20.8971, -46.9908},
  {-20.8971, -46.9907},
  {-20.8971, -46.9907},
  {-20.8971, -46.9907},
  {-20.8971, -46.9907},
  {-20.8971, -46.9907},
  {-20.897, -46.9906},
  {-20.897, -46.9907},
  {-20.897, -46.9907},
  {-20.897, -46.9907},
  {-20.897, -46.9907},
  {-20.897, -46.9908},
  {-20.897, -46.9908}
};

const int numVerticesPerimetro = sizeof(poligonoPerimetro) / sizeof(poligonoPerimetro[0]);

// Prototipação
void output(uint8_t * buffer, size_t length);
bool pontoDentroDoPoligono(double lat, double lon, const Ponto* poligono, int numVertices);
void registrarLogSD(const char* caminho, const String& linha);

// ---------------------------------------------------------
// Setup
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while(!Serial); 

  // Inicializa os pinos dos LEDs
  pinMode(pin_LED_GPS, OUTPUT);
  pinMode(pin_LED_PERIMETRO, OUTPUT);

  // Inicialização do Cartão SD
  if (!SD.begin(pin_SD_CS)) {
    Serial.println("ERRO: Falha na inicializacao do SD Card!");
  } else {
    SD.mkdir("/LOGS_GPS");
    SD.mkdir("/ALERTAS");
    Serial.println("SD Card inicializado com sucesso.");
  }

  // Inicialização do GNSS
  SerialGNSS.begin(115200, SERIAL_8N1, pin_UART1_RX, pin_UART1_TX);

  if (myGNSS.begin(SerialGNSS, "SFE_Unicore_GNSS_Library", output) == false) {
    Serial.println("ERROR: UM980_NOT_FOUND");
    while (true);
  }

  myGNSS.disableOutput(); 
  myGNSS.setModeRoverSurvey(); 
  myGNSS.saveConfiguration(); 
  
  Serial.println("Timestamp_MS,Latitude,Longitude,Altitude_m,SIV,PosType,DentroArea");
}

// ---------------------------------------------------------
// Loop Principal
// ---------------------------------------------------------
void loop() {
  // 1. Atualiza os dados do GPS
  myGNSS.update(); 

  // 2. Lógica de Log e Checagem (Executa a cada 1 segundo)
  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();

    double lat = myGNSS.getLatitude();
    double lon = myGNSS.getLongitude();
    float alt  = myGNSS.getAltitude();
    int siv    = myGNSS.getSIV(); 
    int posType = myGNSS.getPositionType(); 

    // Verifica se pegou sinal válido
    if (lat == 0.0 && lon == 0.0) {
      temSinalGPS = false;
      foraDoPerimetro = false; // Se não tem sinal, não sabemos onde estamos
    } else {
      temSinalGPS = true;

      // Aplica o Algoritmo Ray Casting
      bool dentro = pontoDentroDoPoligono(lat, lon, poligonoPerimetro, numVerticesPerimetro);
      foraDoPerimetro = !dentro; // Atualiza status global para o LED

      // Formatação da linha CSV
      String linhaCSV = String(millis()) + "," + 
                        String(lat, 9) + "," + 
                        String(lon, 9) + "," + 
                        String(alt, 3) + "," + 
                        String(siv) + "," + 
                        String(posType) + "," + 
                        (dentro ? "SIM" : "NAO");

      Serial.println(linhaCSV);

      // Grava logs no SD
      registrarLogSD("/LOGS_GPS/gps_log.csv", linhaCSV);
      if (foraDoPerimetro) {
        registrarLogSD("/ALERTAS/fora_perimetro.csv", linhaCSV);
      }
    }
  }

  // 3. Controle dos LEDs (Executa continuamente de forma não-bloqueante)
  
  // ---> LED do GPS
  if (!temSinalGPS) {
    // Pisca a cada 500ms enquanto busca sinal
    if (millis() - lastBlinkGPS >= 500) {
      lastBlinkGPS = millis();
      estadoLedGPS = !estadoLedGPS;
      digitalWrite(pin_LED_GPS, estadoLedGPS);
    }
  } else {
    // Parar de piscar e ficar ACESO quando acha o sinal
    digitalWrite(pin_LED_GPS, HIGH);
  }

  // ---> LED do Perímetro
  if (foraDoPerimetro) {
    // Pisca rapidamente (a cada 150ms) quando está FORA do perímetro
    if (millis() - lastBlinkPerimetro >= 150) {
      lastBlinkPerimetro = millis();
      estadoLedPerimetro = !estadoLedPerimetro;
      digitalWrite(pin_LED_PERIMETRO, estadoLedPerimetro);
    }
  } else {
    // Apagado enquanto está DENTRO do perímetro (ou sem sinal)
    digitalWrite(pin_LED_PERIMETRO, LOW);
  }
}

// ---------------------------------------------------------
// Implementação do Algoritmo Ray Casting
// ---------------------------------------------------------
bool pontoDentroDoPoligono(double lat, double lon, const Ponto* poligono, int numVertices) {
  bool dentro = false;
  if (numVertices < 3) return false; 

  int j = numVertices - 1;
  for (int i = 0; i < numVertices; i++) {
    double xi = poligono[i].lon, yi = poligono[i].lat;
    double xj = poligono[j].lon, yj = poligono[j].lat;

    bool cruzamento = ((yi > lat) != (yj > lat)) &&
                      (lon < (xj - xi) * (lat - yi) / (yj - yi) + xi);

    if (cruzamento) dentro = !dentro;
    j = i;
  }
  return dentro;
}

// ---------------------------------------------------------
// Funções Auxiliares (SD e Buffer)
// ---------------------------------------------------------
void registrarLogSD(const char* caminho, const String& linha) {
  File file = SD.open(caminho, FILE_APPEND);
  if (file) {
    file.println(linha);
    file.close();
  }
}

void output(uint8_t * buffer, size_t length) {
  // Função vazia propositalmente
}