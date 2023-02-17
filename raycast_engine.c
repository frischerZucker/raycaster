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

#define RAY_STEP_LENGTH .01
#define CLOCKS_PER_TICK CLOCKS_PER_SEC / TICKS_PER_SECOND

//ansonsten meckert vscode rum, dass M_PI entweder nicht definiert oder erneut definiert ist
#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288
#endif

#define TICKS_PER_SECOND 60
#define MOVEMENT_SPEED .03
#define ROTATION_SPEED M_PI / 300
#define FOV 45

#define draw_start_y(distance) (SCREEN_HEIGHT / 2 - SCREEN_HEIGHT / distance)
#define draw_end_y(distance) (SCREEN_HEIGHT / 2 + SCREEN_HEIGHT / distance)

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

bool game_is_running = true;

int map_width = 10, map_height = 10;
int map[10][10] = {
    {1,1,1,2,1,2,2,0,0,1},
    {1,2,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,1,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,1,2,0,0,2},
    {2,0,0,1,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,2}, 
    {1,2,1,2,1,2,1,1,1,1}};

int rotation_direction = 0, movement_direction = 0, side_direction = 0;

struct Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 0, .direction.y = -1, .plane.x = FOV / 90.0, .plane.y = 0};

const SDL_Rect FLOOR_RECT = {.x = 0, .y = SCREEN_HEIGHT / 2, .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT / 2};    //wird genutzt um den "boden" darzustellen 

uint8_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT * 4] = {0};

bool point_is_on_map(struct Vector *point)
{
    return (point->x >= 0 && point->x < map_width) && (point->y >= 0 && point->y < map_height);
}

bool point_is_a_wall(struct Vector *point)
{
    return (map[(int) point->y][(int) point->x] != 0);
}

double distance_between_points(struct Vector *p1, struct Vector *p2)
{
    double delta_x = fabs(p1->x - p2->x);
    double delta_y = fabs(p1->y - p2->y);

    return sqrt((delta_x * delta_x) + (delta_y * delta_y));
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
    
    double delta_x = 0, delta_y = 0, distance;    

    /*
        geht so lange in richtung des strahls, bis das ende der map oder eine wand erreicht ist
    */
    while (point_is_on_map(&ray_point))
    {
        ray_point.x += ray_direction.x * RAY_STEP_LENGTH;
        if (point_is_a_wall(&ray_point))
        {
            *color = map[(int) ray_point.y][(int) ray_point.x] * 3;
            distance = distance_between_points(&cam.position, &ray_point);
            return distance;
        }

        ray_point.y += ray_direction.y * RAY_STEP_LENGTH;
        if (point_is_a_wall(&ray_point))
        {
            *color = map[(int) ray_point.y][(int) ray_point.x];
            distance = distance_between_points(&cam.position, &ray_point);
            return distance;
        }
    }

    return -1;
}

/*
    legt die zu malende farbe abhängig davon fest, welche art von wand getroffen wurde
*/
void set_color(int color_id, SDL_Color *color)
{
    switch (color_id)
    {
    case 1:
        color->r = 20;
        color->g = 20;
        color->b = 20;
        break;

    case 2:
        color->r = 80;
        color->g = 80;
        color->b = 80;
        break;

    case 3:
        color->r = 25;
        color->g = 25;
        color->b = 25;
        break;

    case 6:
        color->r = 85;
        color->g = 85;
        color->b = 85;
        break;

    default:
        break;
    }
}

/*
    veraltet & funktioniert aktuell aufgrund neuer parameter für set_color() nicht, durch render_textured ersetzt
*/
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
    for (int i = 0; i <= SCREEN_WIDTH / 2; i++)
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
        //set_color(renderer, color);        
        if (distance > 0) SDL_RenderDrawLine(renderer, X_OFFSET + i, draw_start_y(distance), X_OFFSET + i, draw_end_y(distance));
        
        /*
            linke seite vom bildschirm
        */
        direction_offset.x = -direction_offset.x, direction_offset.y = -direction_offset.y;
        distance = cast_ray(&direction_offset, &color);
        //set_color(renderer, color);
        if (distance > 0) SDL_RenderDrawLine(renderer, X_OFFSET - i, draw_start_y(distance), X_OFFSET - i, draw_end_y(distance));
    }

    SDL_RenderPresent(renderer);
}

void render_textured(SDL_Renderer *renderer, SDL_Texture *texture)
{
    //  malt die decke
    for (int i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH / 2; i++)
    {
        pixels[4 * i] = 50;     //  rot
        pixels[4 * i + 1] = 50; //  grün
        pixels[4 * i + 2] = 50; //  blau
    }
    //  malt den boden
    for (int i = SCREEN_HEIGHT * SCREEN_WIDTH / 2; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++)
    {
        pixels[4 * i] = 100;        //  rot
        pixels[4 * i + 1] = 100;    //  grün
        pixels[4 * i + 2] = 100;    //  blau
    }

    /*
        rechte seite vom bildschirm
    */
    for (int i = 0; i < SCREEN_WIDTH / 2; i++)
    {
        /*
            offset um wie viel sich die richtung des strahls von der richtung der kamera unterscheidet
        */
        struct Vector direction_offset = {.x = cam.plane.x / (SCREEN_WIDTH / 2) * i, .y = cam.plane.y / (SCREEN_WIDTH / 2) * i};

        int color_id = 0;
        SDL_Color color;

        double distance = cast_ray(&direction_offset, &color_id);
        set_color(color_id, &color);

        if (distance < 0) continue;
        for (int y = draw_start_y(distance); y <= draw_end_y(distance); y++)
        {
            if (y < 0 || y >= SCREEN_HEIGHT) continue;
            pixels[(X_OFFSET + i + SCREEN_WIDTH * y) * 4] = color.r;
            pixels[(X_OFFSET + i + SCREEN_WIDTH * y) * 4 + 1] = color.g;
            pixels[(X_OFFSET + i + SCREEN_WIDTH * y) * 4 + 2] = color.b;
        }
    }
    /*
        linke seite vom bildschirm
    */
    for (int i = 0; i <= SCREEN_WIDTH / 2; i++)
    {
        struct Vector direction_offset = {.x = -cam.plane.x / (SCREEN_WIDTH / 2) * i, .y = -cam.plane.y / (SCREEN_WIDTH / 2) * i};

        int color_id = 0;
        SDL_Color color;

        double distance = cast_ray(&direction_offset, &color_id);
        set_color(color_id, &color);

        if (distance < 0) continue;
        for (int y = draw_start_y(distance); y <= draw_end_y(distance); y++)
        {
            if (y < 0 || y > SCREEN_HEIGHT) continue;
            pixels[(X_OFFSET - i + SCREEN_WIDTH * y) * 4] = color.r;
            pixels[(X_OFFSET - i + SCREEN_WIDTH * y) * 4 + 1] = color.g;
            pixels[(X_OFFSET - i + SCREEN_WIDTH * y) * 4 + 2] = color.b;
        }
    }

    int texture_pitch = 0;
    void* texture_pixels = NULL;
    
    SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch);
    memcpy(texture_pixels, pixels, texture_pitch * SCREEN_HEIGHT);
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);
}

void fps(clock_t t0)
{
    static int frames = 0;
    static clock_t t1, t2;
    static bool function_did_run_before = false;

    if (!function_did_run_before)
    {
        t1 = t0;
        function_did_run_before = true;
    }

    frames++;

    t2 = clock();
    if (difftime(t2 / CLOCKS_PER_SEC, t1 / CLOCKS_PER_SEC) >= 1)
    {
        t1 = t2;
        printf("fps: %d\n", frames);
        frames = 0;
    }
}

int main(int argc, char const *argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    clock_t time_of_last_tick, current_time;
    time_of_last_tick = clock();

    while (game_is_running)
    {
        events();

        /*
            sorgt dafür das logik im if-block nicht beliebig oft, sondern nur TICKS_PER_SECOND oft pro sekunde ausgeführt wird
        */
        current_time = clock();
        
        fps(time_of_last_tick);

        if((current_time - time_of_last_tick) >= CLOCKS_PER_TICK)
        {
            time_of_last_tick = current_time;
            
            update_camera();
        }

        render_textured(renderer, texture);
    }

    return 0;
}