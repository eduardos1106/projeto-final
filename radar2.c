#include <Servo.h>

const int trigPin = 10;
const int echoPin = 11;
const int servoPin = 12;

Servo pan;

// Varredura
const int MIN_ANGLE = 15;
const int MAX_ANGLE = 165;
const int FAST_STEP = 4;        // passo em graus na varredura
const unsigned long STEP_DELAY_MS = 25; // tempo entre passos (ms)

// Lógica de detecção/halt
const float DETECT_THRESHOLD_CM = 10.0; // detectar <= 10 cm -> parar
const int DETECT_CONFIRM = 3;    // quantas leituras consistentes para confirmar detecção
const int NO_DETECT_CONFIRM = 5; // leituras sem detecção para retomar varredura
const unsigned long MEASURE_INTERVAL_MS = 60; // intervalo entre leituras (ms)
const unsigned long PULSE_TIMEOUT = 15000UL; // pulseIn timeout em micros (~15ms)

int angleNow = MIN_ANGLE;
bool forward = true;
bool holdMode = false;

unsigned long lastStepTime = 0;
unsigned long lastMeasureTime = 0;

int detectCounter = 0;
int noDetectCounter = 0;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pan.attach(servoPin);
  pan.write(angleNow);
  delay(200);
  Serial.println("Radar iniciado. Threshold detect = 10 cm");
}

void loop() {
  unsigned long now = millis();

  // Medição periódica independente do movimento (evita perder leituras enquanto move)
  if (now - lastMeasureTime >= MEASURE_INTERVAL_MS) {
    lastMeasureTime = now;
    float dist = measureDistanceCm();

    // Depuração
    Serial.print("A=");
    Serial.print(angleNow);
    Serial.print(" D=");
    Serial.print(dist, 1);
    Serial.print(" cm");
    if (holdMode) Serial.print("  [HOLD]");
    Serial.println();

    // Lógica de confirmação para entrar/sair do hold
    if (dist > 0 && dist <= DETECT_THRESHOLD_CM) {
      detectCounter++;
      noDetectCounter = 0;
    } else {
      detectCounter = 0;
      if (holdMode) {
        noDetectCounter++;
      } else {
        noDetectCounter = 0;
      }
    }

    // Se detectado consistentemente, entrar em hold
    if (!holdMode && detectCounter >= DETECT_CONFIRM) {
      holdMode = true;
      Serial.println(">>> DETECTADO: entrando em HOLD (parando o servo)");
    }

    // Se em hold e sem detecções por tempo suficiente, sair do hold
    if (holdMode && noDetectCounter >= NO_DETECT_CONFIRM) {
      holdMode = false;
      detectCounter = 0;
      noDetectCounter = 0;
      Serial.println(">>> SAINDO DO HOLD: retomando varredura");
    }
  }

  // Movimento do servo (executado somente se não estiver em hold)
  if (!holdMode && (millis() - lastStepTime >= STEP_DELAY_MS)) {
    lastStepTime = millis();

    // Avança o ângulo
    if (forward) {
      angleNow += FAST_STEP;
      if (angleNow >= MAX_ANGLE) {
        angleNow = MAX_ANGLE;
        forward = false;
      }
    } else {
      angleNow -= FAST_STEP;
      if (angleNow <= MIN_ANGLE) {
        angleNow = MIN_ANGLE;
        forward = true;
      }
    }
    pan.write(angleNow);
    // pequeno atraso para servo mover um pouco (breve)
    delay(6);
  }

  // Se em hold, você pode optar por centralizar o servo no ângulo onde detectou
  // ou deixá-lo na posição atual. No código atual, o servo para onde estiver.
}

// Função para medir distância (cm) com timeout
float measureDistanceCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, PULSE_TIMEOUT);
  if (duration == 0) return -1.0; // sem retorno (timeout)

  float distCm = (duration * 0.0343f) / 2.0f; // velocidade do som ~0.0343 cm/us
  return distCm;
}
