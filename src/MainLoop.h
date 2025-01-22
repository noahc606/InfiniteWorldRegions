#pragma once
#include <map>
#include <tuple>
#include <nch/cpp-utils/color.h>
#include <nch/math-utils/vec3.h>
#include <SDL2/SDL.h>

class MainLoop {
public:
    MainLoop();
    ~MainLoop();

    void updateNearbyRegStates(nch::Vec3<int64_t> rPos, bool starting);
    void updateNearbyRegStates(nch::Vec3<int64_t> rPos);
    void tick();
    void mapEvents();
    void ctrls();
    void test();
    void draw(SDL_Renderer* rend);

private:
    void setMutatedColorGivenRXYZ(SDL_Renderer* rend, nch::Vec3<int64_t> rPos, nch::Color c);
    int scanRegsToLoad(int64_t loadDist);

    int64_t getCamRX(); int64_t getCamRY();
    int getRegState(nch::Vec3<int64_t> rPos);
    void loadReg(nch::Vec3<int64_t> rPos);
    void unloadReg(nch::Vec3<int64_t> rPos);



    std::map<std::tuple<int64_t, int64_t, int64_t>, int> regMap;
    std::map<int64_t, std::vector<nch::Vec3<int64_t>>> regsToLoad;  //map<Load Priority, list of regions>


    int64_t lastScanMS = -1234567;
    int64_t stateUpdatesThisSec = 0;

    int64_t loadDist = 6+4; //4
    int loadSpeed = 100;

    int64_t camX = 0, camY = 0;
    int64_t lastCamRX = -1234567, lastCamRY = -1234567;
    int64_t counter = 0;

    std::tuple<int64_t, int64_t, int> mouseLastUsed = std::make_tuple(-9999, -9999, -1);
};