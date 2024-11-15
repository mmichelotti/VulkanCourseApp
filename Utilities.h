#pragma once
//Indicies (locations) of Queue Families (if they exist in the first pplace)

struct QueueFamilyIndices
{
	int graphicsFamily = -1; //location of graphicsQueue family
	bool isValid()
	{
		return graphicsFamily >= 0;
	}
};