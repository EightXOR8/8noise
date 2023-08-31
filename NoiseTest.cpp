#include <iostream>

#include "Noise.h"

int main() {
	long int topLeftXCoord = -4;
	long int topLeftYCoord = 4;
	long int bottomRightXCoord = 4;
	long int bottomRightYCoord = -4;
	long int pixelsPerChunk = 2;
	long int seed = 102943587;
	unsigned int noiseLimiter = 4096;
	unsigned short int octaveCount = 2;

	/*
	long int xSideLength = bottomRightXCoord - topLeftXCoord;
	long int ySideLength = topLeftYCoord - bottomRightYCoord;

	long int targetXCoord = topLeftXCoord;
	long int targetYCoord = topLeftYCoord;

	long int xChunkPosition = topLeftXCoord % pixelsPerChunk;
	long int yChunkPosition = topLeftYCoord % pixelsPerChunk;
	*/

	noise::Noise<char> noiseMap(topLeftXCoord, topLeftYCoord, bottomRightXCoord, bottomRightYCoord, pixelsPerChunk, seed, noiseLimiter, octaveCount);

	//iterate over rows
	for (long int y = topLeftYCoord; y > bottomRightYCoord; --y) {
		//iterate over columns
		for (long int x = topLeftXCoord; x < bottomRightXCoord; ++x) {
			std::cout << (int)noiseMap.getPixelValue(x, y) << " ";
		}

		std::cout << "\n";
	}

	return 0;
}
