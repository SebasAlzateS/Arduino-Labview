//***************************************************//
//***************************************************//
//*****   Recepción y transmición de datos      *****//
//*****   de velocidad de un encoder de un      *****//
//*****   motor DC hacia Simulink para          *****//
//*****   Validación de modelo matematico       *****//
//*****                                         *****//
//***** by: Sergio Andres Castaño Giraldo       *****//
//***** https://controlautomaticoeducacion.com/ *****//
//*****                                         *****//
//***************************************************//
//***************************************************//

// this library includes the ATOMIC_BLOCK macro.
#include <util/atomic.h>

#define ENCODER_A       2 // Amarillo
#define ENCODER_B       3 // Verde

// Declaración del vector global
const int delayLength = 10; // Longitud del vector, define el tiempo de delay
int pwmValues[delayLength] = {0}; // Inicializa el vector con 0

// Pines de Control Shield
const int E1Pin = 10;
const int M1Pin = 12;


//Estructura del Motor
typedef struct{
  byte enPin;
  byte directionPin;
}Motor;


//Variable Union
float velocidad;

//Creo el motor
const Motor motor = {E1Pin, M1Pin};

//Constantes de dirección del Motor
const int Forward = LOW;
const int Backward = HIGH;

//Variable global de pulsos compartida con la interrupción
volatile int pulsos = 0;
unsigned long timeold;
float resolution = 300;
//Variable Global Velocidad
int vel = 0;
int pwmValue = 128;
int pwmPor = 50;
float rpm=0;

void setup(){
  Serial.begin(57600);
  // set timer 1 divisor to  1024 for PWM frequency of 30.64 Hz
  TCCR1B = TCCR1B & B11111000 | B00000101;
  //Configura Motor
  pinMode(motor.enPin, OUTPUT);
  pinMode(motor.directionPin, OUTPUT);
  //Configurar Interrupción
  timeold = 0;
  attachInterrupt(digitalPinToInterrupt(ENCODER_A),leerEncoder,RISING);
  for(int i = 0; i < delayLength; i++) {
    pwmValues[i] = 0;
  }
}

void loop(){

  // Convertir a string
    char strVelocidad[10]; 
    String dato,dataPWM;
    int i,ini,fin;

  //********* Recibir datos por puerto serial  ***************
    if (Serial.available()){
      //leemos el dato enviado
      dato = Serial.readString(); //Ler o dado
      ini = dato.indexOf("S")+1;
      fin = dato.indexOf("$");
       // salvo en degC el caracter con el escalon
      dataPWM=dato.substring(ini, fin);
      pwmPor = dataPWM.toDouble();   // Convert character string to integers
      //Serial.print(pwmValue);
      //Serial.print(dato.substring(ini, fin));
    }
  
  pwmValue = map(pwmPor, 0, 100, 0, 255);
  //Activa el motor dirección Backward con la velocidad
  set (motor, pwmValue, false);

  //Espera un segundo para el calculo de las RPM
  if (millis() - timeold >= 1000)
  {
      //Modifica las variables de la interrupción forma atómica
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        rpm = float((60.0 * 1000.0 / resolution ) / (millis() - timeold) * pulsos);
        timeold = millis();
        pulsos = 0;
         velocidad = rpm * 1; //rad/s
         dtostrf(velocidad, 6, 2, strVelocidad); 
         
      }
   }

   for(i=0;i<3;i++){
     // Enviar el string
     Serial.print("Y"); // Inicio de trama
     Serial.print(velocidad); // Enviar valor Velocidad
     Serial.print("U"); // Variable manipulada
     Serial.print(pwmPor);
     Serial.print("#"); // Fin de trama
     Serial.setTimeout(100);
   }
   delay(100);
}

//Función para dirección y velocidad del Motor
void setMotor(const Motor motor, int vel, bool dir){
  // Desplazar los valores en el vector
  for(int i = delayLength - 1; i > 0; i--) {
    pwmValues[i] = pwmValues[i - 1];
  }

  // Insertar el nuevo valor en la primera posición
  pwmValues[0] = vel;

  // Aplicar el valor en la última posición del vector al motor
  analogWrite(motor.enPin, pwmValues[delayLength - 1]);
  
  if(dir)
    digitalWrite(motor.directionPin, Forward);
  else
    digitalWrite(motor.directionPin, Backward);
}

//Función para la lectura del encoder
void leerEncoder(){
    pulsos++; //Incrementa una revolución
}
