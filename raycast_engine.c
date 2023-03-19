#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

/*
    zu koordinaten addieren -> verschiebt ursprung in mitte des fensters 
*/
#define X_OFFSET WINDOW_WIDTH / 2
#define Y_OFFSET WINDOW_HEIGHT / 2

#define RAY_STEP_LENGTH .01
#define CLOCKS_PER_TICK CLOCKS_PER_SEC / TICKS_PER_SECOND

#define LEFT_SIDE_OF_THE_WINDOW -1
#define RIGHT_SIDE_OF_THE_WINDOW 1

#define TEXTURE_HEIGHT 64
#define TEXTURE_WIDTH 64
#define NUMBER_OF_TEXTURES 2

#define WALL_SECTION_WIDTH (double) 1 / TEXTURE_WIDTH

//ansonsten meckert vscode rum, dass M_PI entweder nicht definiert oder erneut definiert ist
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
#ifndef M_PI_2
#define M_PI_2 M_PI / 2
#endif

#define TICKS_PER_SECOND 60
#define MOVEMENT_SPEED .03
#define ROTATION_SPEED M_PI / 300
#define FOV 40

#define draw_start_y(distance) (WINDOW_HEIGHT / 2 - WINDOW_HEIGHT / distance)
#define draw_end_y(distance) (WINDOW_HEIGHT / 2 + WINDOW_HEIGHT / distance)

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
// struct Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 1, .direction.y = 0, .plane.x = 0, .plane.y = FOV / 90.0};

SDL_Color wall_texture[NUMBER_OF_TEXTURES][TEXTURE_HEIGHT][TEXTURE_WIDTH];
const SDL_Color FLOOR_COLOR = {.r = 100, .g = 100, .b = 100};
const SDL_Color SKY_COLOR = {.r = 50, .g = 50, .b = 50};

const SDL_Rect FLOOR_RECT = {.x = 0, .y = WINDOW_HEIGHT / 2, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT / 2};    //wird genutzt um den "boden" darzustellen 

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

void init_textures()
{
    for (int i = 0; i < TEXTURE_HEIGHT; i++)
    {
        for (int j = 0; j < TEXTURE_WIDTH; j++)
        {
            wall_texture[0][i][j].r = 80;
            wall_texture[0][i][j].g = 3 * j;
            wall_texture[0][i][j].b = 3 * i;
        }
    }

    for (int i = 0; i < TEXTURE_HEIGHT; i++)
    {
        for (int j = 0; j < TEXTURE_WIDTH; j++)
        {
            wall_texture[1][i][j].r = 100;
            wall_texture[1][i][j].g = 0;
            wall_texture[1][i][j].b = 100;
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

    struct Vector destination;

    /*
        vorwärts- / rückwärtsbewegung
    */
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
    legt auf fest, welche textur die wand besitzen soll
*/
double raycast(struct Vector *offset, int *texture_id, double *texture_x)
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
            *texture_x = ray_point.y;
            distance = distance_between_points(&cam.position, &ray_point); 
            return distance;
        }

        ray_point.y += y_step_length;
        if (point_is_a_wall(&ray_point))
        {
            *texture_id = map[(int) ray_point.y][(int) ray_point.x] - 1;
            *texture_x = ray_point.x;
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

        double wall_x;

        double distance = raycast(&direction_offset, &texture_id, &wall_x);
        

        if (distance < 0) continue;

        /*
            berechnet anfangs- und endpunkt der wand
        */
        double draw_start = draw_start_y(distance), draw_end = draw_end_y(distance);
        if (draw_start < 0) draw_start = 0;
        if (draw_end > WINDOW_HEIGHT) draw_end = WINDOW_HEIGHT - 1;

        /*
            anzahl an pixeln des fensters die einem pixel der textur der wand entsprechen
        */
        double wall_section_height = (draw_end - draw_start) / TEXTURE_HEIGHT;

        double end_of_current_wall_section = draw_start + wall_section_height;
        int texture_y = 0, texture_x = -1;

        for (double x = (int) wall_x; x <= wall_x; x += WALL_SECTION_WIDTH) texture_x++;
        
        for (int y = draw_start; y <= draw_end; y++)
        {
            if (y >= end_of_current_wall_section)
            {
                texture_y++;
                end_of_current_wall_section += wall_section_height;
            }

            pixels[(X_OFFSET + i + WINDOW_WIDTH * y) * 4] = wall_texture[texture_id][texture_y][texture_x].r;
            pixels[(X_OFFSET + i + WINDOW_WIDTH * y) * 4 + 1] = wall_texture[texture_id][texture_y][texture_x].g;
            pixels[(X_OFFSET + i + WINDOW_WIDTH * y) * 4 + 2] = wall_texture[texture_id][texture_y][texture_x].b;
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
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

    init_textures();

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

        render(renderer, texture);
    }

    return 0;
}