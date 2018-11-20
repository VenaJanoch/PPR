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


//#define GPU

DE::DE(float CR, int NP, float F, int generation, int D, TBlock &encrypted_, const TBlock &reference_ ) :
	crossover(CR), popSize(NP), factor(F), generations(generation), dimension(D), encrypted(encrypted_), reference(reference_) {


	size_t polulation_array_size = (size_t)NP * ((size_t)D + 1);
	this->population = new float[polulation_array_size];
	this->new_pop = new float[polulation_array_size];              	
}

DE::~DE() {
	if (population) delete population;
	population = 0;
	
}

void DE::set_population() {
	int NP = popSize;
	int D = dimension + 1;    

	int bestIndex = 0;
	
	std::mt19937 generator;
	SYSTEMTIME time;
	GetSystemTime(&time);
	LONG time_ms = (time.wSecond * 1000) + time.wMilliseconds;

	generator.seed(time_ms);
	
	std::uniform_real_distribution<float> uniformDistribution(minimum, maximum);

	for (int i = NP; i < NP*D; i++) {
		population[i] = uniformDistribution(generator); 
	}

	for (int i = 0; i < NP; i++) {
		population[i] = cost_function(population, i, NP);
	}
		
	return;
}

void DE::init(float mini = 0.0, float maxi = 255.0) {
	minimum = mini;
	maximum = maxi;
	set_population();
}

void DE::evolve() {
	int gen = 0;
	for (gen = 0; gen < generations; gen++) {
		
		tbb::parallel_for(0, popSize, [&](int x) {
			
			std::mt19937 generator1;
				SYSTEMTIME time;
			GetSystemTime(&time);
			LONG time_ms = (time.wSecond * 1000) + time.wMilliseconds;

			generator1.seed(time_ms);
			
			std::uniform_real_distribution<float> uniformDistribution(0, 1);
			int i = select_noise_index(uniformDistribution(generator1)) - 1;
			float * trial = new float[(size_t)dimension + 1];
			float * noise = new float[dimension];
			rand1(i, noise, generator1,x);
			exp_cross(i, trial, noise,x);

			for (int ix = 0; ix <= dimension; ix++) {
				new_pop[x + ix * popSize] = trial[ix];
			}

			delete[] trial;
			delete[] noise;
		});
				
		population = new_pop; 
	}
}

void DE::select_parents(mt19937 generator, int *r1, int *r2, int *r3, int active_parent) {
	
	std::uniform_real_distribution<float> uniformDistribution(0, (float)popSize);

		do {
			*r1 =(int) uniformDistribution(generator);
			*r2 = (int)uniformDistribution(generator);
		} while ((*r2 == *r1) || (*r2 == active_parent) || (*r1 == active_parent));
	
		do {
			*r3 = (int)uniformDistribution(generator);
		} while ((*r3 == active_parent) || (*r3 == *r2) || (*r3 == *r1));
}

void DE::exp_cross(int noiseIndex, float* trial, float* noise, int active_parent) {

	for (int i = 1; i <= dimension; i++) {

		

		if (i <= noiseIndex) {
			if (noise[i - 1]<minimum || noise[i - 1]>maximum) { trial[i] = population[active_parent + i * popSize]; }
			else { trial[i] = noise[i - 1]; }
		}
		else {
			trial[i] = population[active_parent + i * popSize];
		}
	}

	
	float trialEnergy = cost_function(trial, 0, 1);
	float parentEnergy = cost_function(population, active_parent, popSize);

	
	if (trialEnergy <= parentEnergy) {
		trial[0] = trialEnergy;
	}
	else {
		trial[0] = parentEnergy;
		for (int i = 1; i <= dimension; i++) {
			trial[i] = population[active_parent + i * popSize];
		}
	}
}

void DE::rand1(int x, float * noise, mt19937 generator, int active_parent) {
	
	int r1, r2, r3;     

	select_parents(generator, &r1, &r2, &r3, active_parent);

#if defined(GPU)
	
	float parent_a[10];
	float parent_b[10];
	float parent_c[10];
	float factor_array[10] = { factor, factor, factor, factor, factor, factor, factor, factor, factor, factor };

	for (int j = 0; j < x; j += 10) {

		parent_a[0] = population[r1 + j * popSize];
		parent_b[0] = population[r2 + j * popSize];
		parent_c[0] = population[r3 + j * popSize];

		parent_a[1] = population[r1 + (j + 1)*popSize];
		parent_b[1] = population[r2 + (j + 1)*popSize];
		parent_c[1] = population[r3 + (j + 1)*popSize];

		parent_a[2] = population[r1 + (j + 2)*popSize];
		parent_b[2] = population[r2 + (j + 2)*popSize];
		parent_c[2] = population[r3 + (j + 2)*popSize];

		parent_a[3] = population[r1 + (j + 3)*popSize];
		parent_b[3] = population[r2 + (j + 3)*popSize];
		parent_c[3] = population[r3 + (j + 3)*popSize];

		parent_a[4] = population[r1 + (j + 4)*popSize];
		parent_b[4] = population[r2 + (j + 4)*popSize];
		parent_c[4] = population[r3 + (j + 4)*popSize];

		parent_a[5] = population[r1 + (j + 5)*popSize];
		parent_b[5] = population[r2 + (j + 5)*popSize];
		parent_c[5] = population[r3 + (j + 5)*popSize];

		parent_a[6] = population[r1 + (j + 6)*popSize];
		parent_b[6] = population[r2 + (j + 6)*popSize];
		parent_c[6] = population[r3 + (j + 6)*popSize];

		parent_a[7] = population[r1 + (j + 7)*popSize];
		parent_b[7] = population[r2 + (j + 7)*popSize];
		parent_c[7] = population[r3 + (j + 7)*popSize];

		parent_a[8] = population[r1 + (j + 8)*popSize];
		parent_b[8] = population[r2 + (j + 8)*popSize];
		parent_c[8] = population[r3 + (j + 8)*popSize];

		parent_a[9] = population[r1 + (j + 9)*popSize];
		parent_b[9] = population[r2 + (j + 9)*popSize];
		parent_c[9] = population[r3 + (j + 9)*popSize];

		array_view<float, 1> parent_a_view(10, parent_a);
		array_view<float, 1> parent_b_view(10, parent_b);
		array_view<float, 1> parent_c_view(10, parent_c);
		array_view<float, 1> result_view(10, noise);
		array_view<float, 1> factor_view(10, factor_array);
		
		parallel_for_each(
			result_view.extent,
			[=](index<1> idx) restrict(amp)
		{
			result_view[idx] = parent_a_view[idx] + (factor_view[idx] * (parent_b_view[idx] - parent_c_view[idx]));

		}
		);
		result_view.synchronize();
	}
#else  
	int i = 0;
	float x1f, x2f, x3f;

	for (; i < x - 7; i += 8) {

		float	x1_1 = population[r1 + i * popSize];
		float 	x2_1 = population[r2 + i * popSize];
		float 	x3_1 = population[r3 + i * popSize];

		float	x1_2 = population[r1 + (i + 1)*popSize];
		float 	x2_2 = population[r2 + (i + 1)*popSize];
		float 	x3_2 = population[r3 + (i + 1)*popSize];

		float	x1_3 = population[r1 + (i + 2)*popSize];
		float 	x2_3 = population[r2 + (i + 2)*popSize];
		float 	x3_3 = population[r3 + (i + 2)*popSize];

		float	x1_4 = population[r1 + (i + 3)*popSize];
		float 	x2_4 = population[r2 + (i + 3)*popSize];
		float 	x3_4 = population[r3 + (i + 3)*popSize];

		float	x1_5 = population[r1 + (i + 4)*popSize];
		float 	x2_5 = population[r2 + (i + 4)*popSize];
		float 	x3_5 = population[r3 + (i + 4)*popSize];

		float	x1_6 = population[r1 + (i + 5)*popSize];
		float 	x2_6 = population[r2 + (i + 5)*popSize];
		float 	x3_6 = population[r3 + (i + 5)*popSize];


		float	x1_7 = population[r1 + (i + 6)*popSize];
		float 	x2_7 = population[r2 + (i + 6)*popSize];
		float 	x3_7 = population[r3 + (i + 6)*popSize];


		float	x1_8 = population[r1 + (i + 7)*popSize];
		float 	x2_8 = population[r2 + (i + 7)*popSize];
		float 	x3_8 = population[r3 + (i + 7)*popSize];


		__m256 x2 = _mm256_set_ps(x2_1, x2_2, x2_3, x2_4, x2_5, x2_6, x2_7, x2_8);
		__m256 x3 = _mm256_set_ps(x3_1, x3_2, x3_3, x3_4, x3_5, x3_6, x3_7, x3_8);
		__m256 result = _mm256_sub_ps(x2, x3);

		__m256 factor_m = _mm256_set_ps(factor, factor, factor, factor, factor, factor, factor, factor);
		__m256 x1 = _mm256_set_ps(x1_1, x1_2, x1_3, x1_4, x1_5, x1_6, x1_7, x1_8);
		
		__m256 mul_m = _mm256_mul_ps(result, factor_m);
		__m256 add_m = _mm256_add_ps(x1, mul_m);


		memcpy(&noise[i], (float*)&add_m, 8 * sizeof(float));

		for (; i < x; i++) {

			x1f = population[r1 + i*popSize];
			x2f = population[r2 + i*popSize];
			x3f = population[r3 + i*popSize];
			noise[i] = x1f + factor * (x2f - x3f);
		}
	}

#endif


}


int DE::select_noise_index(float rand) {

	int noiseIndex;
	
	for (noiseIndex = 0; (noiseIndex <= dimension) && (rand < crossover); noiseIndex++)
	{
	}

	return noiseIndex;
}

float DE::cost_function(float * testPopulation, size_t index, int popSiz) {
	float costValue = 0;
	
	byte value[sizeof(TPassword)];

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		value[i] = (byte)testPopulation[i + index * (size_t)dimension];
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

float* DE::get_best(float &cost) {

	float best_act = 1000000; 
	size_t best_index = 0;
	
	for (size_t i = 0; i < popSize; i++)
	{
		float tmp = cost_function(population, i, popSize);
		if (tmp < best_act) {
			best_act = tmp;
			best_index = i;
		}
	}

	byte value[sizeof(TPassword)];

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		value[i] = (byte)population[i + best_index * (size_t)dimension];
	}


	SJ_context context;
	TBlock decrypted;
	std::cout << "Score: " << best_act << '\n';

	makeKey(&context, value, sizeof(TPassword));
	decrypt_block(&context, decrypted, encrypted);

	cost = best_act;

	return &population[best_index * (size_t)dimension];
}
