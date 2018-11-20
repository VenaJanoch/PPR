#pragma once


#include <random>
#include "../skipjack.h"
#include <tbb/tbb.h>

class DE
{
public:
	
	DE(float CR, int NP, float F, int generations, int D, TBlock &encrypted, const TBlock &reference);
	virtual ~DE();


	void init(float mini, float maxi); 
	void set_population();          

	void evolve();  
	void exp_cross(int i, float* trial, float* noise, int active_parent);
	void rand1(int i, float* noise,	std::mt19937 generator, int active_parent);

	float* get_best(float &cost);
	int select_noise_index(float rand); 
	float cost_function(float * testPopulation, size_t index, int populationSize);

protected:
	float * population;
	float * new_pop;    

	void select_parents(std::mt19937 generator, int *r1, int *r2, int *r3, int active_parent);

private:
	float crossover;   
	int popSize;       
	float factor;      
	int generations;   
	int dimension;      
	
	float minimum = 10000;
	float maximum = -1;;
	int active_parent = -1;   
	float bestCV = 100000;      

	byte* encrypted;
	const byte* reference;
};