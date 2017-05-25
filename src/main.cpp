#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <Ultrasonic.h>

#include "../include/IMUsensor.hpp"
#include "../include/PID.hpp"
#include "../include/motormanager.hpp"

bool safe_mode = 0;             //si activé, les moteurs se coupent automatiquement après 3 secondes d'allumage
bool debug = 0;                // et ce afin d'éviter une perte de contrôle du quadricoptère sur le banc de test

void setup()
{
  pinMode(9, INPUT_PULLUP); //on configure les entrées pour pouvoir utiliser le bouton
  Serial.begin(9600);
}


void loop()
{
  unsigned long millis_at_last_print = millis();
  if(debug)
  {
    while(!Serial); //on attends que le port série soit ouvert pour commencer les calculs
  }

  bool motor_started = 0;
  unsigned long millis_at_motor_start = 0;

  Ultrasonic ultrasonic(6,5);      //objet pour contrôler le capteur ultrason
  IMUsensor mpu;                  //objet pour récupérer les valeurs de l'IMU et calculer une orientation absolue
  PID pid;                        //objet qui gère le calcul des directives pour les moteurs
  MotorManager motors;            //objet qui gère le calcul des valeurs par moteur, et s'occupe de les contrôler

  mpu.calibrateSensors();
  delay(1000);

  while(true)
  {
    mpu.calcAbsoluteOrientation(0.97);
    mpu.actualizeSensorData();

    if( !digitalRead(9) == LOW )
    {
      pid.reset();
      motors.stop();
      if(!safe_mode)
        motor_started = 0;
    }
    else
    {
      if(!motor_started)
      {
        motor_started = 1;
        millis_at_motor_start = millis();
        motors.startMotors();
      }

      //calcul du PID avec les valeurs de l'IMU
      pid.calcCommand(mpu.getX(), mpu.getY(), mpu.getZ(), ultrasonic.Ranging(CM) - 5, mpu.getAngularSpeedX(), mpu.getAngularSpeedY(), mpu.getAngularSpeedZ(), 0, 0, 0, 5);

      motors.command( pid.getCommandX(), pid.getCommandY(), pid.getCommandZ(), pid.getCommandH() ); //commande des moteurs avec les valeurs données par le PID

    }

    if(millis() - millis_at_last_print > 60)
    {
      Serial.print( motors.getMotorValue(0) );  Serial.print("\t");
      Serial.print( motors.getMotorValue(1) );  Serial.print("\t");
      Serial.print( motors.getMotorValue(2) );  Serial.print("\t");
      Serial.print( motors.getMotorValue(3) ); Serial.print("\t|\t");
      Serial.print( mpu.getX(), 2); Serial.print("\t");
      Serial.print( mpu.getY(), 2); Serial.print("\t");
      Serial.print( mpu.getZ(), 2 ); Serial.print("\t\n");



    //  Serial.print("command H : ") ; Serial.print( pid.getCommandH(), 2 ) ; Serial.print("\t\tsum error h :\t"); Serial.println( pid.getSumErrorH(), 2 );
      millis_at_last_print = millis();
    }

    if ( safe_mode && millis() - millis_at_motor_start > 3000 && motor_started )
    {
      motors.stop();
    }
  }
}
