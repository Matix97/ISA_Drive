#include "ISAMobile.h"

QMC5883 qmc;

typedef struct Coordinates
{//C type: short stdint.h type: int16_t Bits: 16 Sign: Signed  Range:-32,768 .. 32,767
	int16_t x;
	int16_t y;
	int16_t z;
};

//ich funkcje(prowadzacych)
int measureSoundSpeed(int trigger_pin, int echo_pin);
//nasze funkcje
void stopWheels();
void drivingForward(int level);
void drivingBack(int level);
void turnLeft(int level);  //do poprawy
void turnRight(int level); //do poprawy
bool isObstacleClose(UltraSoundSensor sensor, int a);
Coordinates readCompass();

void setup(void)
{
	// Czujniki ultradŸwiekowe
	for (int i = (int)UltraSoundSensor::__first; i <= (int)UltraSoundSensor::__last; i++)
	{
		pinMode(ultrasound_trigger_pin[i], OUTPUT);
		pinMode(ultrasound_echo_pin[i], INPUT);
		digitalWrite(ultrasound_trigger_pin[i], 0);
	}

	// Silniki
	pinMode(LEFT_PWM, OUTPUT);
	pinMode(LEFT_IN1, OUTPUT);
	pinMode(LEFT_IN2, OUTPUT);

	pinMode(RIGHT_PWM, OUTPUT);
	pinMode(RIGHT_IN1, OUTPUT);
	pinMode(RIGHT_IN2, OUTPUT);

	stopWheels();

	Serial.begin(9600);
	Serial.print("Test... ");

	Wire.begin();
	qmc.init();
}

void loop(void)
{

	if (isObstacleClose(UltraSoundSensor::All, 10))
		Serial.println("blisko");
	else
		Serial.println("daleko");
	delay(1000);
}

int measureSoundSpeed(int trigger_pin, int echo_pin)
{
	digitalWrite(trigger_pin, false);
	delayMicroseconds(2);

	digitalWrite(trigger_pin, true);
	delayMicroseconds(10);
	digitalWrite(trigger_pin, false);

	// zmierz czas przelotu fali dŸwiêkowej
	int duration = pulseIn(echo_pin, true, 50 * 1000);

	// przelicz czas na odleg³oœæ (1/2 Vsound(t=20st.C))
	int distance = (int)((float)duration * 0.03438f * 0.5f);
	return distance;
}

void stopWheels()
{
	Serial.print("STOP: ");
	Serial.println("0");
	digitalWrite(LEFT_IN1, true);
	digitalWrite(LEFT_IN2, true);
	analogWrite(LEFT_PWM, 0);
	digitalWrite(RIGHT_IN1, true);
	digitalWrite(RIGHT_IN2, true);
	analogWrite(RIGHT_PWM, 0);
}
void drivingForward(int level)
{
	level = constrain(level, -255, 255);
	Serial.print("Do przodu: ");
	Serial.println(level);
	digitalWrite(LEFT_IN1, false);
	digitalWrite(LEFT_IN2, true);
	analogWrite(LEFT_PWM, level);
	digitalWrite(RIGHT_IN1, true);
	digitalWrite(RIGHT_IN2, false);
	analogWrite(RIGHT_PWM, level);
}
void drivingBack(int level)
{

	level = constrain(level, -255, 255);
	Serial.print("Do tylu: ");
	Serial.println(level);
	digitalWrite(LEFT_IN1, true);
	digitalWrite(LEFT_IN2, false);
	analogWrite(LEFT_PWM, -level);
	digitalWrite(RIGHT_IN1, false);
	digitalWrite(RIGHT_IN2, true);
	analogWrite(RIGHT_PWM, -level);
}
void turnLeft(int level)
{
	level = constrain(level, -255, 255);
	Serial.print("W lewo: ");
	Serial.println(level);
	digitalWrite(LEFT_IN1, true);
	digitalWrite(LEFT_IN2, false);
	analogWrite(LEFT_PWM, -level);
	digitalWrite(RIGHT_IN1, true);
	digitalWrite(RIGHT_IN2, false);
	analogWrite(RIGHT_PWM, level);
}
void turnRight(int level)
{
	level = constrain(level, -255, 255);
	Serial.print("W prawo: ");
	Serial.println(level);
	digitalWrite(LEFT_IN1, false);
	digitalWrite(LEFT_IN2, true);
	analogWrite(LEFT_PWM, level);
	digitalWrite(RIGHT_IN1, false);
	digitalWrite(RIGHT_IN2, true);
	analogWrite(RIGHT_PWM, -level);
}

bool isObstacleClose(UltraSoundSensor sensor, int a)
{
	char buffer[128];

	int d[4][5] = {0};
	int sum[4] = {0};
	int id[4] = {0};
	int dist[4] = {0};

	for (int i = 0; i < 5; i++)
	{
		for (int sens = (int)UltraSoundSensor::Front; sens <= (int)UltraSoundSensor::Right; sens++)
		{
			dist[sens] = measureSoundSpeed(
				ultrasound_trigger_pin[sens],
				ultrasound_echo_pin[sens]);

			sum[sens] -= d[sens][id[sens]];
			sum[sens] += d[sens][id[sens]] = dist[sens];
			id[sens] = (id[sens] + 1) % 5;
			dist[sens] = sum[sens] / 5;
		}
	}

	sprintf(buffer, "\nFRONT: %4dcm; BACK: %4dcm; LEFT: %4dcm; RIGHT: %4dcm; ",
			dist[(int)UltraSoundSensor::Front],
			dist[(int)UltraSoundSensor::Back],
			dist[(int)UltraSoundSensor::Left],
			dist[(int)UltraSoundSensor::Right]);
	Serial.print(buffer);
	if (a < dist[(int)sensor])
		return true;
	else
		return false;
}

Coordinates readCompass()
{
	Coordinates tym;
	qmc.reset();
	qmc.measure();
	tym.x = qmc.getX();
	tym.y = qmc.getY();
	tym.z = qmc.getZ();
	return tym;
}

int readProximityBySide(UltraSoundSensor sensor, int level)
{
    int d[5] = {};
    int sum = 0;
    int id = 0;
    
   for(int i=0; i<level; i++)
   {
     int dist = measureSoundSpeed(
        ultrasound_trigger_pin[(int)sensor],
        ultrasound_echo_pin[(int)sensor]);

      // średnia krocząca
      sum -= d[id];
      sum += d[id] = dist;
      id = (id + 1) % 5;
      dist = sum / 5;
   }
   return dist;
}
