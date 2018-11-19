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
#include <amp.h>  
using namespace concurrency;
using namespace std;

const int calculation_definition = 1;

DE::DE(float CR, int NP, float F, int generation, int D, TBlock &encrypted_, const TBlock &reference_ ) :
	crossover(CR), popSize(NP), factor(F), generations(generation), dimension(D), encrypted(encrypted_), reference(reference_) {


	this->population = new float[NP * (D + 1)];//zalozime populaci o velikosti NP*(D+1) - navyseni o 1 dimenzi je kvuli CostValue
	this->newPop = new float[NP * (D + 1)];    //populace potomku
	//this->noise = new float[D];              //sumovy vektor jedince nebude mit CostValue
	//this->trial = new float[D + 1];            //zkusebni vektor jedince CV ma
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
			float * trial = new float[dimension + 1];
			float * noise = new float[dimension];
			best1(i, noise);
			expCross(i, trial, noise);

			for (int ix = 0; ix <= dimension; ix++) {
				newPop[x + ix * popSize] = trial[ix];
			}

			delete[] trial;
			delete[] noise;
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


void DE::best1(int x, float* noise) {
	//vybereme nahodne dva jedince z aktualni populace

		int r1, r2;     //indexy nahodne vybranych jedincu

	selectParents(&r1, &r2);

#if calculation_definition
	gpu_calculation(noise, x, r1, r2);
#else  // 0
	avx_calculation(noise, x, r1, r2);
#endif


	
}
void DE::gpu_calculation(float* noise, int x, int r1, int r2) {
	float a[10];
	float c[10];
	float b[10];
	float factor_array[10] = { factor, factor, factor, factor, factor, factor, factor, factor, factor, factor };

	bestMutex.lock();
	float best_array[10] = { best[0], best[1], best[2], best[3], best[4], best[5], best[6], best[7], best[8], best[9] };
	bestMutex.unlock();

	for (int j = 0; j < x; j += 10) {

		a[0] = population[r1 + j * popSize];
		b[0] = population[r2 + j * popSize];

		a[1] = population[r1 + (j + 1)*popSize];
		b[1] = population[r2 + (j + 1)*popSize];

		a[2] = population[r1 + (j + 2)*popSize];
		b[2] = population[r2 + (j + 2)*popSize];

		a[3] = population[r1 + (j + 3)*popSize];
		b[3] = population[r2 + (j + 3)*popSize];

		a[4] = population[r1 + (j + 4)*popSize];
		b[4] = population[r2 + (j + 4)*popSize];

		a[5] = population[r1 + (j + 5)*popSize];
		b[5] = population[r2 + (j + 5)*popSize];

		a[6] = population[r1 + (j + 6)*popSize];
		b[6] = population[r2 + (j + 6)*popSize];

		a[7] = population[r1 + (j + 7)*popSize];
		b[7] = population[r2 + (j + 7)*popSize];

		a[8] = population[r1 + (j + 8)*popSize];
		b[8] = population[r2 + (j + 8)*popSize];

		a[9] = population[r1 + (j + 9)*popSize];
		b[9] = population[r2 + (j + 9)*popSize];

		array_view<float, 1> a_view(10, a);
		array_view<float, 1> b_view(10, b);
		array_view<float, 1> result_view(10, noise);
		array_view<float, 1> factor_view(10, factor_array);
		array_view<float, 1> best_view(10, best_array);


		parallel_for_each(
			result_view.extent,
			[=](index<1> idx) restrict(amp)
		{
			result_view[idx] = best_view[idx] + (factor_view[idx] * (a_view[idx] - b_view[idx]));

		}
		);


		result_view.synchronize();


	}

	}




	void DE::avx_calculation(float* noise, int x, int r1, int r2) {

		int i = 0;
		float x1f, x2f;	//hodnoty nahodne vybranych jedincu

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
			__m256 factor_m = _mm256_set_ps(factor, factor, factor, factor, factor, factor, factor, factor);

			__m256 result = _mm256_sub_ps(x1, x2);

			__m256 mul_m = _mm256_mul_ps(result, factor_m);

			__m256 best_m = _mm256_loadu_ps(&best[i]);

			__m256 add_m = _mm256_add_ps(mul_m, best_m);


			memcpy(&noise[i], (float*)&add_m, 8 * sizeof(float));

			for (; i < x; i++) {

				x1f = population[r1 + (i)*popSize];
				x2f = population[r2 + (i)*popSize];
				noise[i] = best[i] + factor * (x1f - x2f);
			}
		}

	}



void DE::expCross(int noiseIndex, float* trial, float* noise) {

	//nahodnou hodnotu porovname s CR
	//dokud plati ze:
	//nahodna hodnota<CR -> sumovy vektor
	//nahodna hodnota>CR -> vsechny ostatni parametry 4. rod

	//nahodna hodnota=noiseIndex

	for (int i = 1; i <= dimension; i++) {

		

		if (i <= noiseIndex) {
			//osetreni abychom nevylezli mimo meze
			if (noise[i - 1]<minimum || noise[i - 1]>maximum) { trial[i] = population[activeParent + i * popSize]; }
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

	if (bestCV > trial[0]) {
		stagnation = 0;
		bestCV = trial[0];

		bestMutex.lock();
		for (int i = 0; i <= dimension; i++) {
			best[i] = trial[i];
		}
		bestMutex.unlock();
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
