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
    --------------
*/

struct Vector
{
    double x, y;
};

struct Camera
{
    struct Vector position, direction, plane;
};

bool game_is_running = true, game_is_paused = false;;

int map_width = 10, map_height = 10;
int map[10][10] = {
    {1,1,1,2,1,2,2,0,0,1},
    {1,2,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,1,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2}, 
    {1,2,1,2,1,2,1,1,1,1}};

int rotation_direction = 0, movement_direction = 0, side_direction = 0;

struct Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 0, .direction.y = -1, .plane.x = FOV / 90.0, .plane.y = 0};

const SDL_Rect FLOOR_RECT = {.x = 0, .y = SCREEN_HEIGHT / 2, .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT / 2};    //wird genutzt um den "boden" darzustellen 

bool point_is_on_map(struct Vector *point)
{
    return (point->x >= 0 && point->x < map_width) && (point->y >= 0 && point->y < map_height);
}

bool point_is_a_wall(struct Vector *point)
{
    return (!map[(int) point->y][(int) point->x] == 0);
}

/*
    liest eine map aus einer datei ein
*/
void load_map()
{
    char file_path[50];
    scanf("%50s", file_path);
}

void events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                game_is_running = false;
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

                case SDLK_l:
                    load_map();
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

    /*
        vorwärts- / rückwärtsbewegung
    */
    struct Vector destination;

    destination.x = cam.position.x + movement_direction * cam.direction.x * MOVEMENT_SPEED;
    destination.y = cam.position.y + movement_direction * cam.direction.y * MOVEMENT_SPEED;

    /*
        seitwärtsbewegung
    */
    destination.x += side_direction * cam.direction.y * MOVEMENT_SPEED;
    destination.y -= side_direction * cam.direction.x * MOVEMENT_SPEED;

    if (!point_is_on_map(&destination) || point_is_a_wall(&destination)) return;
    cam.position.x = destination.x;
    cam.position.y = destination.y;
}

/*
    "schießt" einen strahl von der position der kamera aus und gibt die distanz der nächsten wand zurück.
    die richtung des strahls setzt sich aus dem richtungsvektor der kamera und dem offset (teil des plane vectors der kamera) zusammen.
*/
double cast_ray(struct Vector *offset, int *color)
{
    struct Vector ray_point = {.x = cam.position.x, .y = cam.position.y};
    struct Vector ray_direction = {.x = cam.direction.x + offset->x, .y = cam.direction.y + offset->y};
    
    double delta_x = 0, delta_y = 0, distance = -1;

    /*
        geht so lange in richtung des strahls, bis das ende der map oder eine wand erreicht ist
    */
    while (point_is_on_map(&ray_point))
    {
        /*
            verlängert den strahl um RAY_STEP_LENGTH einheiten
        */
        ray_point.x += ray_direction.x * RAY_STEP_LENGTH;
        ray_point.y += ray_direction.y * RAY_STEP_LENGTH;

        if (!point_is_a_wall(&ray_point)) continue;
        
        /*
            übergibt farbe der getroffenen wand
        */
        *color = map[(int) ray_point.y][(int) ray_point.x];

        /*
            berechnung der distanz
        */
        delta_x = fabs(cam.position.x - ray_point.x);
        delta_y = fabs(cam.position.y - ray_point.y);

        distance = sqrt((delta_x * delta_x) + (delta_y * delta_y));

        break;
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

/*
    gibt den y-wert zurück, bei dem eine linie für die übergebene entfernung anfangen / enden sollte
    distance für anfang und -distance für ende übergeben
*/
int draw_start_stop_y(double distance)
{
    return SCREEN_HEIGHT / 2 - SCREEN_HEIGHT / distance;
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

        /*
            rechte seite vom bildschirm
        */
        double distance = cast_ray(&direction_offset, &color);
        set_color(renderer, color);        
        if (distance > 0) SDL_RenderDrawLine(renderer, X_OFFSET + i, draw_start_stop_y(distance), X_OFFSET + i, draw_start_stop_y(-distance));
        
        /*
            linke seite vom bildschirm
        */
        direction_offset.x = -direction_offset.x, direction_offset.y = -direction_offset.y;
        distance = cast_ray(&direction_offset, &color);
        set_color(renderer, color);
        if (distance > 0) SDL_RenderDrawLine(renderer, X_OFFSET - i, draw_start_stop_y(distance), X_OFFSET - i, draw_start_stop_y(-distance));
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char const *argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    while (game_is_running)
    {
        events();

        update_camera();

        render(renderer);
    }

    return 0;
}