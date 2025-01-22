#include "Main.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <nch/cpp-utils/color.h>
#include <nch/sdl-utils/main-loop-driver.h>

SDL_Window* Main::win = nullptr;
SDL_Renderer* Main::rend = nullptr;
MainLoop Main::ml;

int Main::getWidth() { int res; SDL_GetWindowSize(win, &res, NULL); return res; }
int Main::getHeight() { int res; SDL_GetWindowSize(win, NULL, &res); return res; }


void Main::tick() { ml.tick(); }
void Main::draw(SDL_Renderer* rend)
{
    SDL_SetRenderTarget(rend, NULL);
    SDL_RenderClear(rend);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    SDL_RenderFillRect(rend, NULL);

    ml.draw(rend);

    SDL_RenderPresent(rend);
}

Main::Main()
{
    SDL_Init(SDL_INIT_VIDEO);
    win = SDL_CreateWindow("Regions", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    SDL_SetWindowResizable(win, SDL_TRUE);
    rend = SDL_CreateRenderer(win, -1, 0);

    SDL_Surface* icon = IMG_Load("icon.png");
    SDL_SetWindowIcon(win, icon);
    
    nch::MainLoopDriver mld(rend, &tick, 40, &draw, 200);
    SDL_FreeSurface(icon);
}
Main::~Main(){}

int main(int argc, char *argv[]) { Main m; return 0; }
#if ( defined(_WIN32) || defined(WIN32) )
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    char** x = new char*[1];
    return main(0, x);
}
#endif