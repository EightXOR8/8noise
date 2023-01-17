#ifndef EIGHTNOISE_H_
#define EIGHTNOISE_H_

#include <iostream>

namespace noise {

//max pixelMap value = 2(2^noiseMultiplier)*(pixelsPerChunk -1); size used for value should compensate for this
template <typename value>
class EightNoise {
private:
	unsigned long int xSideLength; //(in pixels)
	unsigned long int ySideLength;

	long int topLeftXCoord;
	long int topLeftYCoord;

	unsigned long int pixelsPerChunk; //Generally best to keep relatively small. The larger this value is the less noise effects result
	long int seed;

	unsigned short int noiseMultiplier; //Noise values are limited to 2^noiseMultiplier -1 (0 -> 2^0). The higher the value the more sway noise can have. Limited to [0, 31], but noise limit is always 2-2^31.
	unsigned short int octaveCount; //How many octaves of noise you want stacked on top of eachother. Frequency increases by 2 each level, with amplitude decreasing by 2 as well. Limited to [1, 7]

	value *ppixelMap;

	void fillPixelMap();

public:
	EightNoise(long int topLeftXCoord, long int topLeftYCoord, long int bottomRightXCoord, long int bottomRightYCoordlong,
			  unsigned long int pixelsPerChunk, long int seed, unsigned short int noiseMultiplier, unsigned short int octaveCount);

	~EightNoise();

	value getPixelValue(long int topLeftPixelXCoord, long int topLeftPixelYCoord);
};

//---------------------------Implementation starts------------------------------------------//

int sign(int x) {
	return ((x < 0) ? (-1) : (1));
} 

template <typename value>
EightNoise<value>::EightNoise
		(long int topLeftXCoord, long int topLeftYCoord,
		long int bottomRightXCoord, long int bottomRightYCoord,
		unsigned long int pixelsPerChunk, long int seed,
		unsigned short int noiseMultiplier, unsigned short int octaveCount):

		topLeftXCoord(topLeftXCoord),topLeftYCoord(topLeftYCoord),
		pixelsPerChunk(pixelsPerChunk), seed(seed){

		xSideLength = (bottomRightXCoord - topLeftXCoord);
		ySideLength = (topLeftYCoord - bottomRightYCoord);

		//The higher the noiseMultiplier the more noise effects the output. Limited to 31 to prevent overflow while also allowing significant noise influence.
		this->noiseMultiplier = noiseMultiplier & 31;
		this->octaveCount = (octaveCount) ? (octaveCount & 7) : (1); //make sure octaveCount is on range [1, 7] (lacunarity always two, not worth time changing I reckon)
		ppixelMap = new value[xSideLength * ySideLength];

		this->fillPixelMap();
}

template <typename value>
EightNoise<value>::~EightNoise() {
	delete[] ppixelMap;
}

template <typename value>
void EightNoise<value>::fillPixelMap() {

	//determines how far the top left x and y coordinates stick out (up and to the left) from the nearest (specifically bottom right) vertex.
	unsigned long int xPadding = pixelsPerChunk - ( (topLeftXCoord < 0) ? ((topLeftXCoord *(-1)) % pixelsPerChunk) : (topLeftXCoord % pixelsPerChunk) );
	unsigned long int yPadding = pixelsPerChunk - ( (topLeftYCoord < 0) ? ((topLeftYCoord *(-1)) % pixelsPerChunk) : (topLeftYCoord % pixelsPerChunk) );

	struct vertex {
		long int xComp;
		long int yComp;
	};

	//Determines how many vertices will be needed in each row to calculate noise for all pixels in region based upon how many different chunks are intersected.
	unsigned long int vertexCount = 1 + ((xPadding == pixelsPerChunk) ? ((xSideLength % pixelsPerChunk) ? (xSideLength /pixelsPerChunk +1) : (xSideLength /pixelsPerChunk))
									: (((xSideLength - xPadding) % pixelsPerChunk) ? ((xSideLength /pixelsPerChunk) +1) : (xSideLength /pixelsPerChunk) +1));

	vertex* vertices = new vertex[2 * vertexCount];

	//trim padding if needed
	if (xPadding == pixelsPerChunk) {xPadding = 0;}
	if (yPadding == pixelsPerChunk) {yPadding = 0;}

	//Create power of two to & by to limit noise values (on range [2^4 -1, 2^35 -1]); likely best to use (#bits in value - #bits in ppc - 4).
	unsigned long int noiseLimiter = 1;
	for (int i = 0; i < noiseMultiplier; ++i) {
		noiseLimiter *= 2;
	}

	if (!(noiseLimiter == 1)) {--noiseLimiter;}

	//initialize the first and second row
	for (int i = 0; i < 2; ++i) {
		for (unsigned int j = 0; j < vertexCount; ++j) {
			long int currentX = ((topLeftXCoord - xPadding) + (pixelsPerChunk *j));
			long int currentY = (topLeftYCoord + yPadding) - (pixelsPerChunk *i);
			(vertices + (i *vertexCount + j))->xComp = (seed *sign(currentX) *sign(currentY) + (currentX * (sign(currentY) * currentY)) ) % noiseLimiter;
			(vertices + (i *vertexCount + j))->yComp = (seed *sign(currentX) *sign(currentY) + (currentY * (sign(currentX) * currentX)) ) % noiseLimiter;
		}
	}

	//first two rows already initialized by default, thus rowCounter = 2.
	unsigned long int rowCounter = 2;
	char vertexIndex;

	long int vertexX;
	long int vertexY;

	//store the absolute position of a pixel's top left corner relative to its chunk's top left corner. [0, pixelsPerChunk -1]
	unsigned long int pixelX;
	unsigned long int pixelY = 0;

	//stores the two components used to randomize dot product of each pixel
	vertex topLeft;
	vertex topRight;
	vertex bottomRight;
	vertex bottomLeft;

	long int tlDotProduct;
	long int trDotProduct;
	long int brDotProduct;
	long int blDotProduct;

	//-------------------------------- BEGIN MAP TRAVERSING ------------------------------------------------------------------------------

	//traverses every row
	for (unsigned long int i = yPadding; i < ySideLength + yPadding; ++i) {
		//if crossing into new chunk by row
		if (!(i % pixelsPerChunk)) {

			//initialize two vertex rows in use, indicate which one is top through vertexIndex. (vertexIndex = index of "top" row)
			vertexIndex = rowCounter & 1;
			vertexY = (topLeftYCoord + yPadding) - (pixelsPerChunk *(rowCounter));

			for (unsigned int j = 0; j < vertexCount; ++j) {
				vertexX = ((topLeftXCoord - xPadding) + (pixelsPerChunk *j));
				(vertices + (vertexIndex *vertexCount + j))->xComp = (seed *sign(vertexX) *sign(vertexY) + (vertexX * (sign(vertexY) * vertexY)) ) % noiseLimiter;
				(vertices + (vertexIndex *vertexCount + j))->yComp = (seed *sign(vertexX) *sign(vertexY) + (vertexY * (sign(vertexX) * vertexX)) ) % noiseLimiter;
			}

			//indicate another row has been generated
			++rowCounter;
		}

		//reset vertices in use back to those of left most chunk. topIndex XOR 1 -> notTop = bottom. + 1 => one further in row
		topLeft = *(vertices + (vertexIndex *vertexCount));
		topRight = *(vertices + (vertexIndex *vertexCount + 1));
		bottomRight = *(vertices + ((vertexIndex ^1) *vertexCount + 1));
		bottomLeft = *(vertices + ((vertexIndex ^1) *vertexCount));

		//reset variables to reflect shift back to left farthest column of next row
		unsigned long int chunkCount = 0;
		pixelX = 0;
		pixelY = 0;

		//traverses every column in a row
		for (unsigned long int j = xPadding; j < xSideLength + xPadding; ++j) {
			if (!(j % pixelsPerChunk)) {
				++chunkCount;

				//simplifies accessing correct vertices by storing them in four vertex variables; steps through vertices[][] after each chunk is traversed.
				topLeft = *(vertices + (vertexIndex *vertexCount + chunkCount));
				topRight = *(vertices + (vertexIndex *vertexCount + chunkCount + 1));
				bottomRight = *(vertices + ((vertexIndex ^1) *vertexCount + chunkCount + 1));
				bottomLeft = *(vertices + ((vertexIndex ^1) *vertexCount + chunkCount));

				pixelX = 0;
			}

			*(ppixelMap + (pixelY * xSideLength + pixelX)) = 0;

			//Generates noise octaves, those being the same as the noise before them, but with 2^k frequency and 1/k amplitude. Higher frequency = pixel(x, Y) *K wrapped around ppc.
			for (unsigned short int k = 0; k < octaveCount; k++) {

				//amplify frequency by 2^k, wrap around pixelsPerChunk
				unsigned long int octaveX = pixelX * 1 <<k;
				if (octaveX > pixelsPerChunk) { octaveX %= pixelsPerChunk;}

				unsigned long int octaveY = pixelY * 1 <<k;
				if (octaveY > pixelsPerChunk) { octaveY %= pixelsPerChunk;}

				//generate values for pixel dot product; dot product on range [-2*noiseLimiter * (pixelsPerChunk -1), 2*noiseLimiter * (pixelsPerChunk -1)]
				//pixel positions bit-shifted by 7 to give them 128x more weight than noise (the idea is to make it similar to as if noise was two decimal places lower than pixel order
				tlDotProduct = (topLeft.xComp * (octaveX<<7) + topLeft.yComp * (octaveY<<7) ) >> k;
				trDotProduct = (topRight.xComp * (pixelsPerChunk - ((octaveX<<7) + 1) ) + topRight.yComp * (octaveY<<7) ) >> k;
				brDotProduct = (bottomRight.xComp * (pixelsPerChunk - ((octaveX<<7) + 1) ) + bottomRight.yComp * (pixelsPerChunk - ((octaveY<<7) + 1) )  ) >> k;
				blDotProduct = (bottomLeft.xComp * (octaveX<<7) + bottomLeft.yComp * (pixelsPerChunk - ((octaveY<<7) + 1) )  ) >> k;

				//smooth step + lerp(I wish I didn't have to use doubles, but there is nothing else i can think of which is worth the time and better in performance)
				double fadeValue = (double)octaveX / pixelsPerChunk - 0.001;
				double smoothLerpTop;
				double smoothLerpBottom;
				fadeValue = ((( ((6*fadeValue - 15) * fadeValue) + 10) * fadeValue * fadeValue) + 0.001) * fadeValue - 0.001; //floating point inaccuracy+++++++++++

				smoothLerpTop = tlDotProduct + fadeValue * (trDotProduct - tlDotProduct);
				smoothLerpBottom = blDotProduct + fadeValue * (brDotProduct - blDotProduct);

				//floating point my belovn't
				fadeValue = (double)octaveY / pixelsPerChunk - 0.001;
				fadeValue = ((( ((6*fadeValue - 15) * fadeValue) + 10) * fadeValue * fadeValue) + 0.001) * fadeValue - 0.001;

				*(ppixelMap + (pixelY * xSideLength + pixelX)) += (value)(smoothLerpTop + fadeValue * (smoothLerpBottom - smoothLerpTop));

			}
			++pixelX; //increment pixelX position
		}
		++pixelY; //increment pixelY position
	}

	delete[] vertices;
}

template <typename value>
value EightNoise<value>::getPixelValue(long int topLeftPixelXCoord, long int topLeftPixelYCoord){
	//Throw error if out of bounds areas are requested
	value *ptarget = ppixelMap + ((topLeftYCoord - topLeftPixelYCoord) * pixelsPerChunk) + (topLeftPixelXCoord - topLeftXCoord);

	if (ptarget > (ppixelMap + (xSideLength * ySideLength) -1) || ptarget < ppixelMap) {
		std::cout << "Accessing out of bounds values" << std::endl;
	}

	//adjust input coordinates to fit 1d array index
	return *ptarget;
}

}

#endif /* EIGHTNOISE_H_ */
