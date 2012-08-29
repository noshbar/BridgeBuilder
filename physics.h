/*
ZLib license:
Copyright (c) 2012 Dirk de la Hunt aka NoshBar @gmail.com

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __PHYSICS_H_
#define __PHYSICS_H_

#include <Box2D/Box2D.h>
#include "positioning.h"

/* These are settings that need tweaking to get a good "feeling" bridge.
   The first two work together in deciding how bouncy and stiff the bridge is, which decides what the breaking force
   should be.   I'm a dunce when it comes to maths, so I don't know if there is a "proper" way to work this stuff out,
   I pretty much mess around with values until something good comes out. */
#define JOINT_FREQ      15.0f
#define JOINT_DAMP      0.5f

enum
{
	Simulation_VelocityIterations = 8,
	Simulation_PositionIterations = 3,
};

/* TODO: This should be a virtual base class exposing the Create, Destroy, Step and Add* functions so that
   different physics engines can be pluggable.

   For now, this receives co-ordinates for new pins and slabs to be added, returning pointers to the newly
   created physical bodies that result from the operation, as well as progressing the simulation through
   the Step() function.

   Every time the main Bridge class swaps from edit to/from simulation mode, Destroy() and Create() of this
   class are called. */
class Physics
{
protected:
	/* These are the fixture.filter.categoryBits for the different types of bodies in the physical bridge scene.
	   The car collides with slabs, likewise slabs collide with a car, but pins are basically a non-collidable entity. */
	enum
	{
		Category_Car  = 1 << 1,
		Category_Slab = 1 << 2,
		Category_Pin  = 1 << 3,
	};

protected:
	b2World *world;

public:
	Physics()
	{
		world = NULL;
	}

	~Physics()
	{
		Destroy();
	}

	bool Create()
	{
		Destroy();
		/* Do some basic Box2D world setup stuff, setting gravity etc. */
		b2Vec2 gravity;
		gravity.Set(0.0f, -10.0f);
		world = new b2World(gravity);
		world->SetWarmStarting(true);
		world->SetContinuousPhysics(true);
		world->SetSubStepping(false);

		return true;
	}

	bool Destroy()
	{
		delete world;
		world = NULL;

		return false;
	}

	void Step(float TimeStep)
	{
		if (world == NULL)
			return;

		world->Step(TimeStep, Simulation_VelocityIterations, Simulation_PositionIterations);
	}

	void RemoveJoint(void *Joint)
	{
		world->DestroyJoint((b2Joint*)Joint);
	}

	/* This takes a body that was returned from an AddPin or AddSlab call and sets up a
	   transform instance with the current position and angle of the body. */
	void GetTransform(void *Body, Positioning &Result)
	{
		/* If the simulation isn't running, return having not modified a thing. */
		if (Body == NULL || world == NULL)
			return;

		b2Body *body = (b2Body*)Body;
		const b2Transform &transform = body->GetTransform();
		Result.Set(transform.p.x, transform.p.y, transform.q.GetAngle());
	}

	/* This takes a joint that was returned from an AddSupport call and calculates the force
	   it is currently experiencing, destroying the joint if it exceeds the specified maximum.
	   The calculated force is stored in Result so that the colour of the joint can be calculated. */
	void HandleSupportForce(void *&Joint, float Delta, float &Result, float Maximum)
	{
		/* If the simulation isn't running, return having not modified a thing. */
		if (Joint == NULL || world == NULL)
			return;

		b2Joint *joint = (b2Joint*)Joint;

		Result = joint->GetReactionForce(Delta).Length();
		if (Result >= Maximum)
		{
			Result = Maximum;
			world->DestroyJoint(joint);
			Joint = NULL;
		}
	}

	/* This takes a joint that was returned from an AddPin call and calculates the force
	   it is currently experiencing, destroying the joint if it exceeds the specified maximum.
	   (we separate this and the HandleSupportForce calls so that we can treat Pins and Supports differently). */
	void HandlePinForce(void *Body, float Delta, float Maximum)
	{
		if (Body == NULL || world == NULL)
			return;

		Maximum *= Maximum; /* Square the maximum to save us having to compare sqrt's against it. */

		b2Body      *body  = (b2Body*)Body;
		b2JointEdge *joint = body->GetJointList();
		while (joint)
		{
			b2JointEdge *next = joint->next;
			if (joint->joint->GetReactionForce(Delta).LengthSquared() > Maximum)
				world->DestroyJoint(joint->joint);

			joint = next;
		}
	}

	/* This creates a circular body a the specified location and returns it. */
	void* AddPin(float X, float Y, bool Fixed)
	{
		b2Body *result = NULL;

		b2CircleShape shape;
		shape.m_radius              = 0.5f;

		b2FixtureDef  fixture;
		fixture.shape               = &shape;
		fixture.density             = 20.0f;
		fixture.friction            = 0.2f;
		fixture.filter.categoryBits = Category_Pin;
		fixture.filter.maskBits     = 0; /* Pins don't collide with anything. */

		b2BodyDef body;
		if (!Fixed)
			body.type = b2_dynamicBody;
		body.position.Set(X, Y);
		result = world->CreateBody(&body);
		result->CreateFixture(&fixture);

		return result;
	}

	/* This takes two Pin->PhysicsBody instances, converts them to native objects (in this case Box2D b2Body objects),
	   then constructs a rectangle between them and hooks them up with rotation joints. */
	void* AddStructure(void *Left, void *Right)
	{
		b2Body *result = NULL;
		b2Body *left   = (b2Body*)Left;
		b2Body *right  = (b2Body*)Right;

		/* First get the X,Y positions of the two bodies... */
		b2Vec2  leftPosition  = left->GetPosition();
		b2Vec2  rightPosition = right->GetPosition();
		/* ... then calculate the distance between them, this will be used for the length of the box ... */
		b2Vec2  length        = leftPosition - rightPosition;
		float   slabLength    = length.Length() / 2.0f; /* Box2D specifies a radius of sorts. */
		/* ... calculate the center position of the rectangle, so we can use it as our object origin ... */
		float   centerX       = leftPosition.x + ((rightPosition.x - leftPosition.x) / 2.0f);
		float   centerY       = leftPosition.y + ((rightPosition.y - leftPosition.y) / 2.0f);
		/* ... then work out the angle between the two Pins so we know the orientation of the rectangle. */
		float   angle         = atan2(rightPosition.y - leftPosition.y, rightPosition.x - leftPosition.x);

		/* Create a Box2D shape. */
		b2PolygonShape shape;
		shape.SetAsBox(slabLength, 0.125f);
		/* Create a Box2D fixture set to the above shape, and set the collision index so that it is different to the car. */
		b2FixtureDef fixture;
		fixture.shape               = &shape;
		fixture.density             = 20.0f;
		fixture.friction            = 0.2f;
		fixture.filter.categoryBits = Category_Slab;
		fixture.filter.maskBits     = Category_Car;  /* Slabs do collide with the car. */
		/* Create the Box2D body at the position and angle as calculated above. */
		b2BodyDef body;
		body.position.Set(centerX, centerY);
		body.type  = b2_dynamicBody;
		body.angle = angle;
		result     = world->CreateBody(&body);
		result->CreateFixture(&fixture);

		/* Connect the slab to the two Pins it is attached to with a revolution joint. */
		b2RevoluteJointDef joint;
		joint.Initialize(left, result, leftPosition);
		world->CreateJoint(&joint);
		joint.Initialize(result, right, rightPosition);
		world->CreateJoint(&joint);

		return result;
	}

	/* This takes two Pin->PhysicsBody instances, converts them to native objects (in this case Box2D b2Body objects),
	   then constructs a distance joint between the two bodies. */
	void* AddSupport(void *Left, void *Right)
	{
		b2Body *left  = (b2Body*)Left;
		b2Body *right = (b2Body*)Right;

		/* Connect the support to the two pins it is attached to with a revolution joint */
		b2DistanceJointDef joint;
		joint.Initialize(left, right, left->GetPosition(), right->GetPosition());
		joint.frequencyHz  = JOINT_FREQ;
		joint.dampingRatio = JOINT_DAMP;
		return world->CreateJoint(&joint);
	}

	/* This simply adds a debug box to our scene. */
	void* AddBox(float X, float Y, float Mass)
	{
		if (world == NULL)
			return NULL;

		b2PolygonShape shape;
		shape.SetAsBox(1.0f, 1.0f);
		/* Create a Box2D fixture set to the above shape, and set the collision index so that it is different to the car. */
		b2FixtureDef fixture;
		fixture.shape               = &shape;
		fixture.density             = Mass;
		fixture.friction            = 0.2f;
		fixture.filter.categoryBits = Category_Car;
		fixture.filter.maskBits     = Category_Slab | Category_Car;  /* Slabs do collide with the car. */
		/* Create the Box2D body at the position and angle as calculated above. */
		b2BodyDef  body;
		b2Body    *result;

		body.position.Set(X, Y);
		body.type  = b2_dynamicBody;
		result     = world->CreateBody(&body);
		result->CreateFixture(&fixture);

		return result;
	}

};

#endif
