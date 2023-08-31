#ifndef NOISE_H_
#define NOISE_H_

namespace noise {

//max pixelMap value = 2(noiseLimiter)*(pixelsPerChunk -1); number of bits used for value and size of noiseLimiter should compensate for this
template <typename value>
class Noise {
private:
	long int xSideLength; //(in pixels)
	long int ySideLength;

	//how many samples will be generated. If you want to generate 1 sample for an nxn grid of tiles the conversion is floor(coord-value/n)
	long int topLeftXCoord;
	long int topLeftYCoord;

	//essentially imposes a ppc by ppc grid on top of underlying "map"
	long int  pixelsPerChunk; //The larger this value is the less noise affects result (fewer random samples per same region of nxn pixels)
	long int seed;

	unsigned int noiseLimiter; //Simple non-inclusive limiter on the value each vertex is generated with. Should be no less than 2.
	unsigned short int octaveCount; //How many octaves of noise you want stacked on top of eachother. Frequency increases by 2 each level, with amplitude decreasing by 2 as well. Limited to [1, 7]

	value *ppixelMap;

	void fillPixelMap();

public:
	Noise(long int topLeftXCoord, long int topLeftYCoord, long int bottomRightXCoord, long int bottomRightYCoordlong,
			  long int  pixelsPerChunk, long int seed, unsigned int noiseLimiter, unsigned short int octaveCount);

	~Noise();

	value getPixelValue(long int topLeftPixelXCoord, long int topLeftPixelYCoord);
};

//---------------------------Implementation starts------------------------------------------//

int sign(int x) {
	return ((x < 0) ? (-1) : (1));
}

template <typename value>
Noise<value>::Noise
		(long int topLeftXCoord, long int topLeftYCoord,
		long int bottomRightXCoord, long int bottomRightYCoord,
		long int  pixelsPerChunk, long int seed,
		unsigned int noiseLimiter, unsigned short int octaveCount):

		topLeftXCoord(topLeftXCoord),topLeftYCoord(topLeftYCoord),
		pixelsPerChunk(pixelsPerChunk), seed(seed), noiseLimiter(noiseLimiter){

		xSideLength = (bottomRightXCoord - topLeftXCoord);
		ySideLength = (topLeftYCoord - bottomRightYCoord);

		this->octaveCount = (octaveCount) ? (octaveCount & 7) : (1); //make sure octaveCount is on range [1, 7] (lacunarity always two, not worth time changing I reckon)
		ppixelMap = new value[xSideLength * ySideLength];

		this->fillPixelMap();
}

template <typename value>
Noise<value>::~Noise() {
	delete[] ppixelMap;
}

/*
 * Generally the terminology is pixels as the underlying grid based on the provided corner coordinates, with each pixel
 * in its own chunk of ppc by ppc pixels. Vertices are generally the vertices of this ppc by ppc grid that is imposed upon the underlying map
 * of pixels.
 */
template <typename value>
void Noise<value>::fillPixelMap() {

	//determines how far the top left x and y coordinates stick out (up and to the left) from the nearest South Eastern vertex.
	long int xPadding = pixelsPerChunk - ( (topLeftXCoord < 0) ? ( (topLeftXCoord *(-1)) % pixelsPerChunk) : (topLeftXCoord % pixelsPerChunk) );
	long int yPadding = pixelsPerChunk - ( (topLeftYCoord < 0) ? ( (topLeftYCoord *(-1)) % pixelsPerChunk) : (topLeftYCoord % pixelsPerChunk) );

	struct vertex {
		long int xComp;
		long int yComp;
	};

	//trim padding
	if (xPadding == pixelsPerChunk) {xPadding = 0;}
	if (yPadding == pixelsPerChunk) {yPadding = 0;}

	//Determine how many vertices are needed in a row based on possible left/right overflow into adjacent, but not fully occupied, chunks.
	long int vertexCount = 1 + ((xSideLength - xPadding) % pixelsPerChunk) ? ((xSideLength / pixelsPerChunk) + 1) : (xSideLength /pixelsPerChunk) + ( (xPadding == 0) ? (0) : (1) );

	//old code for reference
	//long int  vertexCount = 1 + ((xPadding == pixelsPerChunk) ? ( (xSideLength % pixelsPerChunk) ? (xSideLength /pixelsPerChunk +1) : (xSideLength /pixelsPerChunk) )
	//								: ( ((xSideLength - xPadding) % pixelsPerChunk) ? ((xSideLength /pixelsPerChunk) +1) : (xSideLength /pixelsPerChunk) +1) );

	vertex* vertices = new vertex[2 * vertexCount];

	//initialize the first and second row (i = row, j = column)
	for (int i = 0; i < 2; ++i) {
		for (unsigned int j = 0; j < vertexCount; ++j) {
			long int currentX = ((topLeftXCoord - xPadding) + (pixelsPerChunk *j));
			long int currentY = (topLeftYCoord + yPadding) - (pixelsPerChunk *i);
			(vertices + (i *vertexCount + j))->xComp = (seed *sign(currentX) *sign(currentY) + (currentX * (sign(currentY) * currentY)) ) % noiseLimiter;
			(vertices + (i *vertexCount + j))->yComp = (seed *sign(currentX) *sign(currentY) + (currentY * (sign(currentX) * currentX)) ) % noiseLimiter;
		}
	}

	//first two rows already initialized by default, thus rowCounter = 2.
	long int rowCounter = 2;
	char vertexIndex;

	//stores absolute (positional) coordinate of any given target vertex. These vertices are part of the imposed ppc by ppc grid
	long int vertexX;
	long int vertexY;

	//store the absolute (positive) position of a pixel's top left corner relative to its chunk's top left corner; on range [0, pixelsPerChunk -1]
	long int pixelX;
	long int pixelY;

	//stores the two components used to randomize dot product of each pixel
	vertex topLeft;
	vertex topRight;
	vertex bottomRight;
	vertex bottomLeft;

	//stores values related to the dot product to avoid having to re-calculate and to make things clearer (probably not very clear anyway, sorry)
	long int tlDotProduct;
	long int trDotProduct;
	long int brDotProduct;
	long int blDotProduct;

	//-------------------------------- BEGIN MAP TRAVERSING ------------------------------------------------------------------------------

	//traverses every row
	for (long int i = yPadding; i < ySideLength + yPadding; ++i) {
		//if crossing into new chunk by row
		if (!(i % pixelsPerChunk)) {

			//initialize two vertex rows in use, indicate which one is top through vertexIndex. (vertexIndex = index of "top" row)
			vertexIndex = rowCounter & 1;
			vertexY = (topLeftYCoord + yPadding) - (pixelsPerChunk *(rowCounter));

			for (unsigned int j = 0; j < vertexCount; ++j) {
				//first vertex's Xcoord is the nearest TL vertex from TL Xcoord, hence it must shift xPadding units from TL xCoord to account for mis-aligned corner coordinates
				vertexX = ((topLeftXCoord - xPadding) + (pixelsPerChunk *j));
				(vertices + (vertexIndex *vertexCount + j))->xComp = (seed *sign(vertexX) *sign(vertexY) + (vertexX * (sign(vertexY) * vertexY)) ) % noiseLimiter;
				(vertices + (vertexIndex *vertexCount + j))->yComp = (seed *sign(vertexX) *sign(vertexY) + (vertexY * (sign(vertexX) * vertexX)) ) % noiseLimiter;
			}

			//indicate another row has been generated
			++rowCounter;
		}

		//reset vertices in use back to those of left most chunk. topIndex XOR 1 -> notTop = bottom. + 1 => one further in row
		topLeft = *(vertices + (vertexIndex *vertexCount) );
		topRight = *(vertices + (vertexIndex *vertexCount + 1) );
		bottomRight = *(vertices + ((vertexIndex ^1) *vertexCount + 1) );
		bottomLeft = *(vertices + ((vertexIndex ^1) *vertexCount) );

		//reset variables to reflect shift back to left farthest column of next row
		long int chunkCount = 0;
		pixelX = 0;
		pixelY = 0;

		//traverses every column in a row
		for (long int j = xPadding; j < xSideLength + xPadding; ++j) {
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
				long int octaveX = pixelX * 1 <<k;
				if (octaveX > pixelsPerChunk) { octaveX %= pixelsPerChunk; }

				long int octaveY = pixelY * 1 <<k;
				if (octaveY > pixelsPerChunk) { octaveY %= pixelsPerChunk; }

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
value Noise<value>::getPixelValue(long int topLeftPixelXCoord, long int topLeftPixelYCoord){
	//Throw error if out of bounds areas are requested
	value *ptarget = ppixelMap + ((topLeftYCoord - topLeftPixelYCoord) * pixelsPerChunk) + (topLeftPixelXCoord - topLeftXCoord);

	//check for out of bounds
	if (ptarget > (ppixelMap + (xSideLength * ySideLength) -1) || ptarget < ppixelMap) {
		return 0;
	}

	//adjust input coordinates to fit 1d array index
	return *ptarget;
}

}




#endif /* SRC_Noise_H_ */
