#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

/*
    zu koordinaten addieren -> verschiebt ursprung in mitte des fensters 
*/
#define X_OFFSET SCREEN_WIDTH / 2
#define Y_OFFSET SCREEN_HEIGHT / 2

#define RAY_STEP_LENGTH .05

#define MOVEMENT_SPEED .01
#define ROTATION_SPEED M_PI / 2250
#define FOV 45

/*
    Raycast-Engine
    -----
    TODO:
        fischauge??
        seitenerkennung
*/

struct Vector
{
    double x, y;
};

struct Camera
{
    struct Vector position, direction, plane;
};

bool is_running = true;

int map_width = 10, map_height = 10;
int map[10][10] = {
    {1,1,1,2,1,2,2,2,1,1},
    {1,2,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,1,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2}, 
    {1,2,1,2,1,2,1,2,1,1}};

int rotation_direction = 0, movement_direction = 0, side_direction = 0;
bool turn_left = false, turn_right = false;

struct Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 0, .direction.y = -1, .plane.x = FOV / 90.0, .plane.y = 0};

const SDL_Rect FLOOR_RECT = {.x = 0, .y = SCREEN_HEIGHT / 2, .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT / 2};    //wird genutzt um den "boden" darzustellen 

void events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                is_running = false;
                SDL_Quit();
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_RIGHT:
                    rotation_direction = 1;
                    break;
                
                case SDLK_LEFT:
                    rotation_direction = -1;
                    break;

                case SDLK_w:
                    movement_direction = 1;
                    break;

                case SDLK_s:
                    movement_direction = -1;
                    break;

                case SDLK_a:
                    side_direction = 1;
                    break;

                case SDLK_d:
                    side_direction = -1;
                    break;                    

                default:
                    break;
                }
                break;
            
            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                case SDLK_RIGHT:
                    rotation_direction = 0;
                    break;
                
                case SDLK_LEFT:
                    rotation_direction = 0;
                    break;

                case SDLK_w:
                    movement_direction = 0;
                    break;

                case SDLK_s:
                    movement_direction = 0;
                    break;

                case SDLK_a:
                    side_direction = 0;
                    break;

                case SDLK_d:
                    side_direction = 0;
                    break;

                default:
                    break;
                }
                break;

            default:
                break;
        }
    }
}

/*
    rotiert die kamera um den übergebenen winkel (bogenmaß!).
    positiver winkel -> drehung nach rechts
*/
void rotate_camera(double rotation_angle)
{
    double old_x = cam.direction.x, old_y = cam.direction.y;

    cam.direction.x = cos(rotation_angle) * old_x - sin(rotation_angle) * old_y;
    cam.direction.y = sin(rotation_angle) * old_x + cos(rotation_angle) * old_y;

    old_x = cam.plane.x, old_y = cam.plane.y;

    cam.plane.x = cos(rotation_angle) * old_x - sin(rotation_angle) * old_y;
    cam.plane.y = sin(rotation_angle) * old_x + cos(rotation_angle) * old_y;
}

/*
    bewegung und rotation der kamera
*/
void update_camera()
{
    if (rotation_direction != 0) rotate_camera(rotation_direction * ROTATION_SPEED);

    //if (turn_left) rotate_camera(-ROTATION_SPEED);
    //if (turn_right) rotate_camera(ROTATION_SPEED);

    /*
        vorwärts- / rückwärtsbewegung
    */
    double next_x = cam.position.x + movement_direction * cam.direction.x * MOVEMENT_SPEED;
    double next_y = cam.position.y + movement_direction * cam.direction.y * MOVEMENT_SPEED;

    if (map[(int) next_y][(int) next_x] != 0) return;
    if (next_x >= 0 && next_x < map_width) cam.position.x += movement_direction * cam.direction.x * MOVEMENT_SPEED;
    if (next_y >= 0 && next_y < map_height) cam.position.y += movement_direction * cam.direction.y * MOVEMENT_SPEED;

    /*
        seitwärtsbewegung
    */
    next_x = cam.position.x + side_direction * cam.direction.y * MOVEMENT_SPEED;
    next_y = cam.position.y - side_direction * cam.direction.x * MOVEMENT_SPEED;

    if (map[(int) next_y][(int) next_x] != 0) return;
    if (next_x >= 0 && next_x < map_width) cam.position.x += side_direction * cam.direction.y * MOVEMENT_SPEED;
    if (next_y >= 0 && next_y < map_height) cam.position.y -= side_direction * cam.direction.x * MOVEMENT_SPEED;
}

/*
    "schießt" einen strahl von der position der kamera aus und gibt die distanz der nächsten wand zurück.
    die richtung des strahls setzt sich aus dem richtungsvektor der kamera und dem offset (teil des plane vectors der kamera) zusammen.
*/
double cast_ray(struct Vector offset, int *color)
{
    double x = cam.position.x, y = cam.position.y;
    double delta_x = 0, delta_y = 0, distance = -1;

    /*
        geht so lange in richtung des strahls, bis das ende der map oder eine wand erreicht ist
    */
    while (x < map_width && x >= 0 && y < map_height && y >= 0)
    {
        /*
            verlängert den strahl um RAY_STEP_LENGTH einheiten
        */
        x += (cam.direction.x + offset.x) * RAY_STEP_LENGTH;
        y += (cam.direction.y + offset.y) * RAY_STEP_LENGTH;

        if (map[(int) y][(int) x] != 0)
        {
            /*
                übergibt farbe der getroffenen wand
            */
            *color = map[(int) y][(int) x];

            /*
                berechnung der distanz
            */
            delta_x = fabs(cam.position.x + offset.x - x);
            delta_y = fabs(cam.position.y + offset.y - y);

            distance = sqrt((delta_x * delta_x) + (delta_y * delta_y));

            break;
        }
    }

    return distance;
}

/*
    legt die zu malende farbe abhängig davon fest, welche art von wand getroffen wurde
*/
void set_color(SDL_Renderer *renderer, int color)
{
    switch (color)
    {
    case 1:
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        break;

    case 2:
        SDL_SetRenderDrawColor(renderer, 130, 130, 130, 255);
        break;

    default:
        break;
    }
}

void render(SDL_Renderer *renderer)
{
    /*
        cleart den bildschirm
    */
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);

    /*
        malt helleren boden
    */
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &FLOOR_RECT);
    
    /*
        führt für jeden x wert des bildschirms einen raycast durch
    */
    for (int i = 0; i < SCREEN_WIDTH / 2; i++)
    {
        /*
            offset um wie viel sich die richtung des strahls von der richtung der kamera unterscheidet
        */
        struct Vector direction_offset = {.x = cam.plane.x / (SCREEN_WIDTH / 2) * i, .y = cam.plane.y / (SCREEN_WIDTH / 2) * i};

        int color = 0;

        double distance = cast_ray(direction_offset, &color);
        set_color(renderer, color);        
        if (distance > 0) SDL_RenderDrawLine(renderer, X_OFFSET + i, distance * 20, X_OFFSET + i, SCREEN_HEIGHT - distance * 20);
        
        direction_offset.x = -direction_offset.x, direction_offset.y = -direction_offset.y;
        distance = cast_ray(direction_offset, &color);
        set_color(renderer, color);
        if (distance > 0) SDL_RenderDrawLine(renderer, X_OFFSET - i, distance * 20, X_OFFSET - i, SCREEN_HEIGHT - distance * 20);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char const *argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);   

    while (is_running)
    {
        events();

        update_camera();

        render(renderer);
    }

    return 0;
}