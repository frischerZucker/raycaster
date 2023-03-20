#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "textures.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

/*
    zu koordinaten addieren -> verschiebt ursprung in mitte des fensters 
*/
#define X_OFFSET WINDOW_WIDTH / 2
#define Y_OFFSET WINDOW_HEIGHT / 2

#define RAY_STEP_LENGTH .01
#define CLOCKS_PER_TICK CLOCKS_PER_SEC / TICKS_PER_SECOND

#define WALL_SECTION_WIDTH (double) 1 / TEXTURE_WIDTH

//ansonsten meckert vscode rum, dass M_PI entweder nicht definiert oder erneut definiert ist
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
#ifndef M_PI_2
#define M_PI_2 M_PI / 2
#endif

#define TICKS_PER_SECOND 30
#define MOVEMENT_SPEED .04
#define ROTATION_SPEED M_PI / 200
#define FOV 40
#define LIGHT_INTENSITY_MULTIPLIER 5

#define draw_start_y(distance) (Y_OFFSET - WINDOW_HEIGHT / distance)
#define draw_end_y(distance) (Y_OFFSET + WINDOW_HEIGHT / distance)

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
// struct Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 1, .direction.y = 0, .plane.x = 0, .plane.y = FOV / 90.0};

const SDL_Color FLOOR_COLOR = {.r = 100, .g = 100, .b = 100};
const SDL_Color SKY_COLOR = {.r = 50, .g = 50, .b = 50};

uint8_t pixels[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};

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

    // beendet funktion wenn keine bewegung stattfindet um unnötige berechnungen zu sparen
    if (movement_direction == 0 && side_direction == 0) return;

    struct Vector destination = {.x = cam.position.x, .y = cam.position.y};

    /*
        vorwärts- / rückwärtsbewegung
    */
    destination.x += movement_direction * cam.direction.x * MOVEMENT_SPEED;
    destination.y += movement_direction * cam.direction.y * MOVEMENT_SPEED;

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
    setzt zudem einige zusätzlich zum rendern benötigte infos (texture_id & horizontal_position_on_wall)
*/
double raycast(struct Vector *offset, int *texture_id, double *horizontal_position_on_wall)
{
    struct Vector ray_point = {.x = cam.position.x, .y = cam.position.y};
    struct Vector ray_direction = {.x = cam.direction.x + offset->x, .y = cam.direction.y + offset->y};
    
    double x_step_length, y_step_length, distance;    
    x_step_length = ray_direction.x * RAY_STEP_LENGTH;
    y_step_length = ray_direction.y * RAY_STEP_LENGTH;

    /*
        geht so lange in richtung des strahls, bis das ende der map oder eine wand erreicht ist
    */
    while (point_is_on_map(&ray_point))
    {
        ray_point.x += x_step_length;
        if (point_is_a_wall(&ray_point))
        {
            *texture_id = map[(int) ray_point.y][(int) ray_point.x] - 1;
            *horizontal_position_on_wall = ray_point.y;
            distance = distance_between_points(&cam.position, &ray_point); 
            return distance;
        }

        ray_point.y += y_step_length;
        if (point_is_a_wall(&ray_point))
        {
            *texture_id = map[(int) ray_point.y][(int) ray_point.x] - 1;
            *horizontal_position_on_wall = ray_point.x;
            distance = distance_between_points(&cam.position, &ray_point);
            return distance;
        }
    }

    return -1;
}

/*
    berechnet und malt die wände im sichtfeld
*/
void render_walls(uint8_t *pixels)
{
    for (int i = -WINDOW_WIDTH / 2; i < WINDOW_WIDTH / 2; i++)
    {
        /*
            offset um wie viel sich die richtung des strahls von der richtung der kamera unterscheidet
        */
        struct Vector direction_offset = {.x = cam.plane.x / (WINDOW_WIDTH / 2) * i, .y = cam.plane.y / (WINDOW_WIDTH / 2) * i};

        int texture_id = 0;
        double horizontal_position_on_wall; //  x bzw y koordinate des punkts in dem die wand getroffen wird

        double distance = raycast(&direction_offset, &texture_id, &horizontal_position_on_wall);
        if (distance < 0) continue;

        /*
            weiter entfernte wände werden durch geringere lichtintensität dunkler gerendert
        */
        double light_intensity = 1 / distance * LIGHT_INTENSITY_MULTIPLIER;
        if (light_intensity > 1) light_intensity = 1;

        /*
            berechnet anfangs- und endpunkt der wand
        */
        double draw_start = draw_start_y(distance), draw_end = draw_end_y(distance);

        /*
            anzahl an pixeln des fensters die einem pixel der textur der wand entsprechen
        */
        double wall_section_height = (draw_end - draw_start) / TEXTURE_HEIGHT;

        double end_of_current_wall_section = draw_start + wall_section_height;
        int texture_y = 0, texture_x = -1;

        /*
            schaut welchem x-wert der textur die position auf der wand entspricht
        */
        for (double x = (int) horizontal_position_on_wall; x <= horizontal_position_on_wall; x += WALL_SECTION_WIDTH) texture_x++;
        
        /*
            begrenzt draw_start und draw_end auf die dimensionen des fensters
        */
        if (draw_start < 0)
        {
            /*
                verschiebt den startwert der textur-y-koordinate um das aus dem fenster ragende stück zu überspringen
            */
            for (draw_start += wall_section_height; draw_start < 0; draw_start += wall_section_height)
            {
                texture_y++;
                end_of_current_wall_section += wall_section_height;
            }
            draw_start = 0;
        }
        if (draw_end > WINDOW_HEIGHT) draw_end = WINDOW_HEIGHT - 1;

        /*
            malt alle pixel zwischen draw_start und draw_end
        */
        for (int y = draw_start; y <= draw_end; y++)
        {
            if (y >= end_of_current_wall_section)
            {
                texture_y++;
                end_of_current_wall_section += wall_section_height;
            }

            /*
                umwandlung der 2d-textur-koordinaten zu 1d-index
            */
            int position_on_texture = texture_pointer_offset(texture_id) + texture_y * TEXTURE_WIDTH + texture_x;
            /*
                umwandlung der 2d-fenster-koordinaten zu 1d-index
            */
            int position_in_pixel_array = (X_OFFSET + i + WINDOW_WIDTH * y) * 4;

            pixels[position_in_pixel_array] = texture_pointer[position_on_texture].r * light_intensity;
            pixels[position_in_pixel_array + 1] = texture_pointer[position_on_texture].g * light_intensity;
            pixels[position_in_pixel_array + 2] = texture_pointer[position_on_texture].b * light_intensity;
        }
    }
}

void render(SDL_Renderer *renderer, SDL_Texture *texture)
{
    //  malt die decke
    for (int i = 0; i < WINDOW_HEIGHT * WINDOW_WIDTH / 2; i++)
    {
        pixels[4 * i] = SKY_COLOR.r;        //  rot
        pixels[4 * i + 1] = SKY_COLOR.g;    //  grün
        pixels[4 * i + 2] = SKY_COLOR.b;    //  blau
    }
    //  malt den boden
    for (int i = WINDOW_HEIGHT * WINDOW_WIDTH / 2; i < WINDOW_HEIGHT * WINDOW_WIDTH; i++)
    {
        pixels[4 * i] = FLOOR_COLOR.r;      //  rot
        pixels[4 * i + 1] = FLOOR_COLOR.g;  //  grün
        pixels[4 * i + 2] = FLOOR_COLOR.b;  //  blau
    }

    render_walls(pixels);

    int texture_pitch = 0;
    void* texture_pixels = NULL;
    
    SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch);
    memcpy(texture_pixels, pixels, texture_pitch * WINDOW_HEIGHT);
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
    SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

    init_textures();

    clock_t time_of_last_tick, current_time;
    time_of_last_tick = clock();

    while (game_is_running)
    {
        events();

        current_time = clock();
        
        fps(time_of_last_tick);

        /*
            sorgt dafür das logik im if-block nicht beliebig oft, sondern nur TICKS_PER_SECOND oft pro sekunde ausgeführt wird
        */
        if((current_time - time_of_last_tick) >= CLOCKS_PER_TICK)
        {
            time_of_last_tick = current_time;
            
            update_camera();
        }

        render(renderer, texture);
    }

    return 0;
}