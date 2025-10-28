#include <Servo.h>
#include <math.h> // Necessário para cos() e sin()

// Define os pinos Trig e Echo do sensor ultrassônico
const int trigPin = 10;
const int echoPin = 11;

// Variáveis para a duração e a distância
long duration;
float distance; // Use float para mais precisão

Servo myServo; // Cria um objeto servo para controlar o servo motor

void setup() {
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
  myServo.attach(12); // Define em qual pino o Servo Motor está conectado

  Serial.println("Angulo (°)\tDistancia (cm)\tX (cm)\tY (cm)");
  Serial.println("------------------------------------------------");
}

void loop() {
  // Varre o Servo de 15° até 165°
  for (int i = 15; i <= 165; i++) {  
    myServo.write(i);
    delay(40);
    distance = calculateDistance(); // Calcula a distância em cm
    
    // Converte ângulo para radianos
    float anguloRad = i * 3.141592 / 180.0;
    
    // Calcula coordenadas cartesianas (X e Y)
    float x = distance * cos(anguloRad);
    float y = distance * sin(anguloRad);
    
    // Mostra no monitor serial
    Serial.print("Angulo: ");
    Serial.print(i);
    Serial.print("°\t Dist: ");
    Serial.print(distance);
    Serial.print(" cm\t X: ");
    Serial.print(x, 1);
    Serial.print(" cm\t Y: ");
    Serial.print(y, 1);
    Serial.println(" cm");
  }

  // Agora varre de volta de 165° até 15°
  for (int i = 165; i >= 15; i--) {  
    myServo.write(i);
    delay(40);
    distance = calculateDistance();
    
    float anguloRad = i * 3.141592 / 180.0;
    float x = distance * cos(anguloRad);
    float y = distance * sin(anguloRad);
    
    Serial.print("Angulo: ");
    Serial.print(i);
    Serial.print("°\t Dist: ");
    Serial.print(distance);
    Serial.print(" cm\t X: ");
    Serial.print(x, 1);
    Serial.print(" cm\t Y: ");
    Serial.print(y, 1);
    Serial.println(" cm");
  }
}

// Função para calcular a distância medida pelo sensor ultrassônico
float calculateDistance() { 
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 25000); // timeout de 25ms (~4m)
  if (duration == 0) return 0; // Nenhum retorno detectado

  float distance = duration * 0.0343 / 2.0; // Distância em cm
  return distance;
}
