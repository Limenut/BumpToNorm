#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "SDL2_image.lib")

#ifdef main
#undef main
#endif

class vector3D
{
public:
	double x;
	double y;
	double z;

	void normalize();
	double lenght();

	vector3D operator+(const vector3D& vec)
	{
		vector3D result;
		result.x = this->x + vec.x;
		result.y = this->y + vec.y;
		result.z = this->z + vec.z;
		return result;
	}
	vector3D operator/(double val)
	{
		vector3D result;
		result.x = this->x / val;
		result.y = this->y / val;
		result.z = this->z / val;
		return result;
	}
};

void vector3D::normalize()
{
	double length = lenght();
	x /= length;
	y /= length;
	z /= length;
}

double vector3D::lenght()
{
	return sqrt(x*x + y*y + z*z);
}

std::vector<vector3D> horiNormals;
std::vector<vector3D> vertiNormals;
std::vector<vector3D> westNormals;
std::vector<vector3D> eastNormals;

bool init()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	//Initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	return true;
}

void close()
{
	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

void constructNormals(SDL_Surface *surf, double depth)
{
	int width = surf->w;
	int height = surf->h;

	Uint32* pixels = (Uint32 *)surf->pixels;

	std::vector<Uint8> values;
	for (int i = 0; i < width*height; i++)
	{
		Uint8 val = Uint8((pixels[i] & surf->format->Rmask) / 0x1000000);	//uses only red color channel, shift to 0-255 range
		values.push_back(val);
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width - 1; x++)
		{
			double val1 = values[width*y + x];
			double val2 = values[width*y + x + 1];
			
			vector3D vec = { val1 - val2, 0, depth };
			vec.normalize();
			horiNormals.push_back(vec);
		}
	}
	for (int y = 0; y < height - 1; y++)
	{
		for (int x = 0; x < width; x++)
		{
			double val1 = values[width*y + x];
			double val2 = values[width*(y+1) + x];

			vector3D vec = { 0, val1 - val2, depth };
			vec.normalize();
			vertiNormals.push_back(vec);
		}
	}
	for (int y = 0; y < height - 1; y++)
	{
		for (int x = 0; x < width - 1; x++)
		{
			double val1 = values[width*y + x];
			double val2 = values[width*(y+1) + x + 1];

			vector3D vec = { (val1 - val2) / sqrt(2), (val1 - val2) / sqrt(2), depth*sqrt(2) };
			vec.normalize();
			westNormals.push_back(vec);
		}
	}
	for (int y = 0; y < height - 1; y++)
	{
		for (int x = 0; x < width - 1; x++)
		{
			double val1 = values[width*y + x + 1];
			double val2 = values[width*(y+1) + x];

			vector3D vec = { (val2 - val1) / sqrt(2), (val1 - val2) / sqrt(2), depth*sqrt(2) };
			vec.normalize();
			eastNormals.push_back(vec);
		}
	}
}

vector3D getAverageNormal_b2n(SDL_Surface *surf, int x, int y)
{
	int height = surf->h;
	int width = surf->w;
	vector3D finalVec = {0,0,0};

	if (y > 0)								//top
	{
		finalVec = finalVec + vertiNormals[width*(y - 1) + x];
	}
	if (x > 0)								//left
	{
		finalVec = finalVec + horiNormals[(width-1)*y + x - 1];
	}
	if (x < width - 1)						//right
	{
		finalVec = finalVec + horiNormals[(width-1)*y + x];
	}
	if (y < height - 1)						//bottom
	{
		finalVec = finalVec + vertiNormals[width*y + x];
	}

	if (y > 0 && x > 0)						//top left
	{
		finalVec = finalVec + (westNormals[(width-1)*(y - 1) + x - 1] / sqrt(2));
	}
	if (y < height - 1 && x < width - 1)	//bottom right
	{
		finalVec = finalVec + (westNormals[(width-1)*y + x] / sqrt(2));
	}
	if (y > 0 && x < width - 1)				//top right
	{
		finalVec = finalVec + (eastNormals[(width-1)*(y - 1) + x] / sqrt(2));
	}
	if (y < height - 1 && x > 0)			//bottom left
	{
		finalVec = finalVec + (eastNormals[(width-1)*y + x - 1] / sqrt(2));
	}


	finalVec.normalize();
	return finalVec;
}

int main(int argc, char *argv[])
{
	init();
	SDL_Surface *bumpSurf = nullptr;
	
	double depth = 0.5;

	for (int i = 1; i < argc; i++)
	{	
		std::string fileName(argv[i]);
		if (!(bumpSurf = IMG_Load(argv[i])))
		{
			std::cout << "Failed to load image " << fileName << std::endl;
			continue;
		}		
		std::cout << "processing " << fileName << "...";

		bumpSurf = SDL_ConvertSurfaceFormat(bumpSurf, SDL_PIXELFORMAT_RGBA8888, 0);	//convert to better format

		//get pixel values into valueMap
		constructNormals(bumpSurf, depth);

		SDL_Surface *normalSurf = SDL_CreateRGBSurface
			(
				0,
				bumpSurf->w,
				bumpSurf->h,
				24,
				0xff000000,
				0x00ff0000,
				0x0000ff00,
				0x000000ff
				);

		//set pixels of new texture
		Uint32* pixels = (Uint32 *)normalSurf->pixels;
		for (int y = 0; y < bumpSurf->h; y++)
		{
			for (int x = 0; x < bumpSurf->w; x++)
			{
				vector3D normal = getAverageNormal_b2n(bumpSurf, x, y);
				Uint8 red = Uint8((normal.x + 1.0) / 2 * 255.0 + 0.5);
				Uint8 green = Uint8((normal.y + 1.0) / 2 * 255.0 + 0.5);
				Uint8 blue = Uint8((normal.z + 1.0) / 2 * 255.0 + 0.5);

				pixels[y*bumpSurf->w + x] = red * 0x1000000 + green * 0x10000 + blue * 0x100;
			}
		}

		normalSurf = SDL_ConvertSurfaceFormat(normalSurf, SDL_PIXELFORMAT_RGB888, 0);	//remove alpha channel

		//modify file name and save result
		size_t t = fileName.find_last_of(".");
		fileName.erase(t);
		fileName += "_b2n.bmp";
		SDL_SaveBMP(normalSurf, fileName.c_str());
		std::cout << "\tdone" << std::endl;

		//cleanup
		horiNormals.clear();
		vertiNormals.clear();
		westNormals.clear();
		eastNormals.clear();
		SDL_FreeSurface(bumpSurf);
		SDL_FreeSurface(normalSurf);
		bumpSurf = nullptr;
		normalSurf = nullptr;
	}

	close();
}