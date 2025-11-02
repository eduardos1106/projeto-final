/*
  MLX90614 + HC-SR04 example
  - Lê distância com HC-SR04
  - Se distância < DIST_THRESHOLD_CM, lê temperatura do MLX90614
  - Faz média de N leituras para suavizar
  - Requer: Adafruit_MLX90614 library

  Wiring (UNO):
   MLX90614:
     VCC -> 5V (ou 3.3V se seu módulo exigir)
     GND -> GND
     SDA -> A4
     SCL -> A5

   HC-SR04:
     VCC -> 5V
     GND -> GND
     TRIG -> 7
     ECHO -> 6
*/

#include <Wire.h>
#include <Adafruit_MLX90614.h>
#define NOTE_F  349

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Ultrasonic pins
const int trig = 7;
const int echo = 6;
const int presenca = 8;
const int temperatura = 9;
const int buzzer = 10;

// thresholds and smoothing
const float dist_minima = 15.0; // ler temperatura só se objeto estiver mais perto que isso
const int leituras_ultrassom = 5;     // média de leituras do SR04
const int leituras_temperatura = 5;            // média de leituras do MLX90614

// intervalo entre leituras (ms)
const unsigned long delay_leitura = 500;

void setup() {
  Serial.begin(115200);
  delay(100);

  // MLX init
  Wire.begin();
  if (!mlx.begin()) {
    Serial.println("Erro: nao foi possivel iniciar o MLX90614. Verifique ligacoes.");
    while (1) delay(10);
  }
  Serial.println("MLX90614 iniciado.");//tirar essa parte depois

  // ultrasonic pins
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  digitalWrite(trig, LOW);

//pinos de saida que vao verificar no ci 7408
  pinMode(presenca, OUTPUT);
  pinMode(temperatura, OUTPUT);
  pinMode(buzzer, OUTPUT);

  Serial.println("Sistema pronto.");
}

float readUltrasonicCm() {
  // Faz leituras_ultrassom e retorna média
  long total = 0;
  for (int i = 0; i < leituras_ultrassom; ++i) {
    // envia pulso ultrassonico
    digitalWrite(trig, LOW);
    delayMicroseconds(5);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    // tempo do pulso (microsegundos)
    unsigned long duration = pulseIn(echo, HIGH, 30000UL); // timeout 30 ms -> ~5 m
    if (duration == 0) {
      // timeout -> objeto fora do alcance
      // atribuí valor grande para indicar ausência
      total += 50000; // valor muito alto, será dividido
    } else {
      // velocidade do som: ~343 m/s => distância (cm) = duration / 58
      float distanceCm = duration / 58.0;
      total += (long)(distanceCm * 1000.0); // escala para manter precisão em long
    }
    delay(10);
  }
  float avgScaled = (float)total / leituras_ultrassom;
  float avgCm = avgScaled / 1000.0;
  return avgCm;//retorna a media de todos as leituras feitas para garantir precisao
}

float readMLXTempC() {
  // faz média de leituras_temperatura
  double total = 0.0;
  for (int i = 0; i < leituras_temperatura; ++i) {
    double t = mlx.readObjectTempC();
    // Se biblioteca retornar NaN por algum motivo, ignore
    if (isnan(t)) {
      // Tente ler novamente ou trate como zero
      t = 0.0;//ver se impacta muito e tirar depois provavelmente
    }
    total += t;
    delay(50); // pequeno delay entre leituras do sensor IR
  }
  return (float)(total / leituras_temperatura);//mesma coisa que o ultrassonico - faz a media das leituras para garantir precisao
}

void loop() {
  float dist = readUltrasonicCm();
  // trata caso de timeout: valores > 1000 indicam 'sem leitura'
  bool distValida = (dist < 1000.0);
  Serial.print("Distancia (cm): ");
  if (!distValida) {
    Serial.println("fora_de_alcance");
  } else {
    Serial.println(dist, 2);
  }

  if (distValida && dist <= dist_minima) {
    digitalWrite(presenca, HIGH);//se conseguiu entrar aqui, objeto esta identificado
    float tempC = readMLXTempC();
    float tempAmbient = mlx.readAmbientTempC();
    Serial.print("Temperatura objeto (C): ");
    Serial.println(tempC, 2);
    Serial.print("Temperatura ambiente (C): ");
    Serial.println(tempAmbient, 2);

    if (tempC<30){
      digitalWrite(temperatura, HIGH);//controla 1 ou 0 dependendo da temperatura
    }

    else if (tempC>30){
      digitalWrite(temperatura, LOW);
      tone(buzzer, NOTE_F);//se ta com febre, manda 0 e liga buzzer por um breve tempo
      delay(20);
    }

  } else {
    Serial.println("Objeto fora do limite - pulando leitura de temperatura.");
    digitalWrite(presenca, LOW);//se fora do limite, volta 0
    digitalWrite(temperatura, LOW);
  }

  Serial.println("-----------------------------");
  delay(delay_leitura);
}
