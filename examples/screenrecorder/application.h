/*
 * This file is part of the Visual Computing Library (VCL) release under the
 * MIT license.
 *
 * Copyright (c) 2018 Basil Fierz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

// C++ standard library
#include <functional>
#include <iostream>

// Abseil
#include <absl/strings/string_view.h>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// ImGui
#include <imgui.h>
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

class Application
{
public:
	Application(absl::string_view application_name, unsigned int width, unsigned int height)
	: _width{width}
	, _height{height}
	{
		glfwSetErrorCallback(glfwErrorCallback);
		if (!glfwInit())
			throw std::runtime_error("Could not initialize GLFW");

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

		_window = glfwCreateWindow(width, height, application_name.data(), nullptr, nullptr);
		if (_window == nullptr)
			throw std::runtime_error("Could not initialize GLFW window");
		
		glfwSetWindowUserPointer(_window, this);
		glfwSetMouseButtonCallback(_window, onMouseButton);
		glfwSetCursorPosCallback(_window, onMouseMove);

		glfwMakeContextCurrent(_window);
		glfwSwapInterval(1); // Enable V-Sync

		// Setup OpenGL environment
		glewInit();

		// Setup Dear ImGui binding
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

		ImGui_ImplGlfw_InitForOpenGL(_window, false);
		ImGui_ImplOpenGL3_Init("#version 330");

		// Manually install the IO callbacks, due to overwriteing some of them here
		glfwSetScrollCallback(_window, ImGui_ImplGlfw_ScrollCallback);
		glfwSetKeyCallback(_window, ImGui_ImplGlfw_KeyCallback);
		glfwSetCharCallback(_window, ImGui_ImplGlfw_CharCallback);

		// Setup style
		//ImGui::StyleColorsDark();
		ImGui::StyleColorsLight();
	}
	~Application()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		if (_window)
			glfwDestroyWindow(_window);
		glfwTerminate();
	}
	Application& operator=(const Application&) = delete;
	Application& operator=(Application&&) = delete;

	GLFWwindow* window() { return _window; }

	template<typename Callback>
	void setSceneDrawCallback(Callback&& callback) { _draw_scene_callback = callback; }

	template<typename Callback>
	void setUIDrawCallback(Callback&& callback) { _draw_ui_callback = callback; }

	template<typename Callback>
	void setMouseButtonCallback(Callback&& callback) { _on_mouse_button = callback; }

	template<typename Callback>
	void setMouseMoveCallback(Callback&& callback) { _on_mouse_move = callback; }

	unsigned int width() const { return _width; }
	unsigned int height() const { return _height; }

	int run()
	{
		while (!glfwWindowShouldClose(window()))
		{
			glfwPollEvents();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (_draw_ui_callback)
				_draw_ui_callback(*this);
			ImGui::Render();

			glfwMakeContextCurrent(window());

			if (_draw_scene_callback)
				_draw_scene_callback(*this);

			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwMakeContextCurrent(window());
			glfwSwapBuffers(window());
		}

		return 0;
	}

private:
	static void glfwErrorCallback(int error, const char* description)
	{
		std::cerr << "Glfw Error " << error << ": " << description << std::endl;
	}

	static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

		auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
		if (app->_on_mouse_button)
			app->_on_mouse_button(*app, button, action, mods);
	}

	static void onMouseMove(GLFWwindow* window, double xpos, double ypos)
	{
		auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
		if (app->_on_mouse_move)
			app->_on_mouse_move(*app, xpos, ypos);
	}

	/// Main window
	GLFWwindow* _window{ nullptr };

	/// Scene drawing callback
	std::function<void(Application&)> _draw_scene_callback;

	/// UI drawing callback
	std::function<void(Application&)> _draw_ui_callback;

	/// Mouse button events
	std::function<void(Application&, int, int, int)> _on_mouse_button;

	/// Mouse move events
	std::function<void(Application&, double, double)> _on_mouse_move;

	/// Width of the application window
	unsigned int _width{ 0 };

	/// Height of the application window
	unsigned int _height{ 0 };
};
