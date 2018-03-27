/*********************************************************************
    Symulacja obiektów fizycznych ruchomych np. samochody, statki, roboty, itd. 
    + obs³uga obiektów statycznych np. teren.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "obiekty.h"
#include "grafika.h"

//#include "wektor.h"
extern FILE *f;
extern Teren teren;
extern int iLiczbaCudzychOb;
extern ObiektRuchomy *CudzeObiekty[2000]; 
extern bool czy_rysowac_ID;


ObiektRuchomy::ObiektRuchomy()             // konstruktor                   
{

	

}

ObiektRuchomy::ObiektRuchomy(Teren *teren)
{
  ObiektRuchomy();
  //iID = (unsigned int)(clock() % 1000);  // identyfikator obiektu
	iID = (unsigned int)(rand() % 1000);  // identyfikator obiektu
    fprintf(f,"MojObiekt->iID = %d\n",iID);


	F = Fb = alfa  = 0;	// si³y dzia³aj¹ce na obiekt 
  F_max = 20000;
  alfa = 0;
  alfa_max = PI*45.0/180;
	ham = 0;			// stopieñ hamowania
	masa_wlasna = 1000.0;			// masa obiektu [kg]
  ilosc_paliwa = 3.0;
	//Fy = m*9.81;        // si³a nacisku na podstawê obiektu (na ko³a pojazdu)
	dlugosc = 6.0;
	szerokosc = 2.3;
	wysokosc = 1.7;
	przeswit = 0.05;     // wysokoœæ na której znajduje siê podstawa obiektu
	dl_przod = 1.0;     // odleg³oœæ od przedniej osi do przedniego zderzaka 
	dl_tyl = 0.2;       // odleg³oœæ od tylniej osi do tylniego zderzaka
	
  //wPol.y = przeswit+wysokosc/2 + 20;
  //wPol.x = teren->srodek.x;
  //wPol.z = teren->srodek.z;
  this->teren = teren;
  wPol = teren->PunktMax(Wektor3(0,1,0)) + Wektor3(0,1,0)*(przeswit+wysokosc/2 + 20);

  fprintf(f,"Pol poczatkowe mojego obiektu = (%f, %f, %f)\n",wPol.x,wPol.y,wPol.z);
  fprintf(f,"Srodek kuli = (%f, %f, %f)\n",teren->srodek.x,teren->srodek.y,teren->srodek.z);

  wV_kat = Wektor3(0,1,0)*0;//40;  // pocz¹tkowa prêdkoœæ k¹towa (w celach testowych)
  promien = sqrt(dlugosc*dlugosc + szerokosc*szerokosc + wysokosc*wysokosc)/2/1.15;

	// obrót obiektu o k¹t 30 stopni wzglêdem osi y:
	kwaternion qObr = AsixToQuat(Wektor3(0,1,0),0.1*PI/180.0);
	qOrient = qObr*qOrient;

  iID_kolid = -1;           // na razie brak kolizji 
  
}

ObiektRuchomy::~ObiektRuchomy()            // destruktor
{
}

void ObiektRuchomy::ZmienStan(StanObiektu stan)  // przepisanie podanego stanu 
{                                                // w przypadku obiektów, które nie s¹ symulowane
	this->iID = stan.iID;                        

	this->wPol = stan.wPol;
	this->qOrient = stan.qOrient;
	this->wV = stan.wV;
	this->wA = stan.wA;
	this->wV_kat = stan.wV_kat;
	this->wA_kat = stan.wA_kat;
  this->masa = stan.masa;
}

StanObiektu ObiektRuchomy::Stan()                // metoda zwracaj¹ca stan obiektu ³¹cznie z iID
{
	StanObiektu stan;

	stan.iID = iID;
	stan.qOrient = qOrient;
	stan.wA = wA;
	stan.wA_kat = wA_kat;
	stan.wPol = wPol;
	stan.wV = wV;
	stan.wV_kat = wV_kat;
  stan.masa = masa;
	return stan;
}



void ObiektRuchomy::Symulacja(float dt)          // obliczenie nowego stanu na podstawie dotychczasowego,
{                                                // dzia³aj¹cych si³ i czasu, jaki up³yn¹³ od ostatniej symulacji

  if (dt == 0) return;

  float tarcie = 1.6;            // wspó³czynnik tarcia obiektu o pod³o¿e 
	float tarcie_obr = tarcie;     // tarcie obrotowe (w szczególnych przypadkach mo¿e byæ inne ni¿ liniowe)
  float tarcie_toczne = 0.05;    // wspó³czynnik tarcia tocznego
  float sprezystosc = 0.1;       // wspó³czynnik sprê¿ystoœci (0-brak sprê¿ystoœci, 1-doskona³a sprê¿ystoœæ) 
  float g = 9.81;                // przyspieszenie grawitacyjne na powierzchni planety, póŸniej maleje
                                 // wraz z kwadratem odlegloœci
  // zmniejszenie g z uwagi na odlegloœæ od œrodka:
  float odleglosc_od_srodka = (teren->srodek - wPol).dlugosc();
  if (odleglosc_od_srodka > teren->promien_sredni)
    g *= teren->promien_sredni*teren->promien_sredni/ odleglosc_od_srodka/odleglosc_od_srodka;

  masa = masa_wlasna + ilosc_paliwa;  // masa ca³kowita
  float Fy = masa*g;        // si³a nacisku na podstawê obiektu (na ko³a pojazdu)
       
  // obracam uk³ad wspó³rzêdnych lokalnych wed³ug kwaterniona orientacji:
	Wektor3 w_przod = qOrient.obroc_wektor(Wektor3(1,0,0)); // na razie oœ obiektu pokrywa siê z osi¹ x globalnego uk³adu wspó³rzêdnych (lokalna oœ x)
	Wektor3 w_gora = qOrient.obroc_wektor(Wektor3(0,1,0));  // wektor skierowany pionowo w górê od podstawy obiektu (lokalna oœ y)
	Wektor3 w_prawo = qOrient.obroc_wektor(Wektor3(0,0,1)); // wektor skierowany w prawo (lokalna oœ z)
	
  //fprintf(f,"dlug. wekt kierunkowych: %f, %f, %f\n",w_przod.dlugosc(),w_gora.dlugosc(), w_prawo.dlugosc());

  w_przod = w_przod.znorm();
  w_gora = w_gora.znorm();
  w_prawo = w_prawo.znorm();


	// rzutujemy wV na sk³adow¹ w kierunku przodu i pozosta³e 2 sk³adowe
	// sk³adowa w bok jest zmniejszana przez si³ê tarcia, sk³adowa do przodu
	// przez si³ê tarcia tocznego
	Wektor3 wV_wprzod = w_przod*(wV^w_przod),
	        wV_wprawo = w_prawo*(wV^w_prawo),
	        wV_wgore = w_gora*(wV^w_gora); 
	
  // dodatkowa normalizacja likwidujaca blad numeryczny:
  if (wV.dlugosc() > 0)
  {
    float blad_dlugosci = (wV_wprzod+wV_wprawo+wV_wgore).dlugosc()/wV.dlugosc();
    wV_wprzod = wV_wprzod /blad_dlugosci;
    wV_wprawo = wV_wprawo /blad_dlugosci;
    wV_wgore = wV_wgore /blad_dlugosci;

  }

  //fprintf(f,"|wV| = %f, po rozlozeniu i zlozeniu |wV| = %f\n",wV.dlugosc(),(wV_wprzod+wV_wprawo+wV_wgore).dlugosc());


  // rzutujemy prêdkoœæ k¹tow¹ wV_kat na sk³adow¹ w kierunku przodu i pozosta³e 2 sk³adowe
	Wektor3 wV_kat_wprzod = w_przod*(wV_kat^w_przod),
	        wV_kat_wprawo = w_prawo*(wV_kat^w_prawo),
	        wV_kat_wgore = w_gora*(wV_kat^w_gora);         


    // ograniczenia 
	if (F > F_max) F = F_max;
	if (F < -F_max) F = -F_max;
	if (alfa > alfa_max) alfa = alfa_max ;
	if (alfa < -alfa_max) alfa = -alfa_max ;


  Wektor3 wektor_ciazenia = (teren->srodek - wPol).znorm();
  Fy = masa*g*(-w_gora^wektor_ciazenia);                      // si³a docisku do pod³o¿a 
  //Fy = m*g*w_gora.y;                      // si³a docisku do pod³o¿a 
  if (Fy < 0 ) Fy = 0;
             // ... trzeba j¹ jeszcze uzale¿niæ od tego, czy obiekt styka siê z pod³o¿em!
  
    
  float V_wprzod = wV_wprzod.dlugosc();// - dt*Fh/m - dt*tarcie_toczne*Fy/m;
  //if (V_wprzod < 0) V_wprzod = 0;
    
  float V_wprawo = wV_wprawo.dlugosc();// - dt*tarcie*Fy/m;
  //if (V_wprawo < 0) V_wprawo = 0;


   
                  

      
      
  // wjazd lub zjazd: 
  //wPol.y = teren.Wysokosc(wPol.x,wPol.z);   // najprostsze rozwi¹zanie - obiekt zmienia wysokoœæ bez zmiany orientacji
	
	// 1. gdy wjazd na wklês³oœæ: wyznaczam wysokoœci terenu pod naro¿nikami obiektu (ko³ami), 
    // sprawdzam która trójka
	// naro¿ników odpowiada najni¿ej po³o¿onemu œrodkowi ciê¿koœci, gdy przylega do terenu
	// wyznaczam prêdkoœæ podbicia (wznoszenia œrodka pojazdu spowodowanego wklês³oœci¹) 
    // oraz prêdkoœæ k¹tow¹
	// 2. gdy wjazd na wypuk³oœæ to si³a ciê¿koœci wywo³uje obrót przy du¿ej prêdkoœci liniowej

	// punkty zaczepienia kó³ (na wysokoœci pod³ogi pojazdu):
  //     P  o------o Q
  //        |      |
  //        |      |
  //        |      |
  //        |      |
  //        |      |
  //     R  o------o S
  Wektor3 P = wPol + w_przod*(dlugosc/2-dl_przod) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
          Q = wPol + w_przod*(dlugosc/2-dl_przod) + w_prawo*szerokosc/2 - w_gora*wysokosc/2,
			    R = wPol + w_przod*(-dlugosc/2+dl_tyl) - w_prawo*szerokosc/2 - w_gora*wysokosc/2,
			    S = wPol + w_przod*(-dlugosc/2+dl_tyl) + w_prawo*szerokosc/2 - w_gora*wysokosc/2;

    // pionowe rzuty punktów zacz. kó³ pojazdu na powierzchniê terenu:  
  Wektor3 Pt = teren->PunktSpadku(P), Qt = teren->PunktSpadku(Q),
          Rt = teren->PunktSpadku(R), St = teren->PunktSpadku(S);   
  Wektor3 normPQR = normalna(Pt,Rt,Qt), normPRS = normalna(Pt,Rt,St), normPQS = normalna(Pt,St,Qt),
          normQRS = normalna(Qt,Rt,St);   // normalne do p³aszczyzn wyznaczonych przez trójk¹ty
	

  // to jeszcze trzeba poprawiæ: 

  // Czteroko³owy pojazd (o ko³ach rozmieszczonynch w wierzcho³kach kwadratu) mo¿e mieæ 4 (na idealnie p³askiej powierzchni)
  // lub 3 (najczêœciej w terenie pofa³dowanym) punkty podparcia. W takim przypadku pojazdy ustawiaj¹ siê zwykle tak, 
  // by osi¹gn¹æ minimum energii potencjalnej, tzn. by œrodek ciê¿koœci by³ jak najbli¿ej œrodka przyci¹gania
  // grawitacyjnego.
  // Szukam wiêc takiego 3-punktowego podparcia, gdzie œrodek pojazdu jest najni¿ej po³o¿ony (s¹ 4 takie przypadki).
  Wektor3 S_PQR = Rt + (Pt-Rt)/2 + (Qt-Pt)/2 + normPQR*wysokosc/2,     // po³o¿enie œrodka pojazdu gdy punkty P,Q,R stykaj¹ siê z pod³o¿em
    S_PRS = Rt + (Pt-Rt)/2 + (St-Pt)/2 + normPRS*wysokosc/2, 
    S_PQS = St + (Qt-St)/2 + (Pt-Qt)/2 + normPQS*wysokosc/2, 
    S_QRS = St + (Qt-St)/2 + (Rt-St)/2 + normQRS*wysokosc/2; 
  float sryPQR = (S_PQR - teren->srodek).dlugosc(),
    sryPRS = (S_PRS - teren->srodek).dlugosc(),
    sryPQS = (S_PQS - teren->srodek).dlugosc(),
    sryQRS = (S_QRS - teren->srodek).dlugosc();
  
  float sry = sryPQR; Wektor3 norm = normPQR;
  if (sry > sryPRS) {sry = sryPRS; norm = normPRS;}  
  if (sry > sryPQS) {sry = sryPQS; norm = normPQS;}
  if (sry > sryQRS) {sry = sryQRS; norm = normQRS;}  // wybór trójk¹ta o œrodku najni¿ej po³o¿onym    
           
  Wektor3 wV_kat_wpoziomie = Wektor3(0,0,0);
  // jesli któreœ z kó³ jest poni¿ej powierzchni terenu
  bool czy_cos_ponizej_terenu = ((P - teren->srodek).dlugosc() <= (Pt - teren->srodek).dlugosc()+ wysokosc/2+przeswit)||
      ((Q - teren->srodek).dlugosc() <= (Qt - teren->srodek).dlugosc()+ wysokosc/2+przeswit)||
      ((R - teren->srodek).dlugosc() <= (Rt - teren->srodek).dlugosc()+ wysokosc/2+przeswit)||
      ((S - teren->srodek).dlugosc() <= (St - teren->srodek).dlugosc()+ wysokosc/2+przeswit);
  if (czy_cos_ponizej_terenu)
  {   
     // obliczam powsta³¹ prêdkoœæ k¹tow¹ w lokalnym uk³adzie wspó³rzêdnych:      
     Wektor3 wobrot = -norm.znorm()*w_gora*0.6;  
     wV_kat_wpoziomie = wobrot/dt; 
  }    

  
  //Wektor3 wAg = Wektor3(0,-1,0)*g;    // przyspieszenie grawitacyjne na terenie p³askim

  Wektor3 wAg = wektor_ciazenia*g;    // przyspieszenie grawitacyjne


	// jesli wiecej niz 2 kola sa na ziemi, to przyspieszenie grawitacyjne jest rownowazone przez opor gruntu:
  int liczba_kol_na_ziemi = ((P - teren->srodek).dlugosc() <= (Pt - teren->srodek).dlugosc()+ wysokosc/2+przeswit)+
      ((Q - teren->srodek).dlugosc() <= (Qt - teren->srodek).dlugosc()+ wysokosc/2+przeswit)+
      ((R - teren->srodek).dlugosc() <= (Rt - teren->srodek).dlugosc()+ wysokosc/2+przeswit)+
      ((S - teren->srodek).dlugosc() <= (St - teren->srodek).dlugosc()+ wysokosc/2+przeswit) ;
  if (liczba_kol_na_ziemi >= 3)
    wAg = wAg + w_gora*(w_gora^wAg)*-1; // odejmujê sk³adow¹ przysp. w kierunku, w którym œrodek pojazdu
                                        // nie mo¿e siê przemieszaczaæ                             

	Fy *= (float)liczba_kol_na_ziemi/4;
  float Fh = Fy*tarcie*ham;                  // si³a hamowania (UP: bez uwzglêdnienia poœlizgu)

  
  // obliczam promien skrêtu pojazdu na podstawie k¹ta skrêtu kó³, a nastêpnie na podstawie promienia skrêtu
	// obliczam prêdkoœæ k¹tow¹ (UPROSZCZENIE! pomijam przyspieszenie k¹towe oraz w³aœciw¹ trajektoriê ruchu)
	if (Fy > 0)
	{
    float V_kat_skret = 0;
    if (alfa != 0)
    {   
	    float Rs = sqrt(dlugosc*dlugosc/4 + (fabs(dlugosc/tan(alfa)) + szerokosc/2)*(fabs(dlugosc/tan(alfa)) + szerokosc/2));
  		V_kat_skret = wV_wprzod.dlugosc()*(1.0/Rs);
    }	
    Wektor3 wV_kat_skret = w_gora*V_kat_skret*(alfa > 0 ? 1 : -1);
		Wektor3 wV_kat_wgore2 = wV_kat_wgore + wV_kat_skret;
		if (wV_kat_wgore2.dlugosc() <= wV_kat_wgore.dlugosc()) // skrêt przeciwdzia³a obrotowi
		{
		  if (wV_kat_wgore2.dlugosc() > V_kat_skret)
        wV_kat_wgore = wV_kat_wgore2; 
      else  
		    wV_kat_wgore = wV_kat_skret;
    }    
		else    
		{
      if (wV_kat_wgore.dlugosc() < V_kat_skret)
        wV_kat_wgore = wV_kat_skret;   
    }

    // tarcie zmniejsza prêdkoœæ obrotow¹ (UPROSZCZENIE! zamiast masy winienem wykorzystaæ moment bezw³adnoœci)     
    float V_kat_tarcie = Fy*tarcie_obr*dt/masa/1.0;      // zmiana pr. k¹towej spowodowana tarciem
    float V_kat_wgore = wV_kat_wgore.dlugosc() - V_kat_tarcie;
    if (V_kat_wgore < V_kat_skret) V_kat_wgore = V_kat_skret;        // tarcie nie mo¿e spowodowaæ zmiany zwrotu wektora pr. k¹towej
    wV_kat_wgore = wV_kat_wgore.znorm()*V_kat_wgore;                     
  } 

  // sk³adam z powrotem wektor prêdkoœci k¹towej:  
  wV_kat = wV_kat_wgore + wV_kat_wpoziomie;
      
      
  float h = sry+wysokosc/2+przeswit - wPol.y;  // ró¿nica wysokoœci jak¹ trzeba pokonaæ  
  //float V_podbicia = 0;
      
  Wektor3 dwPol = wV*dt + wA*dt*dt/2; // czynnik bardzo ma³y - im wiêksza czêstotliwoœæ symulacji, tym mniejsze znaczenie 
  wPol = wPol + dwPol;  
  

  // Sprawdzenie czy obiekt mo¿e siê przemieœciæ w zadane miejsce: Jeœli nie, to 
	// przemieszczam obiekt do miejsca zetkniêcia, wyznaczam nowe wektory prêdkoœci
	// i prêdkoœci k¹towej, a nastêpne obliczam nowe po³o¿enie na podstawie nowych
	// prêdkoœci i pozosta³ego czasu. Wszystko powtarzam w pêtli (pojazd znowu mo¿e 
	// wjechaæ na przeszkodê). Problem z zaokr¹glonymi przeszkodami - konieczne 
	// wyznaczenie minimalnego kroku.
          
  Wektor3 wV_pop = wV;  

  // sk³adam prêdkoœci w ró¿nych kierunkach oraz efekt przyspieszenia w jeden wektor:    (problem z przyspieszeniem od si³y tarcia -> to przyspieszenie 
	//      mo¿e dzia³aæ krócej ni¿ dt -> trzeba to jakoœ uwzglêdniæ, inaczej pojazd bêdzie wê¿ykowa³)
	wV = wV_wprzod.znorm()*V_wprzod + wV_wprawo.znorm()*V_wprawo + wV_wgore + wA*dt; 
	// usuwam te sk³adowe wektora prêdkoœci w których kierunku jazda nie jest mo¿liwa z powodu
	// przeskód:
	// np. jeœli pojazd styka siê 3 ko³ami z nawierzchni¹ lub dwoma ko³ami i œrodkiem ciê¿koœci to
	// nie mo¿e mieæ prêdkoœci w dó³ pod³ogi
	//if ((P.y <= Pt.y  + wysokosc/2+przeswit)||(Q.y <= Qt.y  + wysokosc/2+przeswit)||  
  //      (R.y <= Rt.y  + wysokosc/2+przeswit)||(S.y <= St.y  + wysokosc/2+przeswit))    // jeœli pojazd styka siê co najm. jednym ko³em
  if (czy_cos_ponizej_terenu)
	{
	   Wektor3 dwV = wV_wgore + w_gora*(wA^w_gora)*dt;
	   if ((w_gora.znorm() - dwV.znorm()).dlugosc() > 1 )  // jeœli wektor skierowany w dó³ pod³ogi
	    	wV = wV - dwV;
	}
	 
	 
	// sk³adam przyspieszenia liniowe od si³ napêdzaj¹cych i od si³ oporu: 
	wA = (w_przod*F + w_prawo*Fb)/masa*(Fy>0)  // od si³ napêdzaj¹cych
         - wV_wprzod.znorm()*(Fh/masa + tarcie_toczne*Fy/masa)*(V_wprzod>0.01) // od hamowania i tarcia tocznego (w kierunku ruchu)
         - wV_wprawo.znorm()*tarcie*Fy/masa/**(V_wprawo>0.01)*/    // od tarcia w kierunku prost. do kier. ruchu
         + wAg;           // od grawitacji
	
  /*fprintf(f,"czy_cos_ponizej_terenu = %d, liczba_kol_na_ziemi = %d\n",czy_cos_ponizej_terenu,liczba_kol_na_ziemi);
  fprintf(f,"odl od srodka ziemi = %f (sr.promien = %f)\n",(wPol - teren->srodek).dlugosc(),teren->promien_sredni);
  fprintf(f,"|V| = %f (V w kier. srodka = %f), |A| = %f, (A w kier. srodka = %f)\n",
    wV.dlugosc(),wektor_ciazenia^wV,wA.dlugosc(),wektor_ciazenia^wA);
  fprintf(f,"W KIER. WEKT. CIAZENIA: wV_wprzod.znorm()*V_wprzod = %f, wV_wprawo.znorm()*V_wprawo = %f, wV_wgore = %f,  wA*dt = %f\n",
    (wV_wprzod.znorm()*V_wprzod)^wektor_ciazenia, (wV_wprawo.znorm()*V_wprawo)^wektor_ciazenia, wV_wgore^wektor_ciazenia, (wA*dt)^wektor_ciazenia);
  */

	// obliczenie nowej orientacji:
	Wektor3 w_obrot = wV_kat*dt + wA_kat*dt*dt/2;    
	kwaternion q_obrot = AsixToQuat(w_obrot.znorm(),w_obrot.dlugosc());
	qOrient = q_obrot*qOrient; 
}

void ObiektRuchomy::Rysuj()
{
	glPushMatrix();
    
	
	glTranslatef(wPol.x,wPol.y+przeswit,wPol.z);
	
	Wektor3 k = qOrient.AsixAngle();     // reprezentacja k¹towo-osiowa kwaterniona
	
  Wektor3 k_znorm = k.znorm();

  glRotatef(k.dlugosc()*180.0/PI,k_znorm.x,k_znorm.y,k_znorm.z);
    glTranslatef(-dlugosc/2,-wysokosc/2,-szerokosc/2);
    glScalef(dlugosc,wysokosc,szerokosc);
    	
  glCallList(Auto);
	GLfloat Surface[] = { 2.0f, 2.0f, 1.0f, 1.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Surface);
  if (czy_rysowac_ID){
	  glRasterPos2f(0.30,1.20);
    glPrint("%d",iID ); 
  }
	glPopMatrix();
}




//*************************
//   Obiekty nieruchome
//*************************
Teren::Teren()
{
   char nazwa[128],nazwa_pliku[128],nazwa_pliku2[128];
   strcpy(nazwa,"planeta5");
   sprintf(nazwa_pliku,"%s.obj",nazwa);
   sprintf(nazwa_pliku2,"..//%s.obj",nazwa);
   int wynik = tworz_z_obj(&siatka,nazwa_pliku);      // wczytanie siatki tójk¹tów z formatu *.obj (Wavefront)
   if (wynik == 0)
     int wynik = tworz_z_obj(&siatka,nazwa_pliku2);      // wczytanie siatki tójk¹tów z formatu *.obj (Wavefront)

   float skala = 2000.0;         

   fprintf(f,"liczba wezlow = %d, liczba scian = %d\n",siatka.liczba_wezlow, siatka.liczba_scian); 

   for (long i=0;i<siatka.liczba_wezlow;i++)              // przeskalowanie siatki
     siatka.wezly[i].wPol = siatka.wezly[i].wPol*skala;


    

   for (long i=0;i<siatka.liczba_scian;i++)               // wyznaczenie normalnych do poszcz. œcian i wierzcho³ków
   {
     Wektor3 A = siatka.wezly[siatka.sciany[i].ind1].wPol, B = siatka.wezly[siatka.sciany[i].ind2].wPol,
       C = siatka.wezly[siatka.sciany[i].ind3].wPol;

     siatka.sciany[i].normalna = normalna(A,B,C);         // normalna do trojkata ABC
     siatka.wezly[siatka.sciany[i].ind1].normalna = siatka.wezly[siatka.sciany[i].ind2].normalna =
       siatka.wezly[siatka.sciany[i].ind3].normalna = siatka.sciany[i].normalna;  // normalna do wyg³adzania
   }

   // Wyznaczenie œrodka ciê¿koœci -> œrodka przyci¹gania grawitacyjnego
   float x_min=1e10,x_max=-1e10,y_min=1e10,y_max=-1e10,z_min=1e10,z_max=-1e10;
   for (long i=0;i<siatka.liczba_scian;i++)
   {
     Wektor3 P = siatka.wezly[siatka.sciany[i].ind1].wPol;
     if (x_min > P.x) x_min = P.x; 
     if (x_max < P.x) x_max = P.x;
     if (y_min > P.y) y_min = P.y; 
     if (y_max < P.y) y_max = P.y;
     if (z_min > P.z) z_min = P.z; 
     if (z_max < P.z) z_max = P.z;
   }
   srodek = Wektor3((x_max+x_min)/2, (y_max+y_min)/2, (z_max+z_min)/2);

   // Wyznaczenie œredniego promienia kuli:
   promien_sredni = 0;
   for (long i=0;i<siatka.liczba_wezlow;i++) 
     promien_sredni += (siatka.wezly[i].wPol - srodek).dlugosc()/siatka.liczba_wezlow;


   rozmiar_sektora = (x_max-x_min)/20;   // rozmiar sektora szeœciennego 

   tab_sekt = new TabSektorow(10000);

   // Umieszczenie œcian w sektorach, a sektorów w tablicy rozproszonej, by szybko je wyszukiwaæ:
   for (long i=0;i<siatka.liczba_scian;i++)               
   {
     float x_min=1e10,x_max=-1e10,y_min=1e10,y_max=-1e10,z_min=1e10,z_max=-1e10;  // granice przedzia³ów, w których mieœci siê œciana
     long e[] = {siatka.sciany[i].ind1, siatka.sciany[i].ind2, siatka.sciany[i].ind3};
     Wektor3 N = siatka.sciany[i].normalna, P = siatka.wezly[e[0]].wPol, 
             E0 = siatka.wezly[e[0]].wPol, E1 = siatka.wezly[e[1]].wPol, E2 = siatka.wezly[e[2]].wPol;

     for (long j=0;j<3;j++)      // po wierzcho³kach trójk¹ta
     {
       Wektor3 P = siatka.wezly[e[j]].wPol;
       if (x_min > P.x) x_min = P.x; 
       if (x_max < P.x) x_max = P.x;
       if (y_min > P.y) y_min = P.y; 
       if (y_max < P.y) y_max = P.y;
       if (z_min > P.z) z_min = P.z; 
       if (z_max < P.z) z_max = P.z;
     }
     // wyznaczenie indeksów sektorów dla granic przedzia³ów:
     long ind_x_min = (long)floor(x_min/rozmiar_sektora), ind_x_max = (long)floor(x_max/rozmiar_sektora),
       ind_y_min = (long)floor(y_min/rozmiar_sektora), ind_y_max = (long)floor(y_max/rozmiar_sektora),
       ind_z_min = (long)floor(z_min/rozmiar_sektora), ind_z_max = (long)floor(z_max/rozmiar_sektora);

     //fprintf(f,"i=%d, ind_x_min = %d, ind_x_max = %d\n",i,ind_x_min,ind_x_max);

     // skanowanie po wszystkich sektorach prostopad³oœcianu, w którym znajduje siê trójk¹t by sprawdziæ czy jakiœ
     // fragment trójk¹ta znajduje siê w sektorze. Jeœli tak, to sektor umieszczany jest w tablicy rozproszonej:
     for (long x = ind_x_min;x<=ind_x_max;x++)
       for (long y = ind_y_min;y<=ind_y_max;y++)
         for (long z = ind_z_min;z<=ind_z_max;z++)
         {
           bool czy_zawiera = 0;
           float x1 = (float)x*rozmiar_sektora, x2 = (float)(x+1)*rozmiar_sektora,  // granice sektora
             y1 = (float)y*rozmiar_sektora, y2 = (float)(y+1)*rozmiar_sektora,
             z1 = (float)z*rozmiar_sektora, z2 = (float)(z+1)*rozmiar_sektora;

           // sprawdzenie czy wewn¹trz szeœcianu sektora nie ma któregoœ z 3 wierzcho³ków: 
           for (long j=0;j<3;j++)      // po wierzcho³kach trójk¹ta
           {
             Wektor3 P = siatka.wezly[e[j]].wPol;
             if ((P.x >= x1)&&(P.x <= x2)&&(P.y >= y1)&&(P.y <= y2)&&(P.z >= z1)&&(P.z <= z2))
             {
               czy_zawiera = 1;
               break;
             }
           }

           // sprawdzenie, czy któryœ z 12 boków szeœcianu nie przecina œciany:
           if (czy_zawiera == 0)
           {

             // zbiór par wierzcho³ków definiuj¹cych boki:
             Wektor3 AB[][2] = {{Wektor3(x1,y1,z1), Wektor3(x2,y1,z1)}, {Wektor3(x1,y2,z1), Wektor3(x2,y2,z1)},  // w kier. osi x
                                {Wektor3(x1,y1,z2), Wektor3(x2,y1,z2)}, {Wektor3(x1,y2,z2), Wektor3(x2,y2,z2)},
                                {Wektor3(x1,y1,z1), Wektor3(x1,y2,z1)}, {Wektor3(x2,y1,z1), Wektor3(x2,y2,z1)},  // w kier. osi y
                                {Wektor3(x1,y1,z2), Wektor3(x1,y2,z2)}, {Wektor3(x2,y1,z2), Wektor3(x2,y2,z2)},
                                {Wektor3(x1,y1,z1), Wektor3(x1,y1,z2)}, {Wektor3(x2,y1,z1), Wektor3(x2,y1,z2)},  // w kier. osi z
                                {Wektor3(x1,y2,z1), Wektor3(x1,y2,z2)}, {Wektor3(x2,y2,z1), Wektor3(x2,y2,z2)}};

             for (long j=0;j<12;j++)
             {
               Wektor3 A = AB[j][0], B = AB[j][1];
               Wektor3 X = punkt_przec_prostej_z_plaszcz(A,B,N,P);
               if (czy_w_trojkacie(E0,E1,E2, X)&&(((X-A)^(X-B)) <= 0)){
                 czy_zawiera = 1;
                 break;
               }
             }
           }

           // sprawdzenie, czy któryœ z 3 boków trójk¹ta nie przecina którejœ z 6 œcian szeœcianu:
           if (czy_zawiera == 0)
           {
             Wektor3 AB[][2] = {{E0, E1}, {E1, E2}, {E2, E0}};
             for (long j=0;j<3;j++) // po bokach trójk¹ta
             {
               Wektor3 A = AB[j][0], B = AB[j][1];
               Wektor3 X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(1,0,0),Wektor3(x1,y1,z1));     // p³aszczyzna x = x1
               if ((X.y >= y1)&&(X.y <= y2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(1,0,0),Wektor3(x2,y1,z1));             // p³aszczyzna x = x2
               if ((X.y >= y1)&&(X.y <= y2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,1,0),Wektor3(x1,y1,z1));             // p³aszczyzna y = y1
               if ((X.x >= x1)&&(X.x <= x2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,1,0),Wektor3(x1,y2,z1));             // p³aszczyzna y = y2
               if ((X.x >= x1)&&(X.x <= x2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,0,1),Wektor3(x1,y1,z1));             // p³aszczyzna z = z1
               if ((X.x >= x1)&&(X.x <= x2)&&(X.y >= y1)&&(X.y <= y2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,0,1),Wektor3(x1,y1,z2));             // p³aszczyzna z = z2
               if ((X.x >= x1)&&(X.x <= x2)&&(X.y >= y1)&&(X.y <= y2)) { czy_zawiera = 1; break; }
             }

           }

           if (czy_zawiera)
           {
             SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
             if (sektor == NULL)
             {
               SektorObiektu sek;
               sek.liczba_scian = 1;
               sek.rozmiar_pamieci = 1;
               sek.sciany = new Sciana*[1];
               sek.sciany[0] = &siatka.sciany[i];
               sek.x = x; sek.y = y; sek.z = z;
               tab_sekt->wstaw(sek);
             }
             else
             {              
               if (sektor->liczba_scian >= sektor->rozmiar_pamieci)
               {
                 long nowy_rozmiar = sektor->rozmiar_pamieci*2;
                 Sciana **sciany2 = new Sciana*[nowy_rozmiar];
                 for (long j=0;j<sektor->liczba_scian;j++) sciany2[j] = sektor->sciany[j];
                 delete sektor->sciany;
                 sektor->sciany = sciany2;
                 sektor->rozmiar_pamieci = nowy_rozmiar;
               }
               sektor->sciany[sektor->liczba_scian] = &siatka.sciany[i];
               sektor->liczba_scian ++;
             }
           }


         } // 3x for po sektorach prostopad³oœcianu 

   } // for po œcianach
   fprintf(f,"floor(-1.1) = %f, MAX_LONG/2 = %d\n",floor(-1.1),LONG_MAX/2);

   fprintf(f,"srodek sfery = (%f, %f, %f)\n",srodek.x,srodek.y,srodek.z);  

   long liczba_sektorow = 0, liczba_wsk_scian = 0;
   for (long i=0;i<tab_sekt->liczba_komorek;i++)
   {
     //fprintf(f,"komorka %d zaweira %d sektorow\n",i,tab_sekt->komorki[i].liczba_sektorow);
     liczba_sektorow += tab_sekt->komorki[i].liczba_sektorow;
     for (long j=0;j<tab_sekt->komorki[i].liczba_sektorow;j++)
     {
       //fprintf(f,"  sektor %d (%d, %d, %d) zawiera %d scian\n",j,tab_sekt->komorki[i].sektory[j].x,
       //  tab_sekt->komorki[i].sektory[j].y, tab_sekt->komorki[i].sektory[j].z,
       //  tab_sekt->komorki[i].sektory[j].liczba_scian);
       liczba_wsk_scian += tab_sekt->komorki[i].sektory[j].liczba_scian;
     }
   }
   //fprintf(f,"laczna liczba sektorow = %d, wskaznikow scian = %d\n",liczba_sektorow, liczba_wsk_scian);

   //rozmiar_sektora = 10;
   //SektorObiektu **s = NULL;
   //long liczba_s = SektoryWkierunku(&s, Wektor3(35,-5,0), Wektor3(-15,35,0));
   //long liczba_s = SektoryWkierunku(&s, Wektor3(33,55,0),Wektor3(5,5,0) );
}

Teren::~Teren()
{            
  delete siatka.wezly; delete siatka.sciany;           
}

// Wyznaczanie sektorów przecinanych przez wektor o podanym punkcie startowym i koñcowym
// zwraca liczbê znalezionych sektorów
long Teren::SektoryWkierunku(SektorObiektu ***wsk_sekt, Wektor3 punkt_startowy, Wektor3 punkt_koncowy)
{
  long liczba_sektorow = 0;
  long liczba_max = 10;
  SektorObiektu **s = new SektorObiektu*[liczba_max];

  // wyznaczenie wspó³rzêdnych pocz¹tku sektora startowego i koñcowego:
  long x_start = (long)floor(punkt_startowy.x/rozmiar_sektora), y_start = (long)floor(punkt_startowy.y/rozmiar_sektora),
       z_start = (long)floor(punkt_startowy.z/rozmiar_sektora);
  long x_koniec = (long)floor(punkt_koncowy.x/rozmiar_sektora), y_koniec = (long)floor(punkt_koncowy.y/rozmiar_sektora),
    z_koniec = (long)floor(punkt_koncowy.z/rozmiar_sektora);
   
  float krok = (x_start < x_koniec ? 1 : -1); 
  for (long x=x_start;x!=x_koniec;x+=krok)
  {
    float xf = (float)(x+krok+(krok==-1))*rozmiar_sektora;   // wsp. x punktu przeciecia wektora z górn¹ p³aszczyzn¹ sektora
  	// wyznaczenie punktu przeciêcia wektora z kolejn¹ p³aszczyzn¹ w kierunku osi x (dla indeksu x):
	  Wektor3 P = punkt_przec_prostej_z_plaszcz(punkt_startowy, punkt_koncowy, Wektor3(1,0,0), Wektor3(xf,0,0));
	  // wyznaczenie indeksów sektora, którego górna p³aszczyzna jest przecinana: 
	  long y = (long)floor(P.y/rozmiar_sektora), z = (long)floor(P.z/rozmiar_sektora);
	  // dodanie sektora do listy:
    SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
    //fprintf(f,"dla x = %d, sektor (%d, %d, %d)\n",x,x,y,z);
	  if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	  }
	  if (liczba_max <= liczba_sektorow)
	  {
			long nowa_liczba_max = liczba_max*2;
			SektorObiektu **s2 = new SektorObiektu*[nowa_liczba_max];
			for (long i=0;i<liczba_sektorow;i++) s2[i] = s[i];
			delete s; s = s2; liczba_max = nowa_liczba_max;
	  }
  }
  krok = (y_start < y_koniec ? 1 : -1); 
	for (long y=y_start;y!=y_koniec;y+=krok)
  {
    float yf = (float)(y+krok+(krok==-1))*rozmiar_sektora;   // wsp. x punktu przeciecia wektora z górn¹ p³aszczyzn¹ sektora
  	// wyznaczenie punktu przeciêcia wektora z kolejn¹ p³aszczyzn¹ w kierunku osi x (dla indeksu x):
	  Wektor3 P = punkt_przec_prostej_z_plaszcz(punkt_startowy, punkt_koncowy, Wektor3(0,1,0), Wektor3(0,yf,0));
	  // wyznaczenie indeksów sektora, którego górna p³aszczyzna jest przecinana: 
	  long x = (long)floor(P.x/rozmiar_sektora), z = (long)floor(P.z/rozmiar_sektora);
	  // dodanie sektora do listy:

    SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
    //fprintf(f,"dla y = %d, sektor (%d, %d, %d)\n",y,x,y,z);
	  if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	  }
	  if (liczba_max <= liczba_sektorow)
	  {
			long nowa_liczba_max = liczba_max*2;
			SektorObiektu **s2 = new SektorObiektu*[nowa_liczba_max];
			for (long i=0;i<liczba_sektorow;i++) s2[i] = s[i];
			delete s; s = s2; liczba_max = nowa_liczba_max;
	  }
  }
	krok = (z_start < z_koniec ? 1 : -1); 
	for (long z=z_start;z!=z_koniec;z+=krok)
  {
    float zf = (float)(z+krok+(krok==-1))*rozmiar_sektora;   // wsp. x punktu przeciecia wektora z górn¹ p³aszczyzn¹ sektora
  	// wyznaczenie punktu przeciêcia wektora z kolejn¹ p³aszczyzn¹ w kierunku osi x (dla indeksu x):
	  Wektor3 P = punkt_przec_prostej_z_plaszcz(punkt_startowy, punkt_koncowy, Wektor3(0,0,1), Wektor3(0,0,zf));
	  // wyznaczenie indeksów sektora, którego górna p³aszczyzna jest przecinana: 
	  long x = (long)floor(P.x/rozmiar_sektora), y = (long)floor(P.y/rozmiar_sektora);
	  // dodanie sektora do listy:
    SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
    //fprintf(f,"dla z = %d, sektor (%d, %d, %d)\n",z,x,y,z);
	  if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	  }
	  if (liczba_max <= liczba_sektorow)
	  {
			long nowa_liczba_max = liczba_max*2;
			SektorObiektu **s2 = new SektorObiektu*[nowa_liczba_max];
			for (long i=0;i<liczba_sektorow;i++) s2[i] = s[i];
			delete s; s = s2; liczba_max = nowa_liczba_max;
	  }
  }
	SektorObiektu *sektor = tab_sekt->znajdz(x_koniec,y_koniec,z_koniec);
  //fprintf(f,"punkt koncowy, sektor (%d, %d, %d)\n",x_koniec,y_koniec,z_koniec);
	if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	}

  (*wsk_sekt) = s;

  return liczba_sektorow;
}

Wektor3 Teren::PunktSpadku(Wektor3 P)     // okreœlanie wysokoœci dla punktu P (odleg³oœci od najbli¿szej p³aszczyzny w kierunku grawitacji)
{                                         // 
  float odl_min = 1e10;
  Wektor3 Pp_min = this->srodek;          // œrodek bry³y
  long ind_min = 0;                      
  SektorObiektu **s = NULL;               // tablica wskaŸników sektorów                                   
  long liczba_s = SektoryWkierunku(&s, P, this->srodek); // wyszukanie sektorów przecinanych przez wektor P_srodek, w których znajduj¹ siê œciany 
  for (long j=0;j<liczba_s;j++)           // po niepustych sektorach
  {
    for (long i=0;i<s[j]->liczba_scian;i++)  // po œcianach z sektora j-tego
    {
      Wektor3 A = siatka.wezly[s[j]->sciany[i]->ind1].wPol, B = siatka.wezly[s[j]->sciany[i]->ind2].wPol,
         C = siatka.wezly[s[j]->sciany[i]->ind3].wPol;  // po³o¿enia wierzcho³ków trójk¹ta œciany
      Wektor3 Pp = punkt_przec_prostej_z_plaszcz(P, this->srodek, s[j]->sciany[i]->normalna,A); // punkt przeciêcia wektora P_srodek z p³aszczyzn¹ j-tej œciany
      bool czy_w_tr = czy_w_trojkacie(A, B, C, Pp);     // sprawdzenie czy punkt przeciêcia znajduje siê wewn¹trz trójk¹ta œciany
      if (czy_w_tr)                                     // jeœli punkt przeciêcia jest wewn¹trz trójk¹ta œciany
      {                                                 // to sprawdzenie, czy punkt ten le¿y najbli¿ej od punktu P
        float odl_od_plasz = (P-Pp).dlugosc();
        float odl_od_srodka = (P-srodek).dlugosc();
        if ((odl_od_srodka >= odl_od_plasz)&&(odl_min > odl_od_plasz))
        {
          odl_min = odl_od_plasz;
          ind_min = i;
          Pp_min = Pp;
        }
      }
    } // for po œcianach
  } // for po sektorach
  delete s;

  return Pp_min;    
}

Wektor3 Teren::PunktMax(Wektor3 v)     // okreœlanie punktu le¿¹cego jak najdalej od œrodka sfery w kierunku v
{
  float odl_max = 0;
  Wektor3 Pp_max = this->srodek;
  long ind_max = 0;
  for (long i=0;i<siatka.liczba_scian;i++)
  {
    Wektor3 A = siatka.wezly[siatka.sciany[i].ind1].wPol, B = siatka.wezly[siatka.sciany[i].ind2].wPol,
       C = siatka.wezly[siatka.sciany[i].ind3].wPol;
    Wektor3 Pp = punkt_przec_prostej_z_plaszcz(this->srodek, this->srodek + v,siatka.sciany[i].normalna,A);
    bool czy_w_tr = czy_w_trojkacie(A, B, C, Pp);
      

    if ((czy_w_tr)&&(((Pp-srodek)^v) > 0)) // czy punkt w trójk¹cie i w kierunku v 
    {
      
      float odl_od_srodka = (Pp-srodek).dlugosc();
      if (odl_max < odl_od_srodka)
      {
        odl_max = odl_od_srodka;
        ind_max = i;
        Pp_max = Pp;
      }
fprintf(f,"sciana %d: A(%f,%f,%f), B(%f,%f,%f), C(%f,%f,%f), Pp = (%f,%f,%f), czy_w_tr =%d\n",i,A.x,A.y,A.z,B.x,B.y,B.z,C.x,C.y,C.z,
        Pp.x,Pp.y,Pp.z,czy_w_tr);
      fprintf(f," odl. od srodka = %f\n",odl_od_srodka);
    }

  }

  return Pp_max;    
}

void Teren::PoczatekGrafiki()
{
  bool wygladzanie = 0;
  glNewList(PowierzchniaTerenu,GL_COMPILE);
  glBegin(GL_TRIANGLES);

  for (long i=0;i<siatka.liczba_scian;i++)
  {
    Wektor3 A = siatka.wezly[siatka.sciany[i].ind1].wPol, B = siatka.wezly[siatka.sciany[i].ind2].wPol,
       C = siatka.wezly[siatka.sciany[i].ind3].wPol;
    Wektor3 n = siatka.sciany[i].normalna;
    if (wygladzanie) n = siatka.wezly[siatka.sciany[i].ind1].normalna;   
    glNormal3f(n.x, n.y, n.z);
    glVertex3f( A.x, A.y, A.z);
    if (wygladzanie) n = siatka.wezly[siatka.sciany[i].ind2].normalna;
    glNormal3f(n.x, n.y, n.z);
    glVertex3f( B.x, B.y, B.z);
    if (wygladzanie) n = siatka.wezly[siatka.sciany[i].ind3].normalna;
    glNormal3f(n.x, n.y, n.z);
    glVertex3f( C.x, C.y, C.z);
  }
  glEnd();
  glEndList(); 
}



void Teren::Rysuj()
{
  glCallList(PowierzchniaTerenu);               
}

   
TabSektorow::TabSektorow(long liczba_kom)
{
  liczba_komorek = liczba_kom;
  komorki = new KomorkaTablicy[liczba_komorek];
  for (long i=0;i<liczba_komorek;i++)
  {
    komorki[i].liczba_sektorow = 0;
    komorki[i].rozmiar_pamieci = 0;
    komorki[i].sektory = NULL;  
  }
}

TabSektorow::~TabSektorow()
{
  for (long i=0;i<liczba_komorek;i++) 
    delete komorki[i].sektory;
  delete komorki;
}
      
long TabSektorow::wyznacz_klucz(long x, long y, long z)                  // wyznaczanie indeksu komórki 
{
  long kl = (long)fabs((cos(x*1.0)*cos(y*1.1)*cos(z*1.2)+cos(x*1.0)+cos(y*1.0)+cos(z*1.0))*liczba_komorek*10) % liczba_komorek;
  return kl;
}

SektorObiektu *TabSektorow::znajdz(long x, long y, long z)       // znajdowanie sektora (zwraca NULL jeœli nie znaleziono)
{
  long klucz = wyznacz_klucz(x,y,z);
  SektorObiektu *sektor = NULL;
  for (long i=0;i<komorki[klucz].liczba_sektorow;i++)
  {
    SektorObiektu *s = &komorki[klucz].sektory[i];
    if ((s->x == x)&&(s->y == y)&&(s->z == z)) {
      sektor = s;
      break;
    }
  }
  return sektor;
}

SektorObiektu *TabSektorow::wstaw(SektorObiektu s)      // wstawianie sektora do tablicy
{
  long x = s.x, y = s.y, z = s.z;
  long klucz = wyznacz_klucz(x,y,z);
  long liczba_sekt = komorki[klucz].liczba_sektorow;
  if (liczba_sekt >= komorki[klucz].rozmiar_pamieci)     // jesli brak pamieci, to nale¿y j¹ powiêkszyæ 
  {
    long nowy_rozmiar = (komorki[klucz].rozmiar_pamieci == 0 ? 1: komorki[klucz].rozmiar_pamieci*2);
    SektorObiektu *sektory2 = new SektorObiektu[nowy_rozmiar];
    for (long i=0;i<komorki[klucz].liczba_sektorow;i++) sektory2[i] = komorki[klucz].sektory[i];
    delete komorki[klucz].sektory;
    komorki[klucz].sektory = sektory2;
    komorki[klucz].rozmiar_pamieci = nowy_rozmiar;
  }
  komorki[klucz].sektory[liczba_sekt] = s; 
  komorki[klucz].liczba_sektorow++;

  return &komorki[klucz].sektory[liczba_sekt];           // zwraca wskaŸnik do nowoutworzonego sektora
}

/*
    Wczytywanie eksportowanego z Blendera formatu obj, w ktorym zapisywane sa wierzcholki i sciany
    Zwraca 1 w razie powodzenia, a 0 gdy nie da siê odczytaæ pliku
*/
int tworz_z_obj(SiatkaObiektu *si,char nazwa_pliku[])
{
  FILE *f = fopen(nazwa_pliku,"r");

  if (f == NULL)
    return 0; 

  char lan[1024],napis[128],s1[128],s2[128],s3[128],s4[128];

  long liczba_max_wezlow = 100;
  long liczba_max_scian = 100;

  si->wezly = new Wezel[liczba_max_wezlow];
  si->liczba_wezlow = 0;
  si->sciany = new Sciana[liczba_max_scian];
  si->liczba_scian = 0;

  long lw = si->liczba_wezlow, ls = si->liczba_scian;

  while (fgets(lan,1024,f))
  {
    sscanf(lan,"%s",&napis);

    if (strcmp(napis,"v") == 0)     // znaleziono dane wierzcholka -> tworze nowa kule
    {
      sscanf(lan,"%s %s %s %s",&napis,&s1,&s2,&s3);
      si->wezly[lw].wPol = Wektor3(atof(s1),atof(s2),atof(s3));

      lw++;  si->liczba_wezlow = lw;

      if (si->liczba_wezlow >= liczba_max_wezlow)  // jesli pamiec na kule wyczerpala sie
      {
        liczba_max_wezlow *= 2;
        Wezel *wezly_pom = si->wezly;
        si->wezly = new Wezel[liczba_max_wezlow];
        for (long i=0;i<si->liczba_wezlow;i++) si->wezly[i] = wezly_pom[i];
        delete wezly_pom;
      }
    }

    if (strcmp(napis,"f") == 0)  // znaleziono trojkat (lub czworokat) -> tworze nowa scianke 
    {
      int wynik = sscanf(lan,"%s %s %s %s %s",&napis,&s1,&s2,&s3,&s4);
      int i1 = atoi(s1)-1, i2 = atoi(s2)-1, i3 = atoi(s3)-1;  // w pliku *.obj wezly indeksowane sa od 1, stad trzeba odjac 1 by przejsc na ind. od 0
      si->sciany[ls].ind1 = i1; si->sciany[ls].ind2 = i2; si->sciany[ls].ind3 = i3;
      ls++; si->liczba_scian = ls;

      if (wynik == 5)  // jesli siatka czworokatow a nie trojkatow
      {
        int i4 = atoi(s4)-1;
        si->sciany[ls].ind1 = i3; si->sciany[ls].ind2 = i4; si->sciany[ls].ind3 = i1;
        ls++; si->liczba_scian = ls;

      }
      if (si->liczba_scian >= liczba_max_scian)  // jesli pamiec na kule wyczerpala sie
      {
        liczba_max_scian *= 2;
        Sciana *sciany_pom = si->sciany;
        si->sciany = new Sciana[liczba_max_scian];
        for (long i=0;i<si->liczba_scian;i++) si->sciany[i] = sciany_pom[i];
        delete sciany_pom;
      }

    }

  } // while po wierszach pliku

  fclose(f);

  return 1;
}