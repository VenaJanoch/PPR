#include "DE.h"

#include <cstdlib>
#include <time.h>
#include <math.h>
#include <cstring>
#include <windows.h>
#include <immintrin.h>
#include <iostream>
#include <tbb/tbb.h>
#include <tbb/mutex.h>

using namespace std;

DE::DE(float CR, int NP, float F, int generation, int D, TBlock &encrypted_, const TBlock &reference_ ) :
	crossover(CR), popSize(NP), factor(F), generations(generation), dimension(D), encrypted(encrypted_), reference(reference_) {


	this->population = new float[NP * (D + 1)];//zalozime populaci o velikosti NP*(D+1) - navyseni o 1 dimenzi je kvuli CostValue
	this->newPop = new float[NP * (D + 1)];    //populace potomku
	this->noise = new float[D];              //sumovy vektor jedince nebude mit CostValue
	this->trial = new float[D + 1];            //zkusebni vektor jedince CV ma
	this->best = new float[D + 1];             //stejne tak i doposud nejlepsi jedinec si uchovava svou nejlepsi hodnotu CV

	//nastavime hodnotu priznaku stagnace. Pokud se hodnota bestCV nezmeni 200<=stagnation<=generations/3, ukoncime evoluci.
	if (generation <= 1500)
		maxStagnation = 500;
	else
		maxStagnation = generation / 3;
	stagnation = 0;


	SYSTEMTIME time;
	GetSystemTime(&time);
	LONG time_ms = (time.wSecond * 1000) + time.wMilliseconds;

	generator.seed(time_ms);
	
}

DE::~DE() {
	//destruktor
	if (population) delete population;
	population = 0;
	
}

//naplnime populaci zakladnimi hodnotami
void DE::setPopulation() {
	int NP = this->getPopSize();
	int D = this->getDim() + 1;     //dimenze+cost value(hodnota ucelove funkce s danymi parametry)

	int bestIndex = 0;
	std::uniform_real_distribution<float> uniformDistribution(minimum, maximum);

	//1. vygeneruju populaci
	for (int i = NP; i < NP*D; i++) {
		population[i] = uniformDistribution(generator); 
	}

	//2. ohodnotim populaci
	for (int i = 0; i < NP; i++) {
		population[i] = costFunction(population, i, NP); //vyhodnoceni kvality jedincu v populaci

		/*vyhledam doposud nejlepsi hodnotu Cost Value*/
		if (i == 0) {//pokud se jedna o prvni vstup, nastavime prvni jako nejlepsi hodnotu
			bestCV = population[i];
		}
		else if (bestCV > population[i]) {//jinak porovnavame
			bestCV = population[i];
			bestIndex = i;
		}
	}

	//3. naplnime nejlepsi vektor patricnymi parametry
	for (int i = 0; i <= dimension; i++)
		best[i] = population[bestIndex + i * popSize];

	//cout << "Nejlepsi CV uvodni populace je " << bestCV << endl;
	//myfile << "Nejlepsi CV uvodni populace je " << bestCV << endl;

	return;
}

//inicializacni funkce
void DE::init(float mini = 0.0, float maxi = 255.0, int testFnctn = 0) {
	minimum = mini;
	maximum = maxi;
	testFunction = testFnctn;
	
	setPopulation();
}

void DE::evolve() {
	int gen = 0;
	for (gen = 0; gen < generations; gen++) {
		
		tbb::parallel_for(0, popSize, [&](int x) {
			activeParent = x;
			int i = selectNoiseIndex() - 1;

			best1(i);
			expCross(i);

			for (int ix = 0; ix <= dimension; ix++) {
				newPop[x + ix * popSize] = trial[ix];
			}
		});
				
		population = newPop; 
	}

	if (stagnation == maxStagnation) {
		//cout << endl << endl << "Evoluce stagnuje, bude proto ukoncena a to po " << stagnation << " nezlepsujicich se generacich." << endl;
	}
}

void DE::selectParents(int *r1, int *r2, int *r3, int *r4, int *r5) {
	int active = this->activeParent;  //aktivni rodic - rodice, ktereho pouzijeme pro budouci krizeni
	int limit;                      //mezni hodnota populace pro dany algoritmus

	std::uniform_real_distribution<float> uniformDistribution(0, popSize);

	if (r1 && r2) {
		limit = 3;
		do {
			*r1 = (int) uniformDistribution(generator);
			*r2 = (int) uniformDistribution(generator);
		} while ((*r2 == *r1) || (*r2 == active) || (*r1 == active));
	}
	if (r3) {
		limit = 4;
		do {
			*r3 = (int)uniformDistribution(generator);
		} while ((*r3 == active) || (*r3 == *r2) || (*r3 == *r1));
	}

	if (r4) {
		limit = 5;
		do {
			*r4 = (int)uniformDistribution(generator);
		} while ((*r4 == active) || (*r4 == *r3) || (*r4 == *r2) || (*r4 == *r1));
	}

	if (r5) {
		limit = 6;
		
		do {
			*r5 = (int)uniformDistribution(generator);
		} while ((*r5 == active) || (*r5 == *r4) || (*r5 == *r3) || (*r5 == *r2) || (*r5 == *r1));
	}

	if (popSize < limit) {
		//cout << "Populace je prilis nizka, nez aby se dala vykonat diferencialni evoluce. Musi mit minimalne " << limit << " jedince." << endl;
		return;
	}
}


void DE::best1(int x) {
	//vybereme nahodne dva jedince z aktualni populace
	  
	float x1f, x2f;	//hodnoty nahodne vybranych jedincu
	int r1, r2;     //indexy nahodne vybranych jedincu

	selectParents(&r1, &r2);

	int i = 0;

	for (; i < x - 7; i += 8) {
	
	float	x1_1 = population[r1 + i * popSize];
	float 	x2_1 = population[r2 + i * popSize];

	float	x1_2 = population[r1 + (i + 1)*popSize];
	float 	x2_2 = population[r2 + (i + 1)*popSize];

	float	x1_3 = population[r1 + (i + 2)*popSize];
	float 	x2_3 = population[r2 + (i + 2)*popSize];

	float	x1_4 = population[r1 + (i + 3)*popSize];
	float 	x2_4 = population[r2 + (i + 3)*popSize];

	float	x1_5 = population[r1 + (i + 4)*popSize];
	float 	x2_5 = population[r2 + (i + 4)*popSize];

	float	x1_6 = population[r1 + (i + 5)*popSize];
	float 	x2_6 = population[r2 + (i + 5)*popSize];

	float	x1_7 = population[r1 + (i + 6)*popSize];
	float 	x2_7 = population[r2 + (i + 6)*popSize];

	float	x1_8 = population[r1 + (i + 7)*popSize];
	float 	x2_8 = population[r2 + (i + 7)*popSize];

	
		
	__m256 x1 = _mm256_set_ps(x1_1, x1_2, x1_3, x1_4, x1_5, x1_6, x1_7, x1_8);
	__m256 x2 = _mm256_set_ps(x2_1, x2_2, x2_3, x2_4, x2_5, x2_6, x2_7, x2_8);

	__m256 factor_m = _mm256_set1_ps(factor);  //_mm256_set_ps(factor, factor, factor, factor, factor, factor, factor, factor);

	__m256 result = _mm256_sub_ps(x1, x2);
	
	__m256 mul_m = _mm256_mul_ps(result, factor_m);

	__m256 best_m = _mm256_loadu_ps(&best[i]);

	__m256 add_m = _mm256_add_ps(mul_m, best_m);

	noiseMutex.lock();
	memcpy(&noise[i], (float*)&add_m, 8 * sizeof(float));
	noiseMutex.unlock();
	for (; i < x; i++) {

		x1f = population[r1 + (i)*popSize];
		x2f = population[r2 + (i)*popSize];
		noise[i] = best[i] + factor * (x1f - x2f);
	}
	}



	//pro vsechny parametry populace:
	//for (int i = 0; i < x; i++) {
	//	x1 = population[r1 + (i + 1)*popSize];
	//x2 = population[r2 + (i + 1)*popSize];
						
	//	noise[i] = best[i + 1] + factor * ( x1 - x2);
	//}
}

void DE::expCross(int noiseIndex) {

	//nahodnou hodnotu porovname s CR
	//dokud plati ze:
	//nahodna hodnota<CR -> sumovy vektor
	//nahodna hodnota>CR -> vsechny ostatni parametry 4. rod

	//nahodna hodnota=noiseIndex

	trialMutex.lock();
	for (int i = 1; i <= dimension; i++) {

		

		if (i <= noiseIndex) {
			//osetreni abychom nevylezli mimo meze
			if (noise[i - 1]<minimum or noise[i - 1]>maximum) { trial[i] = population[activeParent + i * popSize]; }
			else { trial[i] = noise[i - 1]; }
		}
		else {
			trial[i] = population[activeParent + i * popSize];
		}
	}

	
	float trialEnergy = costFunction(trial, 0, 1);
	float parentEnergy = costFunction(population, activeParent, getPopSize());

	
	if (trialEnergy <= parentEnergy) {
		trial[0] = trialEnergy;
	}
	else {
		trial[0] = parentEnergy;
		for (int i = 1; i <= dimension; i++) {
			trial[i] = population[activeParent + i * popSize];
		}
	}
	trialMutex.unlock();

	if (bestCV > trial[0]) {
		stagnation = 0;
		bestCV = trial[0];


		for (int i = 0; i <= dimension; i++) {
			best[i] = trial[i];
		}
	}
}

int DE::selectNoiseIndex() {

	int noiseIndex;
	std::uniform_real_distribution<float> uniformDistribution(0, 1);

	for (noiseIndex = 0; (noiseIndex <= dimension) && (uniformDistribution(generator) < crossover); noiseIndex++)
	{
	}

	return noiseIndex;
}

float DE::costFunction(float * testPopulation, int index, int popSiz) {
	float costValue = 0;

		
	byte value[sizeof(TPassword)];

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		value[i] = (byte)testPopulation[i + index*sizeof(TPassword)];
	}

	SJ_context context;
	TBlock decrypted;

	makeKey(&context, value, sizeof(TPassword));
	decrypt_block(&context, decrypted, encrypted);
	

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		for (size_t j = 0; j < 8; j++)
		{

			if (((decrypted[i] >> j) & 1) != ((reference[i] >> j) & 1)) {
				costValue++;
			}

		}

	}

	return costValue;

}

float* DE::getBest(float &cost) {

	float best_act = 1000000; 
	size_t best_index = 0;
	
	for (size_t i = 0; i < popSize; i++)
	{
		float tmp = costFunction(population, i, popSize);
		if (tmp < best_act) {
			best_act = tmp;
			best_index = i;
		}
	}

	byte value[sizeof(TPassword)];

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		value[i] = (byte)population[i + best_index * sizeof(TPassword)];
	}


	SJ_context context;
	TBlock decrypted;
	std::cout << "Score: " << best_act << '\n';

	makeKey(&context, value, sizeof(TPassword));
	decrypt_block(&context, decrypted, encrypted);

	cost = best_act;

	return &population[best_index * sizeof(TPassword)];
}
