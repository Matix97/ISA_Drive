#include "ISAMobile.h"
#include <stdlib.h>

QMC5883 qmc;

typedef struct Coordinates
{ //C type: short stdint.h type: int16_t Bits: 16 Sign: Signed  Range:-32,768 .. 32,767
	int16_t x;
	int16_t y;
	int16_t z;
};
Coordinates kierunek;
//ich funkcje(prowadzacych)
int measureSoundSpeed(int trigger_pin, int echo_pin);
//nasze funkcje
void breakCar();
void driveForward(int level);
void driveBack(int level);
void turnLeft(int level);  //do poprawy
void turnRight(int level); //do poprawy
bool isObstacleClose(UltraSoundSensor sensor, int a);
Coordinates readCompass();
int readProximityBySide(UltraSoundSensor sensor);
bool isObstacleCloseBySide(UltraSoundSensor sensor, int minDistance);
void setCarParrarelToObstacle(UltraSoundSensor sensor, int rotationSpeed, int rotationTime);
void ommitObstacleBySide(UltraSoundSensor sensor, int testSpeed);
void setDirection(Coordinates tym, int speed, int time);

void setup(void)
{
	// Czujniki ultrad?wiekowe
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

	breakCar();

	Serial.begin(9600);
	Serial.print("Test... ");

	Wire.begin();
	qmc.init();
	qmc.reset();
	//tak dla pewności 2 odczyty na pusto, bo po resecie często pierwszych parę pomiarów
	//zwraca -1
	readCompass();
	readCompass();
	kierunek = readCompass();
}

void loop(void)
{
	if (!isObstacleCloseBySide(UltraSoundSensor::Front, 15))
	{
		driveForward(150);
		delay(5);
	}
	else
	{
		setCarParrarelToObstacle(UltraSoundSensor::Front, 150, 30);
		ommitObstacleBySide(UltraSoundSensor::Left, 150);
	}
	setDirection(kierunek, 150, 20); //150,30
}

void setCarParrarelToObstacle(UltraSoundSensor sensor, int rotationSpeed, int rotationTime) //lewym bokiem do przszkody
{
	int frontDistance = readProximityBySide(UltraSoundSensor::Front);
	int initialFrontDistance = frontDistance;
	int leftDistance = readProximityBySide(UltraSoundSensor::Left);

	//obracaj w prawo dopóki leftDistance należy (initialFrontDistance-10,initialFrontDistance+10) i frontDistance>60
	while (!(leftDistance < initialFrontDistance + 5 && leftDistance > initialFrontDistance - 10))
	{
		do
		{
			turnRight(rotationSpeed);
			delay(rotationTime);
			breakCar();
			frontDistance = readProximityBySide(UltraSoundSensor::Front);
			leftDistance = readProximityBySide(UltraSoundSensor::Left);
		} while (!(frontDistance > 60));
	}
}

void ommitObstacleBySide(UltraSoundSensor sensor, int testSpeed)
{
	int lastDistance = readProximityBySide(sensor);

	while (lastDistance != 0 && lastDistance < 30) ///w sumie byłem zmęczony więc można przjerzeć xD
	{
		driveForward(testSpeed);
		delay(20);
		lastDistance = readProximityBySide(sensor);
	}

	breakCar();
}

Coordinates readCompass()
{
	Coordinates tym;
	for (int i = 0; i < 10; i++)
	{
		qmc.measure();
		tym.x += qmc.getX();
		tym.y += qmc.getY();
		tym.z += qmc.getZ();
	}

	tym.x = (int)tym.x / 10;
	tym.y = (int)tym.y / 10;
	tym.z = (int)tym.z / 10;
	return tym;
}

int readProximityBySide(UltraSoundSensor sensor)
{
	int d[5] = {};
	int sum = 0;
	int id = 0;
	int dist;

	for (int i = 0; i < 5; i++)
	{
		dist = measureSoundSpeed(
			ultrasound_trigger_pin[(int)sensor],
			ultrasound_echo_pin[(int)sensor]);

		// �rednia krocz�ca
		sum -= d[id];
		sum += d[id] = dist;
		id = (id + 1) % 5;
		dist = sum / 5;
	}
	return dist;
}

bool isObstacleCloseBySide(UltraSoundSensor sensor, int minDistance)
{
	if (readProximityBySide(sensor) < minDistance)
		return true;
	else
		return false;
}
void setDirection(Coordinates tym, int speed, int time)
{
	Coordinates currentCoordinates = readCompass();
	int value = 40;

	while (abs(currentCoordinates.x - tym.x) > value)
	{
		if (currentCoordinates.x > tym.x + value || currentCoordinates.x > tym.x - value)
		{
			turnLeft(speed);
			delay(time);
			breakCar();
		}
		if (currentCoordinates.x < tym.x + value || currentCoordinates.x < tym.x - value)
		{
			turnRight(speed);
			delay(time);
			breakCar();
		}
		currentCoordinates = readCompass();
	}
}

int measureSoundSpeed(int trigger_pin, int echo_pin)
{
	digitalWrite(trigger_pin, false);
	delayMicroseconds(2);

	digitalWrite(trigger_pin, true);
	delayMicroseconds(10);
	digitalWrite(trigger_pin, false);

	// zmierz czas przelotu fali d?wi?kowej
	int duration = pulseIn(echo_pin, true, 50 * 1000);

	// przelicz czas na odleg?o?? (1/2 Vsound(t=20st.C))
	int distance = (int)((float)duration * 0.03438f * 0.5f);
	return distance;
}

void breakCar()
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
void driveForward(int level)
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
void driveBack(int level)
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
	analogWrite(RIGHT_PWM, level + 50);
}
void turnRight(int level)
{
	level = constrain(level, -255, 255);
	Serial.print("W prawo: ");
	Serial.println(level);
	digitalWrite(LEFT_IN1, false);
	digitalWrite(LEFT_IN2, true);
	analogWrite(LEFT_PWM, level + 50);
	digitalWrite(RIGHT_IN1, false);
	digitalWrite(RIGHT_IN2, true);
	analogWrite(RIGHT_PWM, -level);
}
