# ISA_Drive
>**Opis z założeniem, że to są przerwania czasowe a nie funkcyjne(jak to jest w Arduino musimy jeszcze sprawdzić):**<br>
Auto jedzie cały czas prosto. Uruchamiamy przerwania. W przerwaniu sprawdzamy obecność przeszkody z przodu autka.
Gdy nie ma przeszkody wychodzimy z przerwania.
Gdy mamy przeszkodę to autko skręca w prawo, aby ominąć ścianę przeszkody. Jednocześnie próbujmy ten kierunek ustawić najbliżej kierunku właściwego, tzn. przestań
krecić w prawo, gdy prawy bok przestanie się zbliżać do ściany (próbujemy ustawić autko równolegle do ściany przeszkody).
Gdy nie ma przeszkody (chwilowo!) jedziemy do przodu, czekając na moment, aż ściana zacznie nam "uciekać". Wtedy próbujemy nastawić się na kolejną
(w założeniu zadania przeszkody są wielokątami wypukłymi). W ten sposób, gonimy ściany, aą uda nam się powrócić autem bezpiecznie do kierunku jazdy 
założonego. Dopiero wtedy kończymy przerwanie.
Te wymysły, sprowadzają się do przełożenia większej częci programu z przykadu 1) do procedury obsługi przerwania.<br>
**WNIOSEK:** funkcje, które wypisałem raczej będą potrzebne, więc może je napiszmy. Zapraszam do podziału pracy, przy następnej okazji przetestujemy każdą funkcję i spróbujemy
 razem je złożyć w program.
```c
Compass functions:
	int[] readCompass();
    //zamiast tablic to może sobie zwracajmy 
    typedef struct coordinates
    {
        int x;
        int y;
        int z;
    };
	


Movement functions:
	void driveForward(int level);                       //w sumie mamy
	void turnLeft(int level);                           //chusteczkowe, ale też jest
	void turnRight(int level);                          //chusteczkowe, ale też jest
	void setDirection(int[] readFromCompass);           //tutaj też coordinates raczej
	void ommitObstacleBySide(enum side);

Proximity functions:
	int readProximityBySide(enum side);                 //tą funkcję to właściwie mamy zaszytą 
                                                            //w funkjci isObstacleCloseBySide
	bool isObstacleCloseBySide(enum side, int level);   //w sumie mamy
	bool isObstacleOmittedBySide(enum side, int level);

Pomysły:
1) Program iteracyjny:
	loop{
	
	while(!isObstacleCloseBySide(front)){
		driveForward();
		}
	
	while(!isObstacleOmittedBySide(front)){
		 ommitObstacleBySide(front);
		}
	
	setDirection(compass);
	}

2) Program z przerwaniami:
	loop{driveForward();}

```