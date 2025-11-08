#include <Wire.h>
#include <Adafruit_MLX90614.h>
#define NOTE_F  349

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Pinos
const int trig = 7;
const int echo = 6;
const int presenca = 8;
const int temperatura = 9;
const int buzzer = 10;

// Thresholds
const float dist_minima = 15.0; // Distância para detectar pessoa
const float temp_febre = 37.5;  // Temperatura considerada febril (ajustável)
const float temp_minima = 20.0; // Temperatura mínima plausível para pessoa
const int leituras_ultrassom = 3;     // Reduzi para resposta mais rápida
const int leituras_temperatura = 3;   // Reduzi para resposta mais rápida

// Variáveis de estado
bool pessoa_detectada = false;
bool febre_detectada = false;
unsigned long tempo_ultima_detecao = 0;
const unsigned long timeout_detecao = 3000; // Timeout após detecção (ms)

// Intervalo entre leituras (ms)
const unsigned long delay_leitura = 200;

void setup() {
  Serial.begin(115200);
  delay(100);

  // Inicializa MLX90614
  Wire.begin();
  if (!mlx.begin()) {
    Serial.println("Erro: nao foi possivel iniciar o MLX90614.");
    while (1) delay(10);
  }

  // Configura pinos do ultrassônico
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);

  // Pinoss de saída
  pinMode(presenca, OUTPUT);
  pinMode(temperatura, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Estado inicial
  digitalWrite(presenca, LOW);
  digitalWrite(temperatura, LOW);
  noTone(buzzer);

  Serial.println("Sistema de controle de pessoas iniciado.");
}

float readUltrasonicCm() {
  long total = 0;
  int leituras_validas = 0;
  
  for (int i = 0; i < leituras_ultrassom; ++i) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    unsigned long duration = pulseIn(echo, HIGH, 30000UL);
    
    if (duration > 0) {
      float distanceCm = duration * 0.034 / 2;
      if (distanceCm > 2 && distanceCm < 400) { // Filtro para valores plausíveis
        total += (long)(distanceCm * 1000.0);
        leituras_validas++;
      }
    }
    delay(10);
  }
  
  if (leituras_validas == 0) return 999.9; // Retorna valor alto se nenhuma leitura válida
  
  float avgScaled = (float)total / leituras_validas;
  return avgScaled / 1000.0;
}

float readMLXTempC() {
  double total = 0.0;
  int leituras_validas = 0;
  
  for (int i = 0; i < leituras_temperatura; ++i) {
    double t = mlx.readObjectTempC();
    if (!isnan(t) && t > temp_minima && t < 100) { // Filtra leituras inválidas
      total += t;
      leituras_validas++;
    }
    delay(50);
  }
  
  if (leituras_validas == 0) return 0.0;
  return (float)(total / leituras_validas);
}

void loop() {
  float dist = readUltrasonicCm();
  bool dist_valida = (dist < 400.0); // Considera válida se menor que 4m
  
  Serial.print("Distancia: ");
  if (!dist_valida || dist >= 400) {
    Serial.println("Fora de alcance");
  } else {
    Serial.print(dist, 1);
    Serial.println(" cm");
  }

  // Verifica timeout da última detecção
  if (pessoa_detectada && (millis() - tempo_ultima_detecao > timeout_detecao)) {
    pessoa_detectada = false;
    febre_detectada = false;
    digitalWrite(presenca, LOW);
    digitalWrite(temperatura, LOW);
    noTone(buzzer);
    Serial.println("Timeout: Pessoa saiu da area de deteccao.");
  }

  // Detecta pessoa próxima
  if (dist_valida && dist <= dist_minima) {
    tempo_ultima_detecao = millis();
    
    if (!pessoa_detectada) {
      pessoa_detectada = true;
      Serial.println("=== PESSOA DETECTADA ===");
    }
    
    digitalWrite(presenca, HIGH);
    
    // Lê temperatura
    float tempC = readMLXTempC();
    float tempAmbient = mlx.readAmbientTempC();
    
    Serial.print("Temp. Objeto: ");
    Serial.print(tempC, 1);
    Serial.print(" C | Ambiente: ");
    Serial.print(tempAmbient, 1);
    Serial.println(" C");

    // Verifica febre
    if (tempC >= temp_minima) { // Só considera se for uma temperatura plausível
      if (tempC >= temp_febre) {
        if (!febre_detectada) {
          febre_detectada = true;
          Serial.println("*** ALERTA: FEBRE DETECTADA! ***");
        }
        digitalWrite(temperatura, LOW); // Febre = LOW (conforme sua lógica)
        tone(buzzer, NOTE_F, 500); // Buzzer por 500ms
      } else {
        febre_detectada = false;
        digitalWrite(temperatura, HIGH); // Temperatura normal = HIGH
        noTone(buzzer);
        Serial.println("Temperatura normal.");
      }
    } else {
      Serial.println("Leitura de temperatura invalida.");
      digitalWrite(temperatura, LOW);
    }
    
  } else if (dist_valida) {
    // Pessoa detectada mas fora da distância mínima
    Serial.println("Objeto detectado mas fora do alcance para temperatura.");
    digitalWrite(presenca, LOW);
  }

  // Status resumido
  Serial.print("STATUS: ");
  if (pessoa_detectada) {
    Serial.print("Pessoa presente");
    if (febre_detectada) {
      Serial.println(" - COM FEBRE!");
    } else {
      Serial.println(" - Temperatura normal");
    }
  } else {
    Serial.println("Aguardando pessoa...");
  }
  
  Serial.println("-----------------------------");
  delay(delay_leitura);
}
