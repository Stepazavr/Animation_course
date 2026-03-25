#pragma once
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_events.h>
#include "engine/event.h"

namespace engine
{
  // TIME SUBSYSTEM //

  // return time in seconds since the start of the program
  float get_time();

  // return delta time in seconds since the last frame
  float get_delta_time();

  // WINDOW SUBSYSTEM //

  // return aspect ratio of the window
  float get_aspect_ratio();

  // return screen size
  std::pair<int, int> get_screen_size();

  // Event for window resize
  extern Event<std::pair<int, int>> onWindowResizedEvent;

  // LOGGING SUBSYSTEM //

  // thread safe logging with saving history into ringBuffer

  // max number of log messages to keep in history
  const int MAX_LOG_HISTORY = 128;

  // log red message into console
  void error(const char *format, ...);

  // log white message into console
  void log(const char *format, ...);

  // INPUT SUBSYSTEM //

  // Event for keyboard input
  extern Event<SDL_KeyboardEvent> onKeyboardEvent;
  // Event for mouse button input
  extern Event<SDL_MouseButtonEvent> onMouseButtonEvent;
  // Event for mouse motion input
  extern Event<SDL_MouseMotionEvent> onMouseMotionEvent;
  // Event for mouse wheel input
  extern Event<SDL_MouseWheelEvent> onMouseWheelEvent;
  // Return 1 if key is pressed, 0 otherwise
  float get_key(SDL_Keycode keycode);
} // namespace engine