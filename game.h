/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __GAME_H_
#define __GAME_H_

#include <SDL/SDL.h>
#include "renderer.h"
#include "bridge.h"

enum
{
	ScreenWidth  = 800,
	ScreenHeight = 600,
	FrameRate    = 60,
};

/* This class is responsible for the running of the game.
   For now it's rather simple and not very well abstracted as it contains both calls to SDL creation and timing, but also game logic code.
   It would probably be better for this to contain only game logic, calling abstracted class functions for input handling, timing, etc. */
class Game
{
protected:
	typedef enum Mode
	{
		Mode_Building = 0,
		Mode_Testing,
	}
	Mode;

protected:
	Bridge   bridge;
	Renderer renderer;
	Mode     mode;

public:
	~Game()
	{
		Destroy();
	}

	bool Create()
	{
		if (!renderer.Create(ScreenWidth, ScreenHeight, FrameRate))
			return Destroy("Could not create renderer.");

		if (!bridge.Create())
			return Destroy("Could not create bridge instance.");

		/* The next two lines are for debugging, normally you'd load a bridge level here, and set to a building mode. */
		bridge.CreateTestBridge();
		bridge.SetEditMode(Bridge_EditMode_Car);

		mode = Mode_Building;
		return true;
	}

	bool Destroy(const char *Message = NULL)
	{
		if (Message != NULL)
			printf("Error starting game: %s\n", Message);
		bridge.Destroy();
		renderer.Destroy();
		SDL_Quit();
		return false;
	}

	bool Step()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
				{
					return false;
				}
				case SDL_MOUSEBUTTONDOWN:
				{
					float x = event.button.x;
					float y = event.button.y;
					renderer.ToWorld(x, y);
					bridge.HandleTouch(x, y);
					break;
				}
				case SDL_KEYDOWN:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_1:     bridge.SetEditMode(Bridge_EditMode_Structure); break;
						case SDLK_2:     bridge.SetEditMode(Bridge_EditMode_Support); break;
						case SDLK_3:     bridge.SetEditMode(Bridge_EditMode_Car); break;
						case SDLK_t:     bridge.CreateTestBridge(); break;
						case SDLK_r:     bridge.Create(); break;
						case SDLK_SPACE:
						{
							if (mode == Mode_Testing)
							{
								mode = Mode_Building;
								bridge.Stop();
							}
							else
							{
								mode = Mode_Testing;
								bridge.Start();
							}
							break;
						}
					}
					break;
				}
			}
		}

		renderer.FrameStart();
		bridge.Step(&renderer);
		renderer.FrameEnd();

		SDL_Delay(1000 / FrameRate); /* This is disgusting and WRONG, internet read "How to fix your timestep" */
		return true;
	}
};

#endif
