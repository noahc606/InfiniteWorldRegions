#include "MainLoop.h"
#include <nch/cpp-utils/log.h>
#include <nch/sdl-utils/input.h>
#include <nch/sdl-utils/text.h>
#include <nch/sdl-utils/timer.h>
#include "Main.h"

using namespace nch;

MainLoop::MainLoop()
{
}
MainLoop::~MainLoop(){}

void MainLoop::updateNearbyRegStates(Vec3<int64_t> rPos, bool starting)
{
    if(starting) {
        stateUpdatesThisSec++;
    }


    int nearbyStates[5][5][5] = {0};
    for(int64_t ix = 0; ix<=4; ix++)
    for(int64_t iy = 0; iy<=4; iy++)
    for(int64_t iz = 0; iz<=4; iz++) {
        int state = getRegState(Vec3<int64_t>(rPos.x+ix-2, rPos.y+iy-2, rPos.z+iz-2));
        if(state>10) state = 10;
        nearbyStates[ix][iy][iz] = state;
    }

    //Update the states of any region in a 3x3x3 area centered at rPos.
    //Ultimately we are checking a 3x3x3 (27 total) of 3x3x3's, hence the 5x5x5 'nearbyStates'.
    std::vector<Vec3<int64_t>> toUpdateNext;
    for(int64_t nix = 1; nix<=3; nix++)
    for(int64_t niy = 1; niy<=3; niy++)
    for(int64_t niz = 1; niz<=3; niz++) {
        //Find the minimum value of any region state within this specific 3x3x3
        int minVal = 999999;
        for(int64_t ix = nix-1; ix<=nix+1; ix++)
        for(int64_t iy = niy-1; iy<=niy+1; iy++)
        for(int64_t iz = niz-1; iz<=niz+1; iz++) {
            int state = nearbyStates[ix][iy][iz];
            if(state<minVal) minVal = state;
        }

        //If we didn't find anything significant, continue
        if(minVal==-1) { continue; }
        if(minVal==10) { continue; }
        if(nearbyStates[nix][niy][niz]==minVal+1) { continue; }
        //If we did find a minVal >=0...
        else {
            //Set reg state directly
            auto currRPos = Vec3<int64_t>(rPos.x+(nix-2), rPos.y+(niy-2), rPos.z+(niz-2));
            auto rmItr = regMap.find(currRPos.tuple());
            if(rmItr==regMap.end()) {
                nch::Log::errorv(__PRETTY_FUNCTION__, "Nonexistent region despite minValFound>=0", "Failed to update reg state @ (%d, %d, %d)", rPos.x, rPos.y, rPos.z);
            } else {
                rmItr->second = (minVal+1);

                //Add this region (if it is not the central rPos region) to be updated next using recursion
                if(currRPos!=rPos) {
                    toUpdateNext.push_back(currRPos);
                }
            }
        }
    }

    //Recursively update as needed
    for(int i = 0; i<toUpdateNext.size(); i++) {
        updateNearbyRegStates(toUpdateNext[i], false);
    }
}

void MainLoop::updateNearbyRegStates(Vec3<int64_t> rPos)
{
    updateNearbyRegStates(rPos, true);
}

void MainLoop::tick()
{
    counter++;
    if(counter%40==0) {
        stateUpdatesThisSec = 0;
    }

    mapEvents();

    //Load all the regions that should be loaded depending on regsToLoad and loadCount
    int loadCount = 0;
    while(regsToLoad.size()!=0 && loadCount<=loadSpeed) {
        std::vector<Vec3<int64_t>>* nextLoadList = &regsToLoad.begin()->second;
        if(nextLoadList->size()==0) {
            regsToLoad.erase(regsToLoad.begin());
            continue;
        }


        for(int i = nextLoadList->size()-1; i>=0; i--) {
            loadReg(nextLoadList->at(i));
            nextLoadList->erase(nextLoadList->begin()+i);

            loadCount++;
            if(loadCount>loadSpeed) break;
        }

        int test = 3;
    }

    ctrls();
}

void MainLoop::mapEvents()
{
    int numUnloadedRegs = -1;

    //Camera cross a region border
    if(getCamRX()!=lastCamRX || getCamRY()!=lastCamRY) {
        int64_t dRX = lastCamRX-getCamRX();
        int64_t dRY = lastCamRY-getCamRY();
        
        lastCamRX = getCamRX();
        lastCamRY = getCamRY();
        if(SDL_GetTicks()-lastScanMS>1000) {
            numUnloadedRegs = scanRegsToLoad(loadDist);
        }
    }

    //If it has been 3 seconds since the last scan
    if(SDL_GetTicks()-lastScanMS>=3000) {
        numUnloadedRegs = scanRegsToLoad(loadDist);
    }

    //Regulate loadSpeed
    if(numUnloadedRegs!=-1) {
        loadSpeed = 10; //Setting: "Minimum" value
        if(numUnloadedRegs>1000) {
            loadSpeed = 100; //Setting: "Maximum" value
        }

        printf("%d\n", loadSpeed);
    }
}

void MainLoop::ctrls()
{
    int speed = 1;
    if(nch::Input::isKeyDown(SDLK_LCTRL)) speed = 2;

    if(nch::Input::isKeyDown(SDLK_w)) { camY -= speed; }
    if(nch::Input::isKeyDown(SDLK_a)) { camX -= speed; }
    if(nch::Input::isKeyDown(SDLK_s)) { camY += speed; }
    if(nch::Input::isKeyDown(SDLK_d)) { camX += speed; }


    int64_t loadDist = 3;
    int mrx = std::floor((nch::Input::getMouseX()+camX-Main::getWidth()/2)/64.);
    int mry = std::floor((nch::Input::getMouseY()+camY-Main::getHeight()/2)/64.);
    int mType = -1;
    if(nch::Input::isMouseDown(1)) mType = 1;
    if(nch::Input::isMouseDown(3)) mType = 3;

    
    if(mouseLastUsed!=std::make_tuple(mrx, mry, mType)) {
        if(mType==1) {
            for(int64_t irx = mrx-loadDist; irx<=mrx+loadDist; irx++)
            for(int64_t iry = mry-loadDist; iry<=mry+loadDist; iry++)
            for(int64_t irz = -10; irz<=10; irz++) {
                loadReg(Vec3<int64_t>(irx, iry, irz));
            }
        }
        if(mType==3) {
            for(int64_t irz = -10; irz<=10; irz++) { unloadReg(Vec3<int64_t>(mrx, mry, irz)); }
        }

        
        mouseLastUsed = std::make_tuple(mrx, mry, mType);
    }
}

void MainLoop::draw(SDL_Renderer* rend)
{
    //nch::Text::draw()

    int tx = -camX+Main::getWidth()/2;
    int ty = -camY+Main::getHeight()/2;
    SDL_Rect dst;
    

    


    //Objects

    //Region map
    {
        double sc = 64;

        for(auto reg : regMap) {
            Vec3<int64_t> rPos(reg.first);

            int64_t rx = rPos.x;
            int64_t ry = rPos.y;
            int64_t rz = rPos.z;
            if(rz!=0) continue;

            //if((rx+ry)%2) { SDL_SetRenderDrawColor(rend, 0, 255, 0, 255); }
            //else { SDL_SetRenderDrawColor(rend, 192, 255, 0, 255); }

            switch(reg.second) {
                case 0:
                case 1:
                case 2:
                case 3: { setMutatedColorGivenRXYZ(rend, rPos, nch::Color(128,   0, 192)); } break; //Purple
                case 4: { setMutatedColorGivenRXYZ(rend, rPos, nch::Color(  0,   0, 128)); } break; //Blue
                case 5: { setMutatedColorGivenRXYZ(rend, rPos, nch::Color(  0, 192, 160)); } break; //Turquoise
                case 6: { setMutatedColorGivenRXYZ(rend, rPos, nch::Color(  0, 128,   0)); } break; //Green
                case 7:
                case 8:
                case 9: { setMutatedColorGivenRXYZ(rend, rPos, nch::Color(192, 192,   0)); } break; //Yellow
                case 10:{ setMutatedColorGivenRXYZ(rend, rPos, nch::Color(192,   0,   0)); } break; //Red
               default: { setMutatedColorGivenRXYZ(rend, rPos, nch::Color(255, 255, 255)); } break; //White (unknown)
            }

            dst.x = sc*rx+tx; dst.y = sc*ry+ty; dst.w = sc; dst.h = sc;
            SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(rend, &dst);
        }
    }

    //Crosshair
    {
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);

        dst.x = Main::getWidth()/2-16;  dst.w = 32;
        dst.y = Main::getHeight()/2-1;  dst.h = 2;
        SDL_RenderFillRect(rend, &dst);
        dst.x = Main::getWidth()/2-1;   dst.w = 2;
        dst.y = Main::getHeight()/2-16; dst.h = 32;
        SDL_RenderFillRect(rend, &dst);
    }
}

void MainLoop::setMutatedColorGivenRXYZ(SDL_Renderer* rend, Vec3<int64_t> rPos, nch::Color c)
{
    int64_t rx = rPos.x;
    int64_t ry = rPos.y;
    int64_t rz = rPos.z;
    nch::Color c1 = c;
    nch::Color c2 = c1; c2.brighten(15);
    nch::Color c3 = c;  c3.transpare(128);
    nch::Color c4 = c3; c4.brighten(15);


    if( (int64_t)(std::floor(rx/4.)+std::floor(ry/4.)+std::floor(rz/4.))%2 ) {
        if((rx+ry+rz)%2)    SDL_SetRenderDrawColor(rend, c1.r, c1.g, c1.b, c1.a);
        else                SDL_SetRenderDrawColor(rend, c2.r, c2.g, c2.b, c2.a);
    } else {
        if((rx+ry+rz)%2)    SDL_SetRenderDrawColor(rend, c3.r, c3.g, c3.b, c3.a);
        else                SDL_SetRenderDrawColor(rend, c4.r, c4.g, c4.b, c4.a);
    }
}

int MainLoop::scanRegsToLoad(int64_t loadDist)
{
    int unloadedCount = 0;
    lastScanMS = SDL_GetTicks64();
    nch::Timer t("scan", true);

    int64_t crx = getCamRX();
    int64_t cry = getCamRY();
    int64_t crz = 0;

    regsToLoad.clear();
    for(int64_t ix = crx-loadDist; ix<=crx+loadDist; ix++)
    for(int64_t iy = cry-loadDist; iy<=cry+loadDist; iy++)
    for(int64_t iz = crz-loadDist; iz<=crz+loadDist; iz++) {
        Vec3<int64_t> iPos = Vec3<int64_t>(ix, iy, iz);
        int state = getRegState(iPos);
        if(state==-1) {
            unloadedCount++;
            int64_t taxiDist = std::abs(ix-crx)+std::abs(iy-cry)+std::abs(iz-crz);

            auto tlItr = regsToLoad.find(taxiDist);
            if(tlItr==regsToLoad.end()) {
                std::vector<Vec3<int64_t>> iPosVec;
                iPosVec.push_back(iPos);
                regsToLoad.insert(std::make_pair(taxiDist, iPosVec));
            } else {
                tlItr->second.push_back(iPos);
            }
        }
    }

    return unloadedCount;
}

int64_t MainLoop::getCamRX() { return std::floor(camX/64.); }
int64_t MainLoop::getCamRY() { return std::floor(camY/64.); }

int MainLoop::getRegState(Vec3<int64_t> rPos)
{
    auto rmItr = regMap.find(rPos.tuple());
    if(rmItr==regMap.end()) return -1;
    else                    return rmItr->second;
}

void MainLoop::loadReg(Vec3<int64_t> rPos)
{
    if(regMap.find(rPos.tuple())==regMap.end()) {
        regMap.insert(std::make_pair(rPos.tuple(), 0));
        updateNearbyRegStates(rPos);
    }
}

void MainLoop::unloadReg(Vec3<int64_t> rPos)
{
    auto rmItr = regMap.find(rPos.tuple());
    if(rmItr==regMap.end()) return;
    regMap.erase(rmItr);

    for(int ix = rPos.x-1; ix<=rPos.x+1; ix++)
    for(int iy = rPos.y-1; iy<=rPos.y+1; iy++)
    for(int iz = rPos.z-1; iz<=rPos.z+1; iz++) {
        Vec3<int64_t> irPos = Vec3<int64_t>(ix, iy, iz);
        
        auto rmItr = regMap.find(irPos.tuple());
        if(rmItr!=regMap.end()) { rmItr->second = 0; }
        updateNearbyRegStates(Vec3<int64_t>(ix, iy, iz));
    }
}