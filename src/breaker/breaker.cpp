#include "../breaker.h"

//V ramci vypracovani semestralni prace muzete menit pouze a jedine tento souboru od teto radky dale.


#include <memory.h>
#include <iostream>
#include "DE.h"

bool break_the_cipher(TBlock &encrypted, const TBlock &reference, TPassword &password) {
	SJ_context context;
	TBlock decrypted;
	TPassword testing_key{ 0 };
	
	DE de(0.3, 128, 1, 10000, sizeof(password), encrypted, reference);

	de.init(0,255,0);
	de.evolve();
	
	float cost;
	float* result = de.getBest(cost);

	byte value[sizeof(TPassword)];

	for (size_t i = 0; i < sizeof(TPassword); i++)
	{
		value[i] = (byte)result[i];
	}

	if (cost == 0) {
		memcpy(password, value, sizeof(TPassword));
		return true;
	}
	/** for (testing_key[0] = 0; testing_key[0] < 255; testing_key[0]++) {
		makeKey(&context, testing_key, sizeof(TPassword));
		decrypt_block(&context, decrypted, encrypted);
		if (memcmp(decrypted, reference, sizeof(TBlock)) == 0) {
			memcpy(password, testing_key, sizeof(TPassword));
			return true;
		}
	}**/

	std::cout << "Score: " << cost << '\n';
	
	int tmp;
	cin >> tmp;

	return false;
}