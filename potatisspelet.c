#include <stdio.h>
#include <math.h>
#include <curses.h>
#include <time.h>
#include <stdlib.h>
#include "funktioner.h"
//#include <conio.h>
#define ANTAL_POTATIS 15 //<--- Anger hur många potatisar som ska genereras på spelplanen
#define ANTAL_MONSTER 5
#define TEXTFONSTER_LINJER 10
#define MAINFONSTER_LINJER 18
#define MAINFONSTER_KOLUMNER 50
#define MAINFONSTER_YSTART 1
#define MAINFONSTER_XSTART 22
#define SLEMTROLL 1
#define KODSKELLET 2
#define SKADA 1
#define HP 2
#define SPELNAMN "POTATIS-DUNGEON"
#define VERSION "v160925"
#define LIN "--------------------------------------"

/*
FIXAT:
2016-09-24: Uppplockning av potatisar
2016-09-24: Slumpmässig potatisgenerering
2016-09-24: Spelaren kan inte gå utanför spelplanen
2016-09-24: Textruta med scrollande info-text
2016-09-24: Skrivit macro-definitioner för flera konstanter
2016-09-25: Fixat textrutan, så den uppdaterar korrekt utan flimmer
2016-09-25: Minskat spelytan, annars fungerade inte på min laptop med lägre upplösning
2016-09-25: Börjat arbeta med fiender, de har initialt en simpel AI och jagar spelaren


ATT GÖRA:
----PRIO---- BYT UT JOBBIGA GREJER MOT STRUCT VARIABLER:
	
	struct Monster{
		int id;
		char bokstav[2];
		char namn[15];
		int hp;
		int skada;
	}Slemtroll, Kodskelett;

	struct SpelareInfo{
		int x;
		int y;
		char bokstav[2];
		int hp;
		int skada;
		char vapen[15]
		int skadaMin;
		int skadaMax;
		char rustning[15]
		int armorClass;
		int steg; <--kanske
	}Spelare;
	

--------
KISTOR: med loot, nycklar för att öppna
RANDOM LOOT DROPS
FIGHTINGSYSTEM: Turbaserat
POTATISAR: Check för random, så två inte hamnar på samma koordinater
VARIABLER: Lägg i funktioner?
FUNKTIONER: Egna filer, denna kommer snart bli stor
ACHIEVEMENTS: Det har alla spel
FÄRG?
FÅ BORT MARKÖR FRÅN SPELAREN
FÖRSTA FIENDEN: Blir D (får ej rätt typ 1/2 etc) -- tror fixat
FIXA SÅ FIENDER EJ GÅR IN I VARANDRA, OCH KROCKAR MED SPELAREN!
DÖDEN (HP<0)
*/

/*
STATISKA VARIABLER
Används mellan funktioner
*/

//mainfönster storlek och startcoordinater
static int Halsa = 10;//<-------OBS BYT MOT STRUCT!
//startposition för markör/spelare
static int x = 15, y = 15; //<----- OBS SE TILL ATT INTE FUNKTIONER ANVÄNDER x SOM PARAMETER//<-------OBS BYT MOT STRUCT!

//Potatis-postitioner FIXA SLUMPADE POSITIONER!?
static int potatis_y[ANTAL_POTATIS];
static int potatis_x[ANTAL_POTATIS];
//För att visa om potatis har blivit tagen av spelaren eller inte
static int potatis_tagen[ANTAL_POTATIS];


static int monster_y[ANTAL_MONSTER];
static int monster_x[ANTAL_MONSTER];
static int monster_typ[ANTAL_MONSTER];

/*
Startvapen "Svärd", med skadan 4. 
Tänkt att användas vid eventuella framtida fiendemöten
*/
static char* vapen = "Rostigt Svärd";//<-------OBS BYT MOT STRUCT!
static int skadaMin = 1;//<-------OBS BYT MOT STRUCT!
static int skadaMax = 4;//<-------OBS BYT MOT STRUCT!

static _Bool VisaDebug = 0;

static int stegRaknare=0;//<-------OBS BYT MOT STRUCT!
/*
Startrustning T-Shirt, med armor 0. 
Tänkt att användas vid eventuella framtida fiendemöten
*/
static char* rustning = "T-Shirt";//<-------OBS BYT MOT STRUCT!
static int armorClass = 0;//<-------OBS BYT MOT STRUCT!


/*Deklarerar de fönster som kommer användas i spelet*/
static WINDOW * MainFonster;
static WINDOW * InfoFonster;
static WINDOW * DebugFonster;
static WINDOW * TextFonster;

//Variabler för textrutan
static char *TextHistory[TEXTFONSTER_LINJER-3];


//Uppdaterar fönster
void uppdatera_fonster(void){
	box(MainFonster, ACS_VLINE, ACS_HLINE);
	box(InfoFonster, ACS_VLINE, ACS_HLINE);
	box(TextFonster, ACS_VLINE, ACS_HLINE);
	wrefresh(MainFonster);
	wrefresh(InfoFonster);
	wrefresh(TextFonster);
	if (VisaDebug){
		box(DebugFonster, ACS_VLINE, ACS_HLINE);
		wrefresh(DebugFonster);
	}
}

void radera_fonster(void){
	werase(MainFonster);
	//werase(DebugFonster);
}

//VÄNTAR secs SEKUNDER
void sov(unsigned int secs) {
    unsigned int retTime = time(0) + secs;
    while (time(0) < retTime);
}

void SpelareHP(int andring){
	Halsa += andring;
}

void info(void){
	mvwaddstr(InfoFonster, 1, 1, "Karaktärs-info:");  //Lägger till texten Info i fönster InfoFonster, i x=1, y=1 från dess origo - curses.h
	mvwprintw(InfoFonster, 3, 1, "HP: %03d", Halsa);
	mvwprintw(InfoFonster, 5, 1, "%s", vapen);
	mvwprintw(InfoFonster, 6, 1, "DMG: %d - %d", skadaMin, skadaMax);
	mvwprintw(InfoFonster, 8, 1, "%s", rustning);
	mvwprintw(InfoFonster, 9, 1, "AC: %d", armorClass);
	mvwprintw(InfoFonster, 10, 1, "--------");
	mvwprintw(InfoFonster, 11, 1, "X: %02d Y: %02d", x, y);
	mvwprintw(InfoFonster, 13, 1, "S: Slemtroll");
	mvwprintw(InfoFonster, 14, 1, "K: Kodskelett");
	mvwprintw(InfoFonster, 15, 1, "o: Potatis (+HP)");
}

void DMG(WINDOW * w){
}

//Textruta med information, scrollar
void textruta(_Bool skriv, char* text){
	char * textHallare;
	mvwprintw(TextFonster, 1, 1, "** %s %s **", SPELNAMN, VERSION); 
	mvwprintw(TextFonster, 2, 2, "%s", LIN);
	if (skriv){	//Om funktionen matas med 1
	//Arrayplatser byts till nästa, och funktionens mottagna textsträng sätts till arrayplats 1
		for(int i=TEXTFONSTER_LINJER-3-2;i>0;i--){	
			textHallare = TextHistory[i];
			TextHistory[i+1] = textHallare;
		}
		TextHistory[1] = text;
		wclear(TextFonster);
		mvwprintw(TextFonster, 1, 1, "** %s %s **", SPELNAMN, VERSION); 
		mvwprintw(TextFonster, 2, 2, "%s", LIN);
		//All text i arrayen skrivs ut, den första (senaste inkomna text får en pil, för att visa att den är den aktuella)
		for(int i=1;i<TEXTFONSTER_LINJER-3;i++){
			if (i == 1){
				mvwprintw(TextFonster, i+2, 1, "%s <------ ", TextHistory[i]);
			}
			else 
				mvwprintw(TextFonster, i+2, 1, "%s", TextHistory[i]);
		}
	}
}

/*
Funktion för utplacering av potatisar, samt check för om spelaren befinner
sig på en potatis-koordinat. Om spelaren plockar upp en potatis sätts potatisens
arraynummer i potatis_tagen till 1, och den kommer inte ritas ut i nästa fönster-
uppdatering. När en potatis plockas upp ökar spelaren hälsa, funktionen textruta
anropas för att skriva ut text.
*/
void Potatisar(void){
	for(int i =0;i<ANTAL_POTATIS;i++){
		if ((potatis_x[i] == x) && (potatis_y[i] == y) && (potatis_tagen[i] == 0)){
			SpelareHP(5); //öka hp med 5!
			textruta(1, "Potatis! +5HP!");
			potatis_tagen[i] =1;
		}
		if (potatis_tagen[i] == 0){
			mvwprintw(MainFonster, potatis_y[i], potatis_x[i], "o");
			}
	}
}

//DEBUG-INFO
void jkinfo(void){
	wclear(DebugFonster);
	if (VisaDebug){
		mvwprintw(DebugFonster, 1, 1, "Debug");
		//Potatis-koordinater
		mvwprintw(DebugFonster, 2, 1, "m1: typ %d", monster_typ[0]);
		mvwprintw(DebugFonster, 3, 1, "m2: typ %d", monster_typ[1]);
		mvwprintw(DebugFonster, 4, 1, "m3: typ %d", monster_typ[2]);
		mvwprintw(DebugFonster, 5, 1, "m4: typ %d", monster_typ[3]);
		mvwprintw(DebugFonster, 6, 1, "m5: typ %d", monster_typ[4]);
		//spelar-koordinater
		mvwprintw(DebugFonster, 7, 1, "x: %02d", x);//Skriver ut nuvarande x-koordinat
		mvwprintw(DebugFonster, 8, 1, "y: %02d", y);//Skriver ut nuvarande y-koordinat
		mvwprintw(DebugFonster, 9, 1, "Steg: %02d", stegRaknare);
		mvwprintw(DebugFonster, 10, 1, "S HP %d", monster_info(SLEMTROLL, HP));
		mvwprintw(DebugFonster, 11, 1, "K HP %d", monster_info(KODSKELLET, HP));
	}
}


/*
Styrning med WASD
Spelaren kan inte gå utanför mainfönstrets gränser
Funktionen är av booleansk typ och kommer returnera 1
om spelaren har flyttat sin karaktär.

Om spelaren försöker gå utanför spelplanen, så anropas
funktionen textruta med argumentet "Gå ej utanför!"
*/
_Bool flytta(void){
	int a;
	a = wgetch(MainFonster);
	switch(a){ 
		case 's':
			if (y<MAINFONSTER_LINJER-2){
				y++;
			}
			else {
				textruta(1, "Taggtråd! -1HP");
				SpelareHP(-1);
				}
			return 1;
			break;
		case 'w':
			if (y>1){
				y--;
			}
			else {
				textruta(1, "Taggtråd! -1HP");
				SpelareHP(-1);
				}
			return 1;
			break;
		case 'a':
			if (x>1){
				x--;
			}
			else  {
				textruta(1, "Taggtråd! -1HP");
				SpelareHP(-1);
				}
			return 1;
			break;
		case 'd':
			if (x<MAINFONSTER_KOLUMNER-2){
				x++;
			}
			else  {
				textruta(1, "Taggtråd! -1HP");
				SpelareHP(-1);
				}
			return 1;
			break;
		case 'o':
			knappar();
			return 2;
			break;
		default:
			return 0;
	}
}


char* knappar(void){
	int a;
	textruta(1, "i: Debuginfo | q: Avsluta");
	uppdatera_fonster();
	a = wgetch(MainFonster);
	switch(a){
		case 'i':
			VisaDebug = 1;
			textruta(1, "Visar Debug-info");
			return "";
			break;
		case 'q':
			exit(1);
	default:
		return "";
		break;
	}
}

void monster_move(_Bool Action){
	char * monsterTypNamn = "D", * monsterTypNamnFull = "Default"; //Default, om något blir fel så får monstret namn D
	int XellerY;
	for (int i=0;i<ANTAL_MONSTER;i++){
			switch(monster_typ[i]){
		case SLEMTROLL:
			monsterTypNamn = "S";
			monsterTypNamnFull = "Slemtroll";
			break;
		case KODSKELLET:
			monsterTypNamn = "K";
			monsterTypNamnFull = "Kodskelett";
			break;
		}
		mvwprintw(MainFonster, monster_y[i],monster_x[i], "%s", monsterTypNamn);
	}
	if(Action){ //Körs om funktionen matas med 1 
		/* 
		Jaga spelaren! 
		Först kollas om monster är längst från spelaren i x-led eller y-led
		Sedan kollas åt vilket håll monstret behöver gå åt, innan monstret
		till slut rör på sig.
		*/
		for (int i=0;i<ANTAL_MONSTER;i++){
			if (fabs(monster_y[i] - y) > fabs(monster_x[i] - x)){
				XellerY = 0;
			}
			else XellerY = 1;
			switch(XellerY){
				case 0:
					if (monster_y[i] < y)
						monster_y[i]++;
					else monster_y[i]--;
				case 1:
					if (monster_x[i] < x)
						monster_x[i]++;
					else monster_x[i]--;
			}
		}
		for (int i=0;i<ANTAL_MONSTER;i++)
			if (fabs(monster_y[i] - y) <2 && fabs(monster_x[i] - x) < 2)
				textruta(1, "Ett argt monster är nära!");
	}
}

int monster_info(int typ, int info){ //<-------OBS BYT HELA FUNKTIONEN MOT STRUCT!
	/*Slemtroll: typ 1
	Kodskelett: typ 2*
	Skada: info 1
	HP: info 2
	*/
	switch(typ){
		case SLEMTROLL:
			switch(info){
				case SKADA: return 10;	//Slemtroll Skada
				case HP: return 25;	//Slemtroll HP
			}
		case KODSKELLET:
			switch(info){
				case SKADA: return 8;		//Kodskelett Skada
				case HP: return 10;	//Kodskelett HP
			}
		}
}

int main(){
	initscr();	//Intierar fönster/grafik etc ?? - curses.h
	clear();		//Rensar skärmen - curses.h
	cbreak(); 	//? - curses.h
	noecho();	//? - curses.h
	srand(time(NULL));
	/*
	forsatsen sätter samtliga element i arrayen texthistory till " "
	*/
	for (int i=0;i<TEXTFONSTER_LINJER-3;i++){
		TextHistory[i] = " ";
	}
	/*
	Randomiserar x- och y-koordinater för potatisar som ska placeras
	på spelplanern. Som jag har förstått det så får man ett slumpat
	värde mellan 1-50 om man skriver rand() % 50. Så i mitt fall
	slumpas värdena inom main-fönstrets gränser (-2 för border)
	
	Samtliga potatisar får potatis_tagen[potatisnummer] = 0, den
	array jag använder för att hålla reda på om en potatis har blivit
	upplockat av spelaren eller inte (1/0).
	ANTAL_POTATIS är ett macro som definieras i början av filen.
	*/
	for (int i=0;i<ANTAL_POTATIS;i++){
		potatis_tagen[i] = 0;
		potatis_y[i] = rand() % (MAINFONSTER_LINJER-2);
		potatis_x[i] = rand() % (MAINFONSTER_KOLUMNER-2);
	}
	for (int i=0;i<ANTAL_MONSTER;i++){
		monster_y[i] = rand() % (MAINFONSTER_LINJER-2);
		monster_x[i] = rand() % (MAINFONSTER_KOLUMNER-2);
		if ((i % 2 == 0) || (i == 0)) 	monster_typ[i] = SLEMTROLL;
		else 					monster_typ[i] = KODSKELLET;
	}
	
	MainFonster = newwin(MAINFONSTER_LINJER, MAINFONSTER_KOLUMNER, MAINFONSTER_YSTART, MAINFONSTER_XSTART);
	InfoFonster = newwin(MAINFONSTER_LINJER,MAINFONSTER_XSTART-2,MAINFONSTER_YSTART,2);
	DebugFonster = newwin(MAINFONSTER_LINJER,20,MAINFONSTER_YSTART,MAINFONSTER_XSTART+MAINFONSTER_KOLUMNER+2);
	TextFonster = newwin(TEXTFONSTER_LINJER,MAINFONSTER_KOLUMNER+20,MAINFONSTER_YSTART+MAINFONSTER_LINJER,2);
	keypad(MainFonster, TRUE); 	//? - curses.h
	while (1){
		if (flytta()){
			uppdatera_fonster();
			radera_fonster();
			Potatisar();
			info();
			mvwprintw(MainFonster, y, x, "@");
			jkinfo();
			textruta(0,"N/A");
			uppdatera_fonster();
			stegRaknare++;
			if (stegRaknare % 2 == 0){  //Fiender/monster kommer röra sig var % 2 steg spelaren tar.
				monster_move(1);
			}
			else monster_move(0);
		}
	}
}
