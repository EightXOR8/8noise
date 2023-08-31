//#include <iostream>
#include "EightNoise.h"

int main() {
	long int pixelsPerChunk = 2;
	long int topLeftXCoord = -1;
	long int topLeftYCoord = 1;
	long int bottomRightXCoord = 0;
	long int bottomRightYCoord = 0;

	unsigned long int xSideLength = bottomRightXCoord - topLeftXCoord;
	unsigned long int ySideLength = topLeftYCoord - bottomRightYCoord;

	long int targetXCoord = topLeftXCoord;
	long int targetYCoord = topLeftYCoord;

	unsigned long int xChunkPosition = topLeftXCoord % pixelsPerChunk; // conversion from -1 to unsigned yields errors
	unsigned long int yChunkPosition = topLeftYCoord % pixelsPerChunk;
 
	noise::EightNoise<char> noiseMap = noise::EightNoise<char>(topLeftXCoord, topLeftYCoord, bottomRightXCoord, bottomRightYCoord,
			  pixelsPerChunk, 34590325, 8, 1);

	std::cout << "xSideLength: " << xSideLength << "\nySideLength: " << ySideLength << std::endl;
	std::cout << "xChunkPosition: " << xChunkPosition << "\nyChunkPosition: " << yChunkPosition << std::endl;

	unsigned long int xPos = 0;
	/*
	while (targetXCoord < bottomRightXCoord -1 || targetYCoord > bottomRightYCoord) {
		
	}
	*/
	return 0;
}
