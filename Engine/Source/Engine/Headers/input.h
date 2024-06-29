#pragma once

#include <unordered_set>
#include <GLFW/glfw3.h>

class Input
{

private:
	std::unordered_set<int> currentlyPressedKeys;
	std::unordered_set<int> justReleasedKeys;

public:
	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	bool isKeyPressed(int key);
	bool areKeysPressed(const std::initializer_list<int>& keys);
	bool isKeyReleased(int key);
	void updatePressedKeys();
	bool isOneOfKeysPressed(const std::initializer_list<int>& keys);
};