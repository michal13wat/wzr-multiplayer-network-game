#include <stdio.h>
#include "kwaternion.h"

#define PI 3.1416

struct StanObiektu
{
  int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu (œrodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
  Wektor3 wV_kat, wA_kat;   // predkosc i przyspieszenie liniowe
  float masa;
};


struct Wezel
{
  Wektor3 wPol;         // po³o¿enie wêz³a
  Wektor3 normalna;     // normalna w trybie wyg³adzania
};

struct Sciana             
{
  long ind1,ind2,ind3;  // indeksy wêz³ów
  Wektor3 normalna;     // wektor prostopadly do  plaszczyzny trojkata
};

struct SiatkaObiektu    // siatka trójk¹tów
{
  Wezel *wezly;
  Sciana *sciany;
  long liczba_wezlow, liczba_scian;
};

struct SektorObiektu   // sektor zawieraj¹cy œciany  
{
  long x,y,z;           // wspó³rzêdne pocz¹tku sektora
  Sciana **sciany;      // tablica wskaŸników do œcian   
  long liczba_scian;    // liczba œcian w sektorze
  long rozmiar_pamieci; // liczba œcian, dla której zarezerwowano pamiêæ  
};

struct KomorkaTablicy  // komórka tablicy rozproszonej (te¿ mog³aby byæ tablic¹ rozproszon¹)
{
  SektorObiektu *sektory;  
  long liczba_sektorow;
  long rozmiar_pamieci; // liczba sektorów, dla której zarezerwowano pamiêæ  
};

class TabSektorow      // tablica rozproszona przechowuj¹ca sektory
{
//private:
public:
  long liczba_komorek;                                 // liczba komórek tablicy
  KomorkaTablicy *komorki; 
  long wyznacz_klucz(long x, long y, long z);          // wyznaczanie indeksu komórki 

public:
  TabSektorow(long liczba_kom);                        // na wejœciu konstruktora liczba komórek tablicy
  ~TabSektorow();

  SektorObiektu *znajdz(long x, long y, long z);       // znajdowanie sektorów (zwraca NULL jeœli nie znaleziono)
  SektorObiektu *wstaw(SektorObiektu s);               // wstawianie sektorów do tablicy
};

// Klasa opisuj¹ca teren, po którym poruszaj¹ siê obiekty
class Teren
{
public:     
    SiatkaObiektu siatka;    // siatka wêz³ów i œcian, z których sk³ada siê teren (mo¿e byæ dowolnego kszta³tu)
    Wektor3 srodek;          // œrodek przyci¹gania grawitacyjnego
    float promien_sredni;    // promieñ kuli
    TabSektorow *tab_sekt;    // tablica rozproszona do przechowywania sektorów
    float rozmiar_sektora;    // rozmiar sektora szeœciennego

    Teren();    
    ~Teren();   

    long SektoryWkierunku(SektorObiektu ***wsk_sekt, Wektor3 punkt_startowy, Wektor3 punkt_koncowy);  // zbiór sektorów, przez który przechodzi podany wektor
    Wektor3 PunktSpadku(Wektor3 P);     // okreœlanie rzutu P na teren w kierunku dzia³ania si³y ci¹¿enia
    Wektor3 PunktMax(Wektor3 v);        // okreœlanie punktu le¿¹cego jak najdalej od œrodka sfery w kierunku v
    void Rysuj();	                      // odrysowywanie terenu   
    void PoczatekGrafiki();             // tworzenie listy wyœwietlania
};


// Klasa opisuj¹ca obiekty ruchome
class ObiektRuchomy
{
public:
	int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu
	kwaternion qOrient;       // orientacja (polozenie katowe) 
	Wektor3 wV,wA;            // predkosc, przyspiesznie liniowe
  Wektor3 wV_kat, wA_kat;   // predkosc i przyspieszenie liniowe

	float F,Fb;               // si³y dzia³aj¹ce na obiekt: F - pchajaca do przodu, Fb - w prawo
	float ham;                // stopieñ hamowania Fh_max = tarcie*Fy*ham
  float F_max;              // maksymalna si³a napêdowa

	float alfa;               // kat skretu kol w radianach (w lewo - dodatni)
  float alfa_max;           // maksymalny k¹t skrêtu

	//float m;				  // masa obiektu	
  float masa_wlasna;        // masa w³asna obiektu [kg] (bez paliwa)
	float ilosc_paliwa;       // masa paliwa [kg]
  float masa;               // masa ca³kowita [kg]
	float dlugosc,szerokosc,wysokosc; // rozmiary w kierunku lokalnych osi x,y,z
	float przeswit;           // wysokoœæ na której znajduje siê podstawa obiektu
	float dl_przod;           // odleg³oœæ od przedniej osi do przedniego zderzaka 
	float dl_tyl;             // odleg³oœæ od tylniej osi do tylniego zderzaka
  Teren *teren;             // wskaŸnik do terenu

public:
  float promien;

  int iID_kolid;            // identyfikator pojazdu z którym nast¹pi³a kolizja (-1  brak kolizji)
	Wektor3 wdV_kolid;        // poprawka prêdkoœci pojazdu koliduj¹cego

	ObiektRuchomy();            // konstruktor
  ObiektRuchomy(Teren* teren); // konstruktor z umiejscowieniem w terenie
	~ObiektRuchomy();
	void ZmienStan(StanObiektu stan);          // zmiana stanu obiektu
	StanObiektu Stan();        // metoda zwracajaca stan obiektu
    void Symulacja(float dt);  // symulacja ruchu obiektu w oparciu o biezacy stan, przylozone sily
	                           // oraz czas dzialania sil. Efektem symulacji jest nowy stan obiektu 
	void Rysuj();			   // odrysowanie obiektu					
};



/*
    Wczytywanie eksportowanego z Blendera formatu obj (Wavefront), w ktorym zapisywane sa wierzcholki i sciany
    Zwraca 1 w razie powodzenia
*/
int tworz_z_obj(SiatkaObiektu *si,char nazwa_pliku[]);