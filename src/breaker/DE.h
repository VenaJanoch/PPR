#pragma once


#include <random>
#include "../skipjack.h"
using namespace std;

class DE
{
public:
	/*konstruktor, destruktor*/
	DE(float CR, int NP, float F, int generations, int D, TBlock &encrypted, const TBlock &reference);
	virtual ~DE();

	/*gettery*/
	int getPopSize() { return popSize; }
	int getDim() { return dimension; }

	/*settery*/
	void init(float mini, float maxi, int testFnctn);  //inicializacni funkce.
	void setPopulation();           //zalozi populaci.

	/*zakladni funkce diferencialniho evolucniho algoritmu*/
	void evolve();  //evolucni cyklus
	void binCross();            //binarni krizeni
	void expCross(int i);       //exponencialni krizeni

	/*mutacni algoritmy*/
	void best1(int i);
	void rand1(int i);
	void randToBest1(int i);
	void best2(int i);
	void rand2(int i);

	float* getBest(float &cost);

	int selectNoiseIndex();     //fce, ktera urci u exp. krizeni, kolik parametru vezmeme ze sumoveho vektoru

	/*cost function*/
	float costFunction(float * testPopulation, int index, int populationSize);


protected:
	float * population;//dynamicke pole populace (matice NP x D, kde jedinec = sloupec)
	float * newPop;    //nove vznikajici populace
	float * noise;     //sumovy vektor vznikly mutaci populace
	float * trial;     //zkusebni vektor vznikly krizenim sumoveho vektoru a 4. rodice
	float * best;      //nejlepsi dosavadni reseni problematiky

	void selectParents(int *r1, int *r2 = 0, int *r3 = 0, int *r4 = 0, int *r5 = 0);//pomocna funkce pro nahodny vyber rodicu

private:
	/* ZAKLADNI PARAMETRY ALGORITMU DIFERENCIALNI EVOLUCE */
	float crossover;   //CR - prah krizeni z intervalu <0;1>. Pri nabyti krajnich hodnot: 0 - kopie ctvrteho rodice; 1 - krizeni 3 rodicu, 4. vynechan
	int popSize;        //NP - velikost populace. <2D, 100D>. Nemelo by byt mensi nez 4.
	float factor;      //F - mutacni konstanta. <0, 2>
	int generations;    //>0 pocet evolucnich cyklu, behem nichz se populace vyviji
	int dimension;      //D - dimenze problemu - pocet argumentu ucelove funkce.
	/* KONEC ZAKLADNICH PARAMETRU DIFERENCIALNI EVOLUCE */

	int testFunction;   //vybrana testovaci funkce
	float minimum, maximum;
	int activeParent;   //index aktivniho rodice
	float bestCV;      //nejlepsi hodnota Cost Value



	int stagnation;     //priznak stagnace
	int maxStagnation;  //max priznak stagnace

	byte* encrypted;
	const byte* reference;

	std::mt19937 generator;   
};