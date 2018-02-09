#pragma once

//Input wrapper around GLFW, so the input isn't being driven by the vulkanapp class
#include <glm/vec2.hpp>

#include <array>


namespace Input {

	//Keycodes reflect ascii encodings, for simplicity's sake
	enum class KeyCode {
	SPACE             = 32,
	APOSTROPHE        = 39,  /* ' */
	COMMA             = 44,  /* , */
	MINUS             = 45,  /* - */
	PERIOD            = 46,  /* . */
	SLASH             = 47,  /* / */
	DIGIT_0                 = 48,
	DIGIT_1                 = 49,
	DIGIT_2                 = 50,
	DIGIT_3                 = 51,
	DIGIT_4                 = 52,
	DIGIT_5                 = 53,
	DIGIT_6                 = 54,
	DIGIT_7                 = 55,
	DIGIT_8                 = 56,
	DIGIT_9                 = 57,
	SEMICOLON         = 59,  /* ; */
	EQUAL             = 61,  /* = */
	A                 = 65,
	B                 = 66,
	C                 = 67,
	D                 = 68,
	E                 = 69,
	F                 = 70,
	G                 = 71,
	H                 = 72,
	I                 = 73,
	J                 = 74,
	K                 = 75,
	L                 = 76,
	M                 = 77,
	N                 = 78,
	O                 = 79,
	P                 = 80,
	Q                 = 81,
	R                 = 82,
	S                 = 83,
	T                 = 84,
	U                 = 85,
	V                 = 86,
	W                 = 87,
	X                 = 88,
	Y                 = 89,
	Z                 = 90,
	LEFT_BRACKET      = 91,  /* [ */
	BACKSLASH         = 92,  /* \ */
	RIGHT_BRACKET     = 93,  /* ] */
	GRAVE_ACCENT      = 96,  /* ` */
	WORLD_1           = 161, /* non-US #1 */
	WORLD_2           = 162, /* non-US #2 */

	/* Function keys */
	ESCAPE            = 256,
	ENTER             = 257,
	TAB               = 258,
	BACKSPACE         = 259,
	INSERT            = 260,
	DEL		          = 261,
	RIGHT             = 262,
	LEFT              = 263,
	DOWN              = 264,
	UP                = 265,
	PAGE_UP           = 266,
	PAGE_DOWN         = 267,
	HOME              = 268,
	END               = 269,
	CAPS_LOCK         = 280,
	SCROLL_LOCK       = 281,
	NUM_LOCK          = 282,
	PRINT_SCREEN      = 283,
	PAUSE             = 284,
	F1                = 290,
	F2                = 291,
	F3                = 292,
	F4                = 293,
	F5                = 294,
	F6                = 295,
	F7                = 296,
	F8                = 297,
	F9                = 298,
	F10               = 299,
	F11               = 300,
	F12               = 301,
	F13               = 302,
	F14               = 303,
	F15               = 304,
	F16               = 305,
	F17               = 306,
	F18               = 307,
	F19               = 308,
	F20               = 309,
	F21               = 310,
	F22               = 311,
	F23               = 312,
	F24               = 313,
	F25               = 314,
	KP_0              = 320,
	KP_1              = 321,
	KP_2              = 322,
	KP_3              = 323,
	KP_4              = 324,
	KP_5              = 325,
	KP_6              = 326,
	KP_7              = 327,
	KP_8              = 328,
	KP_9              = 329,
	KP_DECIMAL        = 330,
	KP_DIVIDE         = 331,
	KP_MULTIPLY       = 332,
	KP_SUBTRACT       = 333,
	KP_ADD            = 334,
	KP_ENTER          = 335,
	KP_EQUAL          = 336,
	LEFT_SHIFT        = 340,
	LEFT_CONTROL      = 341,
	LEFT_ALT          = 342,
	LEFT_SUPER        = 343,
	RIGHT_SHIFT       = 344,
	RIGHT_CONTROL     = 345,
	RIGHT_ALT         = 346,
	RIGHT_SUPER       = 347,
	MENU              = 348,

	};

	bool GetKey(KeyCode code);
	bool GetKeyDown(KeyCode code);
	bool GetKeyUp(KeyCode code);

	bool GetMouseButton(int button);
	bool GetMouseButtonPressed(int button);
	bool GetMouseButtonReleased(int button);
	glm::vec2 GetMousePosition();
	glm::vec2 GetMouseChangeInPosition();
	float GetMouseScrollX();
	float GetMouseScrollY();

	void SetTextInputMode();
	void ResetTextInputMode();
	bool GetTextInputMode();

	class InputDirector {
	public:
		InputDirector();

		bool GetKey(KeyCode code);
		bool GetKeyDown(KeyCode code);
		bool GetKeyUp(KeyCode code);

		bool GetMouseButton(int button);
		bool GetMouseButtonPressed(int button);
		bool GetMouseButtonReleased(int button);
		glm::vec2 GetMousePosition();
		glm::vec2 GetMouseChangeInPosition();
		float GetMouseScrollX();
		float GetMouseScrollY();

		void SetTextInputMode();
		void ResetTextInputMode();
		bool GetTextInputMode();

		void keyEvent(int key, int scancode, int action, int mods);
		void mouseButtonEvent(int button, int action, int mods);
		void mouseMoveEvent(double xoffset, double yoffset);
		void mouseScrollEvent(double xoffset, double yoffset);

		void ResetReleasedInput();
		void UpdateInputs();

	private:
		std::array<bool, 1024> keys = { { false } };
		std::array<bool, 1024> keysDown = { { false } };
		std::array<bool, 1024> keysUp = { { false } };

		std::array<bool, 15> mouseButtons = { { false } };
		std::array<bool, 15> mouseButtonsDown = { { false } };
		std::array<bool, 15> mouseButtonsUp = { { false } };

		bool firstMouse = true; //on start
		bool textInputMode = false;
		glm::vec2 mousePosition;
		glm::vec2 mousePositionPrevious;
		glm::vec2 mouseChangeInPosition;
		glm::vec2 mouseScroll;

	};

	extern InputDirector inputDirector;

}
