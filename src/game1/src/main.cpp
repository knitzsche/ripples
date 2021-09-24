/**
 *
 * Copyright (C) 2021 Kyle Nitzsche
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kyle Nitzsche <kyle.nitzsche@gmail.com>
 **/

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <math.h>
#include <cmath>
#include <ctime>
#include <numeric>
#include <cstdlib>

#include <SDL.h>
#include <SDL2_gfxPrimitives.h> //install libsdl2-gfx-dev
#include <wayland-client.h>
#include "SDL_syswm.h"

using namespace std;

void get_info(){
  SDL_WINDOW_OPENGL;
  SDL_version compiled;
  SDL_version linked;
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  printf("Compiled against SDL version %d.%d.%d.\n",
           compiled.major, compiled.minor, compiled.patch);
  printf("Linked against SDL version %d.%d.%d.\n",
           linked.major, linked.minor, linked.patch);
}

int SCREEN_WIDTH  = 1280;
int SCREEN_HEIGHT = 720;

void setScreen() {
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    SCREEN_WIDTH = DM.w;
    SCREEN_HEIGHT = DM.h;
}

void logSDLError(ostream &os, const string &msg) {
    os << msg << " error: " << SDL_GetError() << endl;
}

void addRect(shared_ptr<vector<shared_ptr<SDL_Rect>>> rs) {
    shared_ptr<SDL_Rect> r = make_shared<SDL_Rect>();
    r->x = 0;
    r->y = 0;
    r->w = 50;
    r->h = 50;
    rs->emplace_back(r);
}

int last_aim = 0; // 0 up, 1 right, 2 down, 3 left


struct Position {
    double x;
    double y;
};

struct RGB {
    int r;
    int g;
    int b;
    int a;
};

struct Circle {
    Position p;
    int r = 5; // radius
    RGB rgb;
    bool collided = false;
    int collision_render_count = 0; // track number of render cycles after collision
};

struct PositionRelative {
    shared_ptr<Circle> circle;
    double distance;
};

class  MovingCircle : public Circle {
public:
    Position prevP;
};

class  Ripple : public MovingCircle {
public:
    int expand_speed;
    vector<shared_ptr<PositionRelative>> grid_relative;
    int width = 50;
};

void addRipple(shared_ptr<vector<shared_ptr<Ripple>>> cs, int const& x = 20, int const& y = 80, int const& r = 100, int const& g = 200, int const& b = 100, int const& a = 200, int const& expand_speed = 1, int const& width = 50) {
    //cout << " in addCircle(). x: " << x << " y: " << y << endl;
    shared_ptr<Ripple> c = make_shared<Ripple>();
    c->p.x = x;
    c->p.y = y;
    c->rgb.r = r;
    c->rgb.g = g;
    c->rgb.b = b;
    c->rgb.a = a;
    c->expand_speed = expand_speed;
    c->width = width;
    cs->emplace_back(c);
}

double getDistance(Position const& p1, Position const& p2) {
    return sqrt(pow((p2.x - p1.x), 2) + pow((p2.y - p1.y), 2));
}

/*
 * calculate distances from ripple origin to all grid points
 */
void addGridToRipple(Ripple &r, vector<shared_ptr<Circle>> const& grid) {
    for (auto c : grid)
    {
        shared_ptr<PositionRelative> pos = make_shared<PositionRelative>();
        pos->circle = c;
        pos->distance = getDistance(c->p, r.p);
        //cout << " addGridToR. distance: " << distance << endl;
        r.grid_relative.emplace_back(pos);
    }
}

void addCircle(shared_ptr<vector<shared_ptr<Circle>>> cs, int x = 20, int y = 80, int r = 100, int g = 200, int b = 200, int a = 200) {
    //cout << " in addCircle(). x: " << x << " y: " << y << endl;
    shared_ptr<Circle> c = make_shared<Circle>();
    c->p.x = x;
    c->p.y = y;
    c->rgb.r = r;
    c->rgb.g = g;
    c->rgb.b = b;
    c->rgb.a = a;
    cs->emplace_back(c);
}

void addMovingCircle(shared_ptr<vector<shared_ptr<MovingCircle>>> cs, int x, int y) {
    shared_ptr<MovingCircle> c = make_shared<MovingCircle>();
    int prevX = rand() % 3 + 1;
    if (rand() % 2 == 0) {
        prevX=-prevX;
    }
    int prevY = rand() % 3 + 1;
    if (rand() % 2 == 0) {
        prevY=-prevY;
    }
    c->r = 20;
    c->p.x = x;
    c->prevP.x = x-prevX;
    c->p.y = y;
    c->prevP.y = y-prevY;
    c->rgb.r = 100;
    c->rgb.g = 200;
    c->rgb.b = 100;
    c->rgb.a = 200;
    cs->emplace_back(c);
}

struct Gun {
    int x;
    int y;
    int x2;
    int y2;
    int angle = 45; //degrees
    int length = 30;
};

double getRad(double degree) {
    return degree * 3.1415/180;
}

double getDeg(double radian) {
    return radian * 180/3.1415;
}
/*
 * arg 2:
 *        1 means clockwise (right arrow pressed)
 *        2 means counterclockwise (left arrow preseed)
 */
void rotateGun(shared_ptr<Gun> g, int rotation) {
    // radians =  degrees * pi / 180 ;
    // opposite = sin(angle) * gun lengtth (hypot)
    //adjacent = cos(angle) * hypt
    int anglechange;
    if (rotation == 1)
        anglechange = -5;
    else if (rotation == 2)
        anglechange = 5;

    int newA = g->angle + anglechange;
    if (newA >= 360) {
        newA=newA-360;
    }
    if (newA <= 0) {
        newA=newA+360;
    }
    g->angle = newA;

    //rotate angle so it is less than 90 (lower right quadrant visually)
    int workingA = 0;
    if (newA>= 0 && newA < 90)
        workingA = newA;
    else if (newA >= 90 && newA < 180)
        workingA = newA - 90;
    else if (newA >= 180 && newA < 270)
        workingA = newA - 180;
    else if (newA >= 270 && newA <= 360)
        workingA = newA - 270;

    //get delta X and delta y
    double rads = getRad(workingA);
    double deltaX = sin(rads) * g->length;
    double deltaY = cos(rads) * g->length;

    //rotate deltax & y back
    if (newA >= 0 && newA < 90) {
        g->x2 = g->x + deltaX;
        g->y2 = g->y + deltaY;
    } else if (newA >= 90 && newA < 180) {
        g->x2 = g->x + deltaY;
        g->y2 = g->y - deltaX;
    } else if (newA >= 180 && newA < 270) {
        g->x2 = g->x - deltaX;
        g->y2 = g->y - deltaY;
    } else if (newA >= 270 && newA <= 360) {
        g->x2 = g->x - deltaY;
        g->y2 = g->y + deltaX;
    }
    return;
}

void addBullet(shared_ptr<vector<shared_ptr<MovingCircle>>> cs, shared_ptr<Gun> gun) {
    shared_ptr<MovingCircle> b = make_shared<MovingCircle>();
    b->p.x = gun->x2;
    b->p.y = gun->y2;
    int deltaX = gun->x2 - gun->x;
    int deltaY = gun->y2 - gun->y;
    b->prevP.x = gun->x2 - (0.25 * deltaX);
    b->prevP.y = gun->y2 - (0.25 * deltaY);

    b->rgb.b = 20;
    b->rgb.g = 20;
    b->rgb.r = 200;
    b->rgb.a = 255;
    cs->emplace_back(b);

    return;
}

void growRipple(shared_ptr<Ripple> t) {
    t->r = t->r + t->expand_speed;
}

double getDistanceMove(shared_ptr<MovingCircle> c) {
    return sqrt(pow((c->p.x - c->prevP.x), 2) + pow((c->p.y - c->prevP.y), 2));
}

void moveCircle(shared_ptr<MovingCircle> c, bool wrap) {
    Position next;
    double deltaX = c->p.x - c->prevP.x;
    double deltaY = c->p.y - c->prevP.y;
    next.x = c->p.x + deltaX;
    next.y = c->p.y + deltaY;
    if (!wrap) {
        c->prevP.x = c->p.x;
        c->p.x = next.x;
        c->prevP.y = c->p.y;
        c->p.y = next.y;
        return;
    }
    //horiz wrap if needed
    if (next.x >= SCREEN_WIDTH) {
        c->p.x = SCREEN_WIDTH - c->p.x;
        c->prevP.x = c->p.x - deltaX;
    } else if (next.x <= 0) {
        c->p.x = SCREEN_WIDTH + next.x;
        c->prevP.x = c->p.x - deltaX;
    } else {
        c->prevP.x = c->p.x;
        c->p.x = next.x;
    }
    // vertical wrpa if needed
    if (next.y >= SCREEN_HEIGHT) {
        c->p.y = SCREEN_HEIGHT - c->p.y;
        c->prevP.y = c->p.y - deltaY;
    } else if (next.y <= 0) {
        c->p.y = SCREEN_HEIGHT + next.y;
        c->prevP.y = c->p.y - deltaY;
    } else {
        c->prevP.y = c->p.y;
        c->p.y = next.y;
    }

    return;
}

void cleanup(SDL_Window * window, SDL_Renderer * renderer) {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
}

void cleanup(SDL_Window * window) {
    SDL_DestroyWindow(window);
}

void cleanup(SDL_Renderer * renderer) {
    SDL_DestroyRenderer(renderer);
}

bool isCollided(shared_ptr<Circle> c1, shared_ptr<Circle> c2) {
    int dx = c1->p.x - c2->p.x;
    int dy = c1->p.y - c2->p.y;
    int distance = sqrt(dx * dx + dy * dy);

    if (distance < c1->r + c2->r) {
        // collision detected!
        return true;
    }
    return false;
}

void help(){
  printf("Left arrow: rotates gun counter-clockwise.\n");
  printf("Right arrow: rotates gun clockwise.\n");
  printf("Space: shoots.\n");
  printf("ESC: quits.\n");

  printf("See also README* in install directory.\n");
}

int main(int argc, char *argv[]) {
    bool quit = false;
    int delay = 0; // per iter delay to adjust for current system

    if (argc == 2 ){
        if (strcmp(argv[1], "info") == 0){
            get_info();
            return 1;
        } else if (strcmp(argv[1], "help") == 0){
            help();
            return 1;
        } else {
            delay = atoi(argv[1]);
        }
    }

    srand(time(0));

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window;
    SDL_Renderer *renderer;

    window  = SDL_CreateWindow( "Ripples", 
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                SCREEN_WIDTH,
                                SCREEN_HEIGHT,
                                SDL_WINDOW_FULLSCREEN||SDL_WINDOW_OPENGL);
    if( window == NULL ){
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        return false;
    } else {
        printf("Window created.\n");
    }
    setScreen();
     
    SDL_SysWMinfo info;

    if (SDL_GetWindowWMInfo(window,&info)) {
      if (info.subsystem == SDL_SYSWM_WAYLAND) {
        printf("Is Wayland\n");
      } else {
        printf("Not a Wayland system\n");
      }
    } else {
      printf("Unable to get Wayland window info. %s\n", SDL_GetError());
    }
    
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    if ( window == nullptr || renderer == nullptr ) {
        cout << "SDL setup error. Quitting" << endl;
        return 1;
    }

    shared_ptr<Gun> gun = make_shared<Gun>();
    gun->angle = 0;
    gun-> length = 50;
    gun->x = 200;
    gun->y = SCREEN_HEIGHT/2;

    // vector of moving bullets
    shared_ptr<vector<shared_ptr<MovingCircle>>> bullets = make_shared<vector<shared_ptr<MovingCircle>>>();

    // vector of grid circles
    shared_ptr<vector<shared_ptr<Circle>>> grid_circles = make_shared<vector<shared_ptr<Circle>>>();

    // vector of "target" circles
    shared_ptr<vector<shared_ptr<MovingCircle>>> ripples = make_shared<vector<shared_ptr<MovingCircle>>>();

    int x_iters = int(SCREEN_WIDTH/20);
    int y_iters = int(SCREEN_HEIGHT/20);
    int  grid = 20;
    for ( int x=0; x <= x_iters; ++x)
    {
        for (int y=0; y <= y_iters; ++y)
        {
            addCircle(grid_circles, x * grid, y * grid, 200,200,200, 50);
        }
    }

    SDL_Event e;
    int mx = gun->x - 40;
    int my = gun->y + 40;
    rotateGun(gun,1);

    int idx = -1;
    // used to handle KEYUP/DOWN for aiming the gun
    bool aim = false;
    while (!quit) {
        idx += 1;
        clock_t startTime = clock();

        SDL_SetRenderDrawColor( renderer, 20,20,20, 255 );
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor( renderer, 200,20,20, 255 );
        filledCircleRGBA(renderer, gun->x, gun->y, 5, 200, 20, 20, 255);

        // add a new target circle every 25 cycles up to a limit
        if ( idx % 25 == 0 && idx < 500) {
            int x, y;
            x = rand() % SCREEN_WIDTH/2;
            x = x+SCREEN_WIDTH/2;
            y = rand() % SCREEN_HEIGHT/2;
            x = y+SCREEN_HEIGHT/2;
            addMovingCircle(ripples, x, y);
        }

        //check for user iput
        while (SDL_PollEvent(&e)) {
            // user closes the window
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            // user clicks the mouse
            /*if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                //addMovingCircle(ripples, x, y);
            }
	    */
            //if (e.type == SDL_FINGERDOWN) {
                //addRipple(ripples, e.tfinger.x, e.tfinger.y);
                //addGridToRipple((*ripples->back()), (*grid_circles));
            //}
            int amt = 8;
            if (e.type == SDL_KEYUP) {
              switch(e.key.keysym.scancode){
                  case SDL_SCANCODE_LEFT:
                      aim = false;
                      break;
                  case SDL_SCANCODE_RIGHT:
                      aim = false;
                      break;
                  case SDL_SCANCODE_UP:
                      aim = false;
                      break;
                  case SDL_SCANCODE_DOWN:
                      aim = false;
                      break;
              }
            }
            if (aim){
              switch (last_aim){
                case 3:
                    mx -= amt;
                    rotateGun(gun,2);
                    last_aim = 3;
                case 1:
                    mx += amt;
                    rotateGun(gun,1);
                    last_aim = 1;
                /*case 0:
                    mx += amt;
                    rotateGun(gun,1);
                    last_aim = 1;
                */

              }
            }
            if (e.type == SDL_KEYDOWN) {
                cout << "key down: " << SDL_GetKeyName(e.key.keysym.sym) << endl;
                if (e.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                    mx -= amt;
                    rotateGun(gun,2);
                    last_aim = 3;
                    aim = true;
                } else if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                    mx += amt;
                    rotateGun(gun,1);
                    last_aim = 1;
                    aim = true;
                } else if (e.key.keysym.scancode == SDL_SCANCODE_UP) {
                    my -= amt;
                    last_aim = 0;
                    aim = true;
                } else if (e.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                    my += amt;
                    last_aim = 2;
                    aim = true;
                } else if (e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                    addBullet(bullets, gun);
                    aim = false;
                } else if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    quit = true;
                    aim = false;
                }
            }
        }

        //render grid cricles TTO SLOW FOR RPI3
        /*for( shared_ptr<Circle> &c : *grid_circles ) {
            SDL_SetRenderDrawColor(renderer, c->rgb.b, c->rgb.g, c->rgb.r, c->rgb.a);
            int res = filledCircleRGBA(renderer, c->p.x, c->p.y, c->r, c->rgb.r, c->rgb.g, c->rgb.b, c->rgb.a);
            if (res == -1)
                cout << "render grid circles ERROR res: " << res << endl;
        }*/

        // reset ripples to exclude those that have expanded too much
        shared_ptr<vector<shared_ptr<MovingCircle>>> remaining_ripples = make_shared<vector<shared_ptr<MovingCircle>>>();
        for( shared_ptr<MovingCircle> &c : *ripples )
        {
            if (c->r <= SCREEN_WIDTH)// this can be improved
                remaining_ripples->emplace_back(c);
        }
        ripples->clear();
        for ( shared_ptr<MovingCircle> &rr : *remaining_ripples )
        {
            ripples->emplace_back(rr);
        }
        remaining_ripples->clear();
        
        //move ripple cricles
        for( shared_ptr<MovingCircle> &c : *ripples )
        {
            moveCircle(c,true);
            //growRipple(c);
        }

        //move bullets
        shared_ptr<vector<shared_ptr<MovingCircle>>> new_bullets = make_shared<vector<shared_ptr<MovingCircle>>>();
        for( shared_ptr<MovingCircle> &c : *bullets ) {
            moveCircle(c,false);
            if (c->p.x < SCREEN_WIDTH && c->p.x > 0 && c->p.y < SCREEN_HEIGHT && c->p.y > 0 ) {
                new_bullets->emplace_back(c); // keep if still on screen
            }
        }
        bullets = new_bullets;

        bool collision = false;
        for( shared_ptr<MovingCircle> &b : *bullets ) {
            for( shared_ptr<MovingCircle> &r : *ripples ) {
                collision = isCollided(b, r);
                if (collision) {
                    //cout << "Collision!" << endl;
                    b->collided = true;
                    b->collision_render_count += 1;
                    r->collided = true;
                    r->collision_render_count += 1;
                }
            }
        }

        //render ripple cricles
        for( shared_ptr<MovingCircle> &c : *ripples )
        {
            SDL_SetRenderDrawColor(renderer, c->rgb.b, c->rgb.g, c->rgb.r, c->rgb.a);
            if ( ! c->collided ) {
                int res = circleRGBA(renderer, c->p.x, c->p.y, c->r, c->rgb.r, c->rgb.g, c->rgb.b, c->rgb.a);
                //if (res == -1)
                //    cout << "=========== render ripple ERROR res: " << res << endl;
            } else { //draw as collided and update
                int res = circleRGBA(renderer, c->p.x, c->p.y, c->r, 200, 100, 100, c->rgb.a);
                SDL_SetRenderDrawColor(renderer, 200, 200, 50, 200);
                //filledCircleRGBA(renderer, c->p.x, c->p.y, c->r, 230, 10, 10, 255);
                c->collision_render_count += 1;
            }
            if (c->collision_render_count < 20) {
                remaining_ripples->emplace_back(c);
            }
        }
        ripples->clear();
        for ( shared_ptr<MovingCircle> &rr : *remaining_ripples )
        {
            ripples->emplace_back(rr);
        }
        remaining_ripples->clear();

        //Render bullets
        for( shared_ptr<MovingCircle> &c : *bullets ) {
            SDL_SetRenderDrawColor( renderer, c->rgb.b, c->rgb.g, c->rgb.r, c->rgb.a);

            int res = filledCircleRGBA(renderer, c->p.x, c->p.y, c->r, c->rgb.r, c->rgb.g, c->rgb.b, c->rgb.a);
            if (res == -1)
                cout << "=========== ERROR res: " << res << endl;
        }

        // render gun
        SDL_SetRenderDrawColor( renderer, 200, 100, 200, 255 );
        SDL_RenderDrawLine(renderer, gun->x, gun->y, gun->x2, gun->y2);

        //Update the screen
        SDL_RenderPresent(renderer);
        SDL_Delay(delay);
        clock_t endTime = clock();
        clock_t ellapsedTime = endTime - startTime;
        float ellapsed = (float)ellapsedTime/CLOCKS_PER_SEC;
        //cout << "elapsed time: " << ellapsed << endl;;
        if (ellapsed < 0.0333) // 30 franmes per second
            SDL_Delay(0.0333 - ellapsed);
    }

    cleanup(window, renderer);
    SDL_Quit();

    return 0;
}
