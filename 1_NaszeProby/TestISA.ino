#include "ISAMobile.h"

QMC5883 qmc;

void SetPowerLevel(EngineSelector side, int level)
{
	level = constrain(level, -255, 255);

	if (side == EngineSelector::Left)
	{
		if (level > 0)
		{
			// do przodu
			digitalWrite(LEFT_IN1, false);
			digitalWrite(LEFT_IN2, true);
			analogWrite(LEFT_PWM, level);
		}
		else if (level < 0)
		{
			// do ty³u
			digitalWrite(LEFT_IN1, true);
			digitalWrite(LEFT_IN2, false);
			analogWrite(LEFT_PWM, -level);
		}
		else
		{
			// stop (soft)
			digitalWrite(LEFT_IN1, true);
			digitalWrite(LEFT_IN2, true);
			analogWrite(LEFT_PWM, 0);
		}
	}

	if (side == EngineSelector::Right)
	{
		if (level > 0)
		{
			// do przodu
			digitalWrite(RIGHT_IN1, true);
			digitalWrite(RIGHT_IN2, false);
			analogWrite(RIGHT_PWM, level);
		}
		else if (level < 0)
		{
			// do ty³u
			digitalWrite(RIGHT_IN1, false);
			digitalWrite(RIGHT_IN2, true);
			analogWrite(RIGHT_PWM, -level);
		}
		else
		{
			// stop (soft)
			digitalWrite(RIGHT_IN1, true);
			digitalWrite(RIGHT_IN2, true);
			analogWrite(RIGHT_PWM, 0);
		}
	}
}

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

	SetPowerLevel(EngineSelector::Left, 0);
	SetPowerLevel(EngineSelector::Right, 0);

	// Wejœcia enkoderowe
	pinMode(ENCODER_LEFT, INPUT);
	pinMode(ENCODER_RIGHT, INPUT);

	Serial.begin(9600);
	Serial.print("Test... ");

	Wire.begin();
	qmc.init();

	Serial1.begin(9600); // HC06
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


void cmd_proximity(const char *msg, UltraSoundSensor sensor)
{
	if (sensor == UltraSoundSensor::All)
	{

		char buffer[128];

		int d[4][5] = {0};
		int sum[4] = {0};
		int id[4] = {0};
		int dist[4] = {0};

		while (Serial.available() == 0)
		{
			for (int sens = (int)UltraSoundSensor::Front; sens <= (int)UltraSoundSensor::Right; sens++)
			{
				dist[sens] = measureSoundSpeed(
					ultrasound_trigger_pin[sens],
					ultrasound_echo_pin[sens]);

				// œrednia krocz¹ca
				sum[sens] -= d[sens][id[sens]];
				sum[sens] += d[sens][id[sens]] = dist[sens];
				id[sens] = (id[sens] + 1) % 5;
				dist[sens] = sum[sens] / 5;
			}
			sprintf(buffer, "\nFRONT: %4dcm; BACK: %4dcm; LEFT: %4dcm; RIGHT: %4dcm; ",
					dist[(int)UltraSoundSensor::Front],
					dist[(int)UltraSoundSensor::Back],
					dist[(int)UltraSoundSensor::Left],
					dist[(int)UltraSoundSensor::Right]);
			Serial.print(buffer);
		}
	}
	else
	{

		char buffer[64];
		int d[5] = {};
		int sum = 0;
		int id = 0;

		while (Serial.available() == 0)
		{
			int dist = measureSoundSpeed(
				ultrasound_trigger_pin[(int)sensor],
				ultrasound_echo_pin[(int)sensor]);

			// œrednia krocz¹ca
			sum -= d[id];
			sum += d[id] = dist;
			id = (id + 1) % 5;
			dist = sum / 5;

			sprintf(buffer, "\n%s: %0dcm", msg, dist);
			Serial.print(buffer);
		}
	}

	while (Serial.available())
		Serial.read();
}

void cmd_qmc(void)
{
	char buffer[64];

	qmc.reset();
	while (Serial.available() == 0)
	{
		qmc.measure();
		int16_t x = qmc.getX();
		int16_t y = qmc.getY();
		int16_t z = qmc.getZ();

		sprintf(buffer, "\n X=%5d Y=%5d Z=%5d", x, y, z);
		Serial.print(buffer);
	}

	while (Serial.available())
		Serial.read();
}

void cmd_bluetooth(void)
{
	Serial.println("### HC06: Tryb komunikacji z modu³em HC06. Aby wyjœæ, wpisz \"++++++\"...");
	Serial.println("### Protokó³: Modu³ analizuje czas otrzymywania danych; polecenie musi");
	Serial.println("###           koñczyæ siê krótk¹ przerw¹ (ok. 500ms) BEZ znaku nowej linii");
	Serial.println("### Testy:    Wyœlij AT (dok³adnie dwa bajty)");
	Serial.println("### Klient:   Wykorzystaj apkê androidow¹ (np. Serial Bluetooth Terminal");
	Serial.println("### Modu³:    Miganie diod oznacza brak sparowanego urz¹dzenia; pin=1234");

	Serial.print("\n> ");

	int plus_counter = 0;
	while (true)
	{
		int b = 0;
		if (Serial.available())
		{

			b = Serial.read();

			if (b == '+')
			{
				plus_counter++;
				if (plus_counter >= 6)
					break; // wyjdŸ na 6 plusów
			}

			if (b != '\n')		  // HC06 nie lubi znaków nowej linii ;)
				Serial1.write(b); // wyœlij do hc06

			Serial.write(b); // echo lokalne
		}

		if (Serial1.available())
		{
			int b = Serial1.read();
			Serial.write(b);
		}
	}

	Serial.println("HC06: Koniec.");
}

void cmd_serial0(void)
{
	Serial.println("### Komunikacja po porcie szeregowym Serial0 Aby wyjœæ, wpisz \"++++++\"...");
	Serial.println("### Parametry ³acza: 9600bps, 8 bitów danych, brak parzystoœci, 1 bit stopu (9600,8N1)");

	Serial.print("\n> ");

	int plus_counter = 0;
	while (true)
	{
		int b = 0;
		if (Serial.available())
		{

			b = Serial.read();

			if (b == '+')
			{
				plus_counter++;
				if (plus_counter >= 6)
					break; // wyjdŸ na 6 plusów
			}

			Serial1.write(b); // wyœlij do urz¹dzenia zewnêtrznego (np. raspberry pi)
			Serial.write(b);  // echo lokalne
		}

		if (Serial1.available())
		{
			int b = Serial1.read();
			Serial.write(b);
		}
	}

	Serial.println("Serial0: Koniec.");
}

void cmd_encoders(void)
{
	pinMode(ENCODER_LEFT, INPUT);
	pinMode(ENCODER_RIGHT, INPUT);

	char buffer[] = {'\n', 'L', '-', 'R', '-', '\x0'}; // 2, 4

	while (Serial.available() == 0)
	{
		buffer[2] = '0' + digitalRead(50);
		buffer[4] = '0' + digitalRead(51);

		Serial.print(buffer);
	}

	while (Serial.available())
		Serial.read();
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

void loop(void)
{

	if (isObstacleClose(UltraSoundSensor::All, 10))
		Serial.println("blisko");
	else
		Serial.println("daleko");
	delay(1000);
}
