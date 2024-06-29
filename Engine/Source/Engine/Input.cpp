#include "Input.h"

void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		currentlyPressedKeys.insert(key);
	}
	else if (action == GLFW_RELEASE)
	{
		currentlyPressedKeys.erase(key);
		justReleasedKeys.insert(key);
	}
}

bool Input::isKeyPressed(int key)
{
	return currentlyPressedKeys.find(key) != currentlyPressedKeys.end();
}

bool Input::areKeysPressed(const std::initializer_list<int>& keys)
{
	for (int key : keys)
	{
		if (currentlyPressedKeys.find(key) == currentlyPressedKeys.end())
		{
			return false;
		}
	}
	return true;
}

bool Input::isOneOfKeysPressed(const std::initializer_list<int>& keys)
{
	for (int key : keys)
	{
		if (currentlyPressedKeys.find(key) != currentlyPressedKeys.end())
		{
			return true;
		}
	}
	return false;
}

bool Input::isKeyReleased(int key)
{
	return justReleasedKeys.find(key) != justReleasedKeys.end();
}

void Input::updatePressedKeys()
{
	justReleasedKeys.clear();
}