#include "../breaker.h"

//V ramci vypracovani semestralni prace muzete menit pouze a jedine tento souboru od teto radky dale.


#include <memory.h>
#include <iostream>
#include "DE.h"

bool break_the_cipher(TBlock &encrypted, const TBlock &reference, TPassword &password) {
	TPassword testing_key{ 0 };
	
	DE de(0.6f, 128, 1, 10000000, sizeof(password), encrypted, reference);

	de.init(0,255);
	de.evolve();
	
	float cost;
	float* result = de.get_best(cost);

	byte value[sizeof(TPassword)];

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		value[i] = (byte)result[i];
	}

	if (cost == 0) {
		memcpy(password, value, sizeof(TPassword));
		return true;
	}
	std::cout << "Score: " << cost << '\n';
	int tmp;
	std::cin >> tmp;
	return false;
}