#pragma once
#include "MainLoop.h"

class Main {
public:
    Main();
    ~Main();

    static int getWidth();
    static int getHeight();

private:
    static void tick();
    static void draw(SDL_Renderer* rend);

    static SDL_Window* win;
    static SDL_Renderer* rend;
    static MainLoop ml;
};