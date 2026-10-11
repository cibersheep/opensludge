/*
 * Playing with a game controller.
 *
 * SLUDGE games are played with the mouse, so we use a controller to move the
 * mouse pointer and to click:
 *
 *   left stick or d-pad     move the pointer
 *   A (bottom button)       left click
 *   B (right button)        right click
 *   left and right shoulder move the pointer to the previous or next thing
 *                           that the player can click on
 *   Start                   the Escape key
 */

#include <math.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "allfiles.h"
#include "debug.h"
#include "gamepad.h"
#include "graphics.h"
#include "objtypes.h"
#include "people.h"
#include "region.h"
#include "sludger.h"

extern inputType input;
extern float cameraZoom;
extern int cameraX, cameraY;
extern onScreenPerson * allPeople;
extern screenRegion * allScreenRegions;

// Stick positions closer to the centre than this count as the centre.
static const float STICK_DEAD_ZONE = 0.25f;
// At full speed, the pointer crosses the screen in this many seconds.
static const float SECONDS_TO_CROSS_SCREEN = 1.2f;
// The d-pad moves the pointer at this fraction of full speed.
static const float DPAD_SPEED = 0.6f;

// The pointer position in game coordinates, with the fraction that we lose
// in input.mouseX and input.mouseY.
static float pointerX, pointerY;
static Uint64 lastUpdate = 0;

static float screenWidth () {
	return (float) winWidth / cameraZoom;
}

static float screenHeight () {
	return (float) winHeight / cameraZoom;
}

// Put the pointer at a position in game coordinates.
static void movePointerTo (float x, float y) {
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > screenWidth () - 1) x = screenWidth () - 1;
	if (y > screenHeight () - 1) y = screenHeight () - 1;
	pointerX = x;
	pointerY = y;
	input.mouseX = (int) x;
	input.mouseY = (int) y;
	input.justMoved = true;

	// We also move the system pointer, because in some games the player sees
	// it instead of a pointer drawn by the game. On some platforms, such as
	// Wayland, moving the system pointer is not possible.
	float windowX, windowY;
	gameToWindowCoords ((float) input.mouseX, (float) input.mouseY, &windowX, &windowY);
	if (sludgeWindow) SDL_WarpMouseInWindow (sludgeWindow, windowX, windowY);
}

// A place that the player can click on, in game coordinates.
struct hotspot {
	float x, y;
};

static int compareHotspots (const void * a, const void * b) {
	const hotspot * h1 = (const hotspot *) a;
	const hotspot * h2 = (const hotspot *) b;
	if (h1->x != h2->x) return h1->x < h2->x ? -1 : 1;
	if (h1->y != h2->y) return h1->y < h2->y ? -1 : 1;
	return 0;
}

// Move the pointer to the next thing that the player can click on, from
// left to right, or to the previous one when forward is false. These are the
// screen regions and the characters on the screen.
static void moveToNextHotspot (bool forward) {
	int count = 0;
	for (screenRegion * r = allScreenRegions; r; r = r->next) count ++;
	for (onScreenPerson * p = allPeople; p; p = p->next) count ++;
	if (! count) return;

	hotspot * spots = new hotspot[count];
	int n = 0;
	for (screenRegion * r = allScreenRegions; r; r = r->next) {
		spots[n].x = (r->x1 + r->x2) / 2.0f - cameraX;
		spots[n].y = (r->y1 + r->y2) / 2.0f - cameraY;
		n ++;
	}
	for (onScreenPerson * p = allPeople; p; p = p->next) {
		if (! p->show || ! p->thisType) continue;
		// The middle of the character, whose position is at the feet.
		spots[n].x = p->x - cameraX;
		spots[n].y = p->y - cameraY - p->scale * (p->height - p->floaty) / 2;
		n ++;
	}

	// We only go to hotspots inside the screen.
	int onScreen = 0;
	for (int i = 0; i < n; i ++) {
		if (spots[i].x >= 0 && spots[i].y >= 0 && spots[i].x < screenWidth () && spots[i].y < screenHeight ()) {
			spots[onScreen ++] = spots[i];
		}
	}
	if (onScreen) {
		qsort (spots, onScreen, sizeof (hotspot), compareHotspots);
		hotspot current = { (float) input.mouseX, (float) input.mouseY };
		int target;
		if (forward) {
			target = 0;
			for (int i = 0; i < onScreen; i ++) {
				if (compareHotspots (&spots[i], &current) > 0) {
					target = i;
					break;
				}
			}
		} else {
			target = onScreen - 1;
			for (int i = onScreen - 1; i >= 0; i --) {
				if (compareHotspots (&spots[i], &current) < 0) {
					target = i;
					break;
				}
			}
		}
		movePointerTo (spots[target].x, spots[target].y);
	}
	delete [] spots;
}

void initGamepads () {
	if (! SDL_InitSubSystem (SDL_INIT_GAMEPAD)) {
		debugOut ("Failed to initialise game controllers: %s\n", SDL_GetError ());
	}
}

void handleGamepadEvent (const SDL_Event & event) {
	switch (event.type) {
		case SDL_EVENT_GAMEPAD_ADDED:
			if (SDL_OpenGamepad (event.gdevice.which)) {
				debugOut ("Game controller connected: %s\n", SDL_GetGamepadNameForID (event.gdevice.which));
			}
			break;

		case SDL_EVENT_GAMEPAD_REMOVED:
			SDL_CloseGamepad (SDL_GetGamepadFromID (event.gdevice.which));
			break;

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP: {
			bool down = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
			switch (event.gbutton.button) {
				case SDL_GAMEPAD_BUTTON_SOUTH:
					if (down) input.leftClick = true;
					else input.leftRelease = true;
					break;
				case SDL_GAMEPAD_BUTTON_EAST:
					if (down) input.rightClick = true;
					else input.rightRelease = true;
					break;
				case SDL_GAMEPAD_BUTTON_START:
					if (down) input.keyPressed = 27;
					break;
				case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
					if (down) moveToNextHotspot (false);
					break;
				case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
					if (down) moveToNextHotspot (true);
					break;
				default:
					break;
			}
			break;
		}

		default:
			break;
	}
}

// Return the position of a stick axis from -1 to 1, with the dead zone
// removed.
static float stickPosition (SDL_Gamepad * gamepad, SDL_GamepadAxis axis) {
	float value = SDL_GetGamepadAxis (gamepad, axis) / 32767.0f;
	if (fabsf (value) < STICK_DEAD_ZONE) return 0;
	float sign = value < 0 ? -1.0f : 1.0f;
	value = (fabsf (value) - STICK_DEAD_ZONE) / (1 - STICK_DEAD_ZONE);
	if (value > 1) value = 1;
	// Small movements of the stick move the pointer slowly, for precision.
	return sign * value * value;
}

void updateGamepadPointer () {
	Uint64 now = SDL_GetTicks ();
	float seconds = lastUpdate ? (now - lastUpdate) / 1000.0f : 0;
	lastUpdate = now;
	if (seconds > 0.1f) seconds = 0.1f;

	float dx = 0, dy = 0;
	int count;
	SDL_JoystickID * ids = SDL_GetGamepads (&count);
	if (! ids) return;
	for (int i = 0; i < count; i ++) {
		SDL_Gamepad * gamepad = SDL_GetGamepadFromID (ids[i]);
		if (! gamepad) continue;
		dx += stickPosition (gamepad, SDL_GAMEPAD_AXIS_LEFTX);
		dy += stickPosition (gamepad, SDL_GAMEPAD_AXIS_LEFTY);
		if (SDL_GetGamepadButton (gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) dx -= DPAD_SPEED;
		if (SDL_GetGamepadButton (gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) dx += DPAD_SPEED;
		if (SDL_GetGamepadButton (gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) dy -= DPAD_SPEED;
		if (SDL_GetGamepadButton (gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) dy += DPAD_SPEED;
	}
	SDL_free (ids);
	if (dx == 0 && dy == 0) return;

	// Start from the mouse position, because the player may have moved the
	// mouse since the last update.
	if ((int) pointerX != input.mouseX || (int) pointerY != input.mouseY) {
		pointerX = (float) input.mouseX;
		pointerY = (float) input.mouseY;
	}
	float speed = screenWidth () / SECONDS_TO_CROSS_SCREEN * seconds;
	movePointerTo (pointerX + dx * speed, pointerY + dy * speed);
}
