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

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;

SDL_Surface *bumpSurf;
SDL_Surface *normalSurf;

std::vector<Uint8> valueMap;

std::vector<vector3D> horiNormals;
std::vector<vector3D> vertiNormals;
std::vector<vector3D> horiNormals;
std::vector<vector3D> horiNormals;

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

bool init()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	//Initialize renderer color
	SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

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
	//Destroy global things	
	if (gRenderer)
	{
		SDL_DestroyRenderer(gRenderer);
	}

	if (gWindow)
	{
		SDL_DestroyWindow(gWindow);
	}
	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

void extractPixels(SDL_Surface *surf)
{
	Uint32* pixels = (Uint32 *)surf->pixels;
	
	//807934 = magic number
	for (int i = 0; i < surf->w*surf->h; i++)
	{
		Uint8 val = Uint8((pixels[i] & surf->format->Rmask) / 0x1000000);	//uses only red color channel, shift to 0-255 range
		valueMap.push_back(val);
		//std::cout << std::hex << (surf->format->Rmask / 0x1000000) << std::endl;
	}
}

vector3D getAverageNormal(int x, int y, double depth)
{
	double tl;		//topleft
	double tm;		//topmiddle
	double tr;		//topright
	double ml;		//middleleft
	double mid;		//middle
	double mr;		//middleright
	double bl;		//bottomleft
	double bm;		//bottommiddle
	double br;		//bottomright

	std::vector<vector3D> normals;
	std::vector<vector3D> dia_normals;

	mid = valueMap[bumpSurf->w*y + x];
	if (y > 0)
	{
		tm = valueMap[bumpSurf->w*(y - 1) + x];
		vector3D vec = { 0, tm - mid, depth };
		normals.push_back(vec);

		if (x > 0)
		{
			tl = valueMap[bumpSurf->w*(y - 1) + x - 1];
			vector3D vec = { (tl - mid)/sqrt(2), (tl - mid) / sqrt(2), depth*sqrt(2) }; //check math later
			dia_normals.push_back(vec);
		}
		if (x < bumpSurf->w - 1)
		{
			tr = valueMap[bumpSurf->w*(y - 1) + x + 1];
			vector3D vec = { (mid - tr)/sqrt(2), (tr - mid)/sqrt(2), depth*sqrt(2) }; //check math later
			dia_normals.push_back(vec);
		}
	}
	if (x > 0)
	{
		ml = valueMap[bumpSurf->w*y + x - 1];
		vector3D vec = { ml - mid, 0, depth };
		normals.push_back(vec);
	}
	if (x < bumpSurf->w - 1)
	{
		mr = valueMap[bumpSurf->w*y + x + 1];
		vector3D vec = { mid - mr, 0, depth };
		normals.push_back(vec);
	}
	if (y < bumpSurf->h - 1)
	{
		bm = valueMap[bumpSurf->w*(y + 1) + x];
		vector3D vec = { 0, mid - bm, depth };
		normals.push_back(vec);

		if (x > 0)
		{
			bl = valueMap[bumpSurf->w*(y + 1) + x - 1];
			vector3D vec = { (bl - mid) / sqrt(2), (mid - bl) / sqrt(2), depth*sqrt(2) }; //check math later
			dia_normals.push_back(vec);
		}
		if (x < bumpSurf->w - 1)
		{
			br = valueMap[bumpSurf->w*(y + 1) + x + 1];
			vector3D vec = { (mid - br) / sqrt(2), (mid - br) / sqrt(2), depth*sqrt(2) }; //check math later
			dia_normals.push_back(vec);
		}
	}

	vector3D finalVec = { 0,0,0 };
	for (auto& i : normals)
	{
		i.normalize();
		finalVec = finalVec + i;
	}
	for (auto& i : dia_normals)
	{
		i.normalize();
		i = i / sqrt(2); //weighting
		finalVec = finalVec + i;
	}

	finalVec.normalize();
	return finalVec;
}


int main(int argc, char *argv[])
{
	init();

	double depth = 0.5;

	for (int i = 1; i < argc; i++)
	{
		std::string fileName(argv[i]);
		if (!(bumpSurf = IMG_Load(argv[i])))
		{
			std::cout << "Failed to load image " << argv << std::endl;
			continue;
		}		

		bumpSurf = SDL_ConvertSurfaceFormat(bumpSurf, SDL_PIXELFORMAT_RGBA8888, 0);	//convert to better format

		//get pixel values into valueMap
		extractPixels(bumpSurf);

		normalSurf = SDL_CreateRGBSurface
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
				vector3D normal = getAverageNormal(x, y, depth);
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
		std::cout << fileName << " processed" << std::endl;
	}

	close();
}