/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __BRIDGE_H_
#define __BRIDGE_H_

#include "renderer.h"
#include "physics.h"
#include "pin.h"
#include "slab_structure.h"
#include "slab_support.h"

/* The BREAK_AT_FORCE value is another "tweak until it feels right" value. It determines how much force can be
   exerted on a joint before it is considered broken.

   The MAX_BLOCKS value is debug information at the moment, simply stating how many boxes you can add to the level.
   This will be removed and replaced with the instances of cars in the level. */
#define BREAK_AT_FORCE  2.5f
#define MAX_BLOCKS      100 /* temporary */

typedef enum Bridge_EditMode
{
	Bridge_EditMode_Support = 0,
	Bridge_EditMode_Structure,
	Bridge_EditMode_Car,
}
Bridge_EditMode;

/* This class mostly holds editing/meta data for bridge creation.
   For now it is very simple, holding only a list of pins and slabs that make up the bridge.
   This class is/will be used for adding/removing/moving items around in the bridge, handling input, and finally
   converting the user data into physics engine data and running the simulation thereof. */
class Bridge
{
protected:
	/* A list of slabs that make up the bridge. */
	struct Slabs
	{
		Slab *First; /* The root element, contains a pointer to the ->Next instance. */
		Slab *Last;  /* NULL on start, set to the most recently added slab. Make it easier to add to the end of the list. */
	}
	slabs;

	/* A list of pins/joins that make up the bridge. */
	struct Pins
	{
		Pin *First;
		Pin *Last;
	}
	pins;

	Physics          physics; /* The abstracted physics engine, be it Box2D, Chipmunk, etc. */
	Pin             *startPin;
	Bridge_EditMode  editMode;
	bool             running;
	
	/* The following is temporary stuff to test the bridge with for now, will be replaced by cars. */
	void            *boxes[MAX_BLOCKS];
	int              boxCount;

protected:
	/* This returns a Pin instance at the specified co-ordinates with the specified accuracy, or NULL if none are found.
	   Accuracy caters for touches on screens, where the finger normally touches "more or less" around an area. */
	Pin* getPinAt(float X, float Y, float Accuracy)
	{
		Pin *result = pins.First;
		while (result)
		{
			if ( (fabs(result->Transform.X() - X) <= Accuracy) && (fabs(result->Transform.Y() - Y) <= Accuracy) )
				break;
			result = result->Next;
		}
		return result;
	}

	/* This first tries to find a Pin at the current location, returning it if one was found, otherwise it tries to add one. */
	Pin* addPin(float X, float Y)
	{
		Pin *pin = getPinAt(X, Y, 0.5f);
		if (pin != NULL)
			return pin;

		pin = new Pin(X, Y, false);
		if (pin == NULL)
			return pin;

		if (pins.First == NULL)
		{
			pins.First = pin;
			pins.Last  = pin;
		}
		else
		{
			pins.Last->Next = pin;
			pins.Last       = pin;
		}

		return pin;
	}

	/* This adds two Pins, one at X1,Y1 and another at X2,Y2.
	   These Pins serve to hold up the newly created Slab instance this function will create. */
	Slab* addSlab(float X1, float Y1, float X2, float Y2, Slab_Purpose Purpose)
	{
		Pin  *left  = addPin(X1, Y1);
		Pin  *right = addPin(X2, Y2);
		Slab *slab  = NULL;

		if (left == NULL || right == NULL)
			return slab;
		
		switch (Purpose)
		{
			case Slab_Purpose_Support: 
				slab = new SlabSupport(left, right);
				break;
			case Slab_Purpose_Structure:
				slab = new SlabStructure(left, right); 
				break;
			default:
				break;
		}

		if (slab == NULL)
			return slab;

		if (slabs.First == NULL)
		{
			slabs.First = slab;
			slabs.Last  = slab;
		}
		else
		{
			slabs.Last->Next = slab;
			slabs.Last       = slab;
		}

		return slab;
	}

	/* This function converts the pin and slab meta-data structures into physical objects within
	   the physics engine. */
	void createSimulation()
	{
		/* Clear any existing physics world (genocide, yay!) */
		physics.Create();

		/* First we create bodies for the pins, so that slabs have something to attach to. */
		Pin *pin = pins.First;
		while (pin != NULL)
		{
			/* Each pin gets assigned a pointer to a physics engine body for things to latch onto. */
			pin->Transform.Reset();
			pin->PhysicBody = physics.AddPin(pin->Transform.X(), pin->Transform.Y(), pin->Fixed);
			pin = pin->Next;
		}

		/* Now we create our different types of slabs, attaching each one to two existing pins. */
		Slab *slab = slabs.First;
		while (slab != NULL)
		{
			slab->Recalculate(); /* Recalculate the angle and length of the slab, as its pins may have moved. */
			switch (slab->Purpose)
			{
				case Slab_Purpose_Structure:
					slab->PhysicBody = physics.AddStructure(slab->Left->PhysicBody, slab->Right->PhysicBody);
					break;
				case Slab_Purpose_Support:
					slab->PhysicBody = physics.AddSupport(slab->Left->PhysicBody, slab->Right->PhysicBody);
					break;
				default:
					break;
			}
			slab = slab->Next;
		}

		running = true;
	}

public:
	Bridge()
	{
		slabs.First = NULL;
		pins.First  = NULL;
		startPin    = NULL;
	}

	~Bridge()
	{
		Destroy();
	}

	bool Create()
	{
		Destroy(); /* any existing bridge = bye bye. */

		/* After this call, the Game would probably call a Bridge.Load function, but for now, simply add 3 pins to attach
		   new slabs to for testing. */
		addPin(-20.0f,  0)->Fixed = true; /* The left and right most extreme pins are "attached" to our imaginary ground. */
		addPin(20.0f, 0)->Fixed = true;
		addPin(-10.0f, -10.0f)->Fixed = true;

		return true;
	}

	bool Destroy()
	{
		/* Nuke the slabs. */
		Slab *slab = slabs.First;
		while (slab != NULL)
		{
			Slab *next = slab->Next;
			delete slab;
			slab = next;
		}
		slabs.First = NULL;
		slabs.Last  = NULL;

		/* Nuke the pins */
		Pin *pin = pins.First;
		while (pin != NULL)
		{
			Pin *next = pin->Next;
			delete pin;
			pin = next;
		}
		pins.First   = NULL;
		pins.Last    = NULL;

		/* Reset our editing mode. */
		startPin     = NULL;
		editMode     = Bridge_EditMode_Structure;
		running      = false;

		return false;
	}

	/* This is called for every single "step" in the game.
	   It is responsible for advancing the physics engine and drawing the bits of our level */
	void Step(Renderer *Renderer)
	{
		float timeStep = Renderer->FrameRate() > 0 ? 1.0f / (float)Renderer->FrameRate() : 0.0f;
		physics.Step(timeStep);

		Slab *slab = slabs.First;
		while (slab)
		{
			switch (slab->Purpose)
			{
				case Slab_Purpose_Structure: /* Draw these as boxes around the X,Y co-ords of the physics entity. */
				{
					physics.GetTransform(slab->PhysicBody, slab->Transform);
					float         angle  = slab->Transform.Angle();
					float         x      = cos(angle) * (slab->Length / 2.0f);
					float         y      = sin(angle) * (slab->Length / 2.0f);
					Renderer->Box(slab->Transform.X(), slab->Transform.Y(), slab->Length, 0.5f, angle, 0x0000FF);
					break;
				}
				case Slab_Purpose_Support: /* Draw these simply as lines between the two pins. */
				{
					float force = 0.0f;
					/* Try get the force exerted on the joint, knowing that if it's too much, the joint will be deleted
					   and the slab->PhysicBody will be set to NULL, meaning we don't need to draw it. */
					physics.HandleSupportForce(slab->PhysicBody, timeStep, force, BREAK_AT_FORCE);
					force = force / (float)BREAK_AT_FORCE;

					if (running == false || slab->PhysicBody != NULL)
					{
						unsigned char red    = 255;                                      /* We want the joints to show red when they are stressed, so this stays constant. */
						unsigned char green  = (unsigned char)(255 - (force * 255.0f));  /* Green and blue must decrease with an increase in Force, so that the colour */
						unsigned char blue   = green;                                    /* gets a red tint to it. */
						unsigned long colour = (red << 16) + (green << 8) + (blue << 0);
						Renderer->Line(slab->Left->Transform.X(), slab->Left->Transform.Y(), slab->Right->Transform.X(), slab->Right->Transform.Y(), colour);
					}
					break;
				}
			}

			slab = slab->Next;
		}

		/* Optionally draw all our pins. */
		Pin *pin = pins.First;
		while (pin)
		{
			physics.HandlePinForce(pin->PhysicBody, timeStep, BREAK_AT_FORCE);
			physics.GetTransform(pin->PhysicBody, pin->Transform);
			Renderer->Circle(pin->Transform.X(), pin->Transform.Y(), 0.5f, 0x999999);
			pin = pin->Next;
		}

		/* The rest here is simply debug stuff, drawing our debug boxes, and displaying some instructions. */
		if (running)
		{
			for (int box = 0; box < boxCount; box++)
			{
				Positioning transform;
				physics.GetTransform(boxes[box], transform);
				Renderer->Box(transform.X(), transform.Y(), 2.0f, 2.0f, transform.Angle(), 0xFFFFFF);
			}
			Renderer->Text(10,10, "Simulation Mode", 0xFFFFFF);
		}
		else
		{
			Renderer->Text(10,10, "Editing Mode", 0xFFFFFF);
		}
		Renderer->Text(20,20, "(toggle with SPACE)", 0x888888);

		switch (editMode)
		{
			case Bridge_EditMode_Support:   Renderer->Text(10,40, "Adding support beams", 0xFFFFFF); break;
			case Bridge_EditMode_Structure: Renderer->Text(10,40, "Adding structure beams", 0xFFFFFF); break;
			case Bridge_EditMode_Car:       Renderer->Text(10,40, "Adding debug blocks (simulation mode only)", 0xFFFFFF); break;
		}
		Renderer->Text(20,50, "(press 1 for support, 2 for structure, 3 for blocks)", 0x888888);

		Renderer->Text(10,70, "(Also press R to reset the bridge, and T to generate a test bridge)", 0x888888);
	}

	/* Stop the simulation, but keep the bridge in tact. */
	void Stop()
	{
		physics.Destroy();
		Slab *slab = slabs.First;
		while (slab)
		{
			slab->Transform.Reset();
			slab = slab->Next;
		}
		Pin *pin = pins.First;
		while (pin)
		{
			pin->Transform.Reset();
			pin = pin->Next;
		}

		memset(boxes, 0, sizeof(void*) * MAX_BLOCKS);
		boxCount = 0;

		running = false;
	}

	/* Swap to simulation mode. */
	void Start()
	{
		Stop();
		createSimulation();
	}

	void SetEditMode(Bridge_EditMode EditMode)
	{
		editMode = EditMode;
	}

	/* This function is incredibly simple for now, accepting only co-ordinates of incoming touches/mouse clicks.
	   It will be expanded later to include an ID and time of the touch so that multi-touch can be dealt with. */
	void HandleTouch(float X, float Y)
	{
		switch (editMode)
		{
			case Bridge_EditMode_Car:
			{
				if (running && boxCount < MAX_BLOCKS)
				{
					boxes[boxCount] = physics.AddBox(X, Y, 10.0f);
					boxCount++;
				}
				break;
			}
			case Bridge_EditMode_Structure:
			case Bridge_EditMode_Support:
			{
				if (startPin == NULL)
				{
					startPin = getPinAt(X, Y, 0.5f);
				}
				else
				{
					Pin *pin = addPin(X, Y);
					if (pin != startPin)
					{
						addSlab(startPin->Transform.X(), startPin->Transform.Y(), pin->Transform.X(), pin->Transform.Y(), editMode == Bridge_EditMode_Structure ? Slab_Purpose_Structure : Slab_Purpose_Support);
						startPin = NULL;
					}
				}
				break;
			}
		}
	}

	/* This simply creates a test bridge (saving the user/developer from having to click out a bridge every time).
	   This is where our "load" function will come in future. */
	void CreateTestBridge()
	{
		int   slabCount     = 5;
		float slabWidth     = 8.0f;
		float supportHeight = 5.0f;
		float left          = -slabWidth * (slabCount / 2.0f); /* We do this to center our bridge around 0.0f */
		float right         =  slabWidth * (slabCount / 2.0f);

		Destroy();

		addPin(left,  0)->Fixed = true; /* The left and right most extreme pins are "attached" to our imaginary ground. */
		addPin(right, 0)->Fixed = true;

		/* We create several slabs in two parallel straight lines. The lower level is the "structure"/"road" level,
		   the upper level are the support structures, offset horizontally exactly half the width of a slab.
		   These two lines are linked by more support structures, so that we have a nice half-crosshatch pattern. */
		int index;
		for (index = 0; index < slabCount; index++)
		{
			right = left + slabWidth;

			float middle = left + (right - left) / 2.0f;

			addSlab(left,   0,             right,  0,             Slab_Purpose_Structure);
			addSlab(left,   0,             middle, supportHeight, Slab_Purpose_Support);
			addSlab(middle, supportHeight, right,  0,             Slab_Purpose_Support);
			if (index > 0)
				addSlab(middle, supportHeight, middle - slabWidth, supportHeight, Slab_Purpose_Support);

			left += slabWidth;
		}
	}

};

#endif
