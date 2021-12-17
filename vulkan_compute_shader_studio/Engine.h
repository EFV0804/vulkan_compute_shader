#pragma once
#include "VkRenderer.h"
#include "Simulation.h"

class Engine
{
public:
	Engine();
	~Engine();

	void init(Simulation simulation);
	void run();
	void close();

private:
	VkRenderer* renderer;
	Simulation simulation;
};

