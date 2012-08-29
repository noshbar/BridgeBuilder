/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __RENDERER_H_
#define __RENDERER_H_

#include <math.h>
#include "SDL/SDL.h"
#include "font_small.h"

#define BPP         32
#define PIXELBUFFER Uint32
#define INTX(x)     (int)((x) * scale + halfWidth + offsetX)
#define INTY(y)     (int)(screenHeight - ((y) * scale + halfHeight + offsetY))

/* TODO: This should be an abstract base class for other renderers to derive from.
   For now, this is a simple renderer with throw-away code that uses SDL for drawing lines and circles.
   CAUTION: you shouldn't try to learn anything from this code except the usual "how not to do something". */
class Renderer
{
private:
    SDL_Surface *screen;
	float        scale;
	float        offsetX;
	float        offsetY;
	int          screenWidth;
	int          screenHeight;
	int          frameRate;
	int          halfWidth;
	int          halfHeight;

protected:
	PIXELBUFFER* lock()
	{
		if (SDL_MUSTLOCK(screen))
		{
			if (SDL_LockSurface(screen) < 0)
			{
				printf("Error locking surface: %s\n", SDL_GetError());
				abort();
			}
		}
		return (PIXELBUFFER*)screen->pixels;
	}

	void unlock()
	{
		SDL_UnlockSurface(screen);
	}

public:
	Renderer()
	{
		screen = NULL;
	}

	~Renderer()
	{
		Destroy();
	}

	bool Create(int Width, int Height, int FrameRate)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
			return Destroy("Could not initialise SDL.");

		if ((screen = SDL_SetVideoMode(Width, Height, BPP, 0)) == NULL)
			return Destroy("Error setting video mode");

		screenWidth  = Width;
		screenHeight = Height;
		frameRate    = FrameRate;
		halfWidth    = Width / 2;
		halfHeight   = Height / 2;
		scale        = 10.0f;
		offsetX      = 0.0f;
		offsetY      = 0.0f;
		return true;
	}

	bool Destroy(const char *Message = NULL)
	{
		if (Message != NULL)
			printf("Message");

		screen = NULL;
		return false;
	}

	int FrameRate()
	{
		return frameRate;
	}

	/* This "moves the contents of the game around" on the screen. */
	void SetTransform(float OffsetX, float OffsetY, float Scale)
	{
		offsetX = OffsetX;
		offsetY = OffsetY;
		scale   = Scale;
	}

	/* This converts a screen co-ordinate to an in-game co-ordinate. */
	void ToWorld(float &X, float &Y)
	{
		X -= halfWidth;
		X -= offsetX;
		X /= scale;

		Y  = screenHeight - Y;
		Y -= halfHeight;
		Y -= offsetY;
		Y /= scale;
	}

	/* This is called at the start of every single game frame.
	   Use the time to clear the screen, reset view transformations, whatever here. */
	void FrameStart()
	{
		SDL_FillRect(screen, NULL, 0);
	}

	/* This is called at the end of every single game frame, once everything has been drawn.
	   Use it to blit the double-buffer to the screen. */
	void FrameEnd()
	{
	    SDL_UpdateRect(screen, 0, 0, 0, 0);
	}

	/* Draws a box centered around X,Y with the dimensions and angle as supplied. */
	void Box(float X, float Y, float Width, float Height, float Angle, unsigned long Colour)
	{
		float cosine       = cos(Angle);
		float sine         = sin(Angle);
		float widthCosine  = (Width / 2.0f) * cosine;
		float heightCosine = (Height / 2.0f) * cosine;
		float widthSine    = (Width / 2.0f) * sine;
		float heightSine   = (Height / 2.0f) * sine;
		float ulX          = X + widthCosine  - heightSine;
		float ulY          = Y + heightCosine + widthSine;
		float urX          = X - widthCosine  - heightSine;
		float urY          = Y + heightCosine - widthSine;
		float blX          = X + widthCosine  + heightSine;
		float blY          = Y - heightCosine + widthSine;
		float brX          = X - widthCosine  + heightSine;
		float brY          = Y - heightCosine - widthSine;
		Line(ulX, ulY, urX, urY, Colour);
		Line(urX, urY, brX, brY, Colour);
		Line(brX, brY, blX, blY, Colour);
		Line(blX, blY, ulX, ulY, Colour);
	}

	/* Draws a line from X0,Y0 to X1,Y1 in the specified Colour using the Bresenham algorithm. */
	void Line(float X0, float Y0, float X1, float Y1, unsigned long Colour)
	{
		int x0 = INTX(X0);
		int y0 = INTY(Y0);
		int x1 = INTX(X1);
		int y1 = INTY(Y1);

		/* A rather pessimistic clipping routine that doesn't draw the line at all if any of the points are off-screen.
		   Would be better if it simply clipped the out of range values. */
		if (x0 < 0 || x0 >= screenWidth || x1 < 0 || x1 >= screenWidth || y0 < 0 || y0 >= screenHeight || y1 < 0 || y1 >= screenHeight)
			return;

		PIXELBUFFER *buffer = lock();
		int          xinc   = 1;
		int          yinc   = screenWidth;
		int          xspan  = x1 - x0 + 1;
		int          yspan  = y1 - y0 + 1;
	
		if (xspan < 0)
		{
			xinc  = -xinc;
			xspan = -xspan;
		}
		if (yspan < 0) 
		{
			yinc  = -yinc;
			yspan = -yspan;
		}
	
		int  sum         = 0;
		int  drawpos     = screenWidth * y0 + x0;

		bool yBigger     = (xspan < yspan);
		int  iMax        = yBigger ? yspan : xspan;
		int  sumInc      = yBigger ? xspan : yspan;
		int  posInc      = yBigger ? xinc  : yinc;
		int  compare     = yBigger ? yspan : xspan;
		int  finalPosInc = yBigger ? yinc  : xinc;
	
		for (int i = 0; i < iMax; i++)
		{
			buffer[drawpos] = Colour;
			sum += sumInc;
			if (sum >= compare)
			{
				drawpos += posInc;
				sum -= compare;
			}
			drawpos += finalPosInc;
		}

		unlock();
	}

	/* Draws a circle around X,Y with the specified Radius in the specified Colour using the Bresenham algorithm. */
	void Circle(float X, float Y, float Radius, unsigned long Colour)
	{
		double cx = INTX(X);
		double cy = INTY(Y);
		Radius   *= scale;

		/* A rather pessimistic clipping routine that doesn't draw the circle at all if any of the points are off-screen.
		   Would be better if it simply clipped the out of range values. */
		if (cx - Radius < 0 || cx + Radius >= screenWidth || cy - Radius < 0 || cy + Radius >= screenHeight)
			return;

		
		PIXELBUFFER *buffer = lock();
		double       error  = (double)-Radius;
		double       x      = (double)Radius - 0.5;
		double       y      = (double)0.5;
		cx                  = cx - 0.5f;
		cy                  = cy - 0.5f;

		while (x >= y)
		{
			buffer[(int)(cx + x) + ((int)(cy + y) * screenWidth)] = Colour;
			buffer[(int)(cx + y) + ((int)(cy + x) * screenWidth)] = Colour;
          
			if (x != 0)
			{
				buffer[(int)(cx - x) + ((int)(cy + y) * screenWidth)] = Colour;
				buffer[(int)(cx + y) + ((int)(cy - x) * screenWidth)] = Colour;
			}
      
			if (y != 0)
			{
				buffer[(int)(cx + x) + ((int)(cy - y) * screenWidth)] = Colour;
				buffer[(int)(cx - y) + ((int)(cy + x) * screenWidth)] = Colour;
			}
      
			if (x != 0 && y != 0)
			{
				buffer[(int)(cx - x) + ((int)(cy - y) * screenWidth)] = Colour;
				buffer[(int)(cx - y) + ((int)(cy - x) * screenWidth)] = Colour;
			}

			error += y;
			++y;
			error += y;

			if (error >= 0)
			{
				--x;
				error -= x;
				error -= x;
			}
		}

		unlock();
	}

	void Text(int X, int Y, const char *String, unsigned long Colour)
	{
		if (screen == NULL || String == NULL)
			return;

		PIXELBUFFER *buffer = lock();
		for (int index = 0; index < strlen(String); index++)
		{
			for (int x = 0; x < 5; x++)
			{
				for (int y = 0; y < 7; y++)
				{
					if ((SmallFont[String[index] - 32][x] & (1 << y)) != 0)
						buffer[(X + x) + ((Y + y) * screenWidth)] = Colour;
				}
			}
			X += 6;
		}

		unlock();
	}
};

#endif
