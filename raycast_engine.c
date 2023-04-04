#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "defines.h"
#include "math_stuff.c"
#include "textures.c"
#include "sprites.c"

bool game_is_running = true;

int wall_map[MAP_HEIGHT][MAP_WIDTH] = 
{
    {1,1,1,2,1,2,2,0,0,1},
    {1,2,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,1,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,1,2,0,0,2},
    {2,0,0,1,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,2}, 
    {1,2,1,2,1,2,1,1,1,1}
};
int sprite_map[MAP_HEIGHT][MAP_WIDTH] = 
{
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0}
};

int rotation_direction = 0, movement_direction = 0, side_direction = 0;

Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 0, .direction.y = -1, .plane.x = FOV / 90.0, .plane.y = 0};
// struct Camera cam = {.position.x = 5, .position.y = 5, .direction.x = 1, .direction.y = 0, .plane.x = 0, .plane.y = FOV / 90.0};

const SDL_Color FLOOR_COLOR = {.r = 100, .g = 100, .b = 100};
const SDL_Color SKY_COLOR = {.r = 50, .g = 50, .b = 50};

uint8_t pixels[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};

bool point_is_on_map(Vector *point)
{
    return (point->x >= 0 && point->x < MAP_WIDTH) && (point->y >= 0 && point->y < MAP_HEIGHT);
}

bool point_is_a_wall(Vector *point)
{
    return (wall_map[(int) point->y][(int) point->x] != 0);
}

bool sprite_hit(Vector *ray_point, Sprite *sprite)
{
    double x = ray_point->x - sprite->position.x;
    double y = ray_point->y - sprite->position.y;

    return (x * x + y * y <= SPRITE_WIDTH);
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

    Vector destination = {.x = cam.position.x, .y = cam.position.y};

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

void update_sprite_map()
{
    // for (int y = 0; y < MAP_HEIGHT; y++)
    // {
    //     for (int x = 0; x < MAP_WIDTH; x++)
    //     {
    //         sprite_map[y][x] = 0;
    //     }
    // }
    // sprite_map[(int) test_sprite.position.y][(int) test_sprite.position.x] = 1;
}

void update_sprites()
{
    //  updated zum rendern benötigte variablen des sprites
    for (int i = 0; i < sprite_array.count; i++) update_sprite(&sprite_array.sprites[i], &cam);
}

/*
    "schießt" einen strahl von der position der kamera aus und gibt die distanz der nächsten wand zurück.
    die richtung des strahls setzt sich aus dem richtungsvektor der kamera und dem offset (teil des plane vectors der kamera) zusammen.
    setzt zudem einige zusätzlich zum rendern benötigte infos (texture_id & horizontal_position_on_wall)
*/
double raycast(Vector *offset, int *texture_id, double *horizontal_position_on_wall)
{
    Vector ray_point = {.x = cam.position.x, .y = cam.position.y};
    Vector ray_direction = {.x = cam.direction.x + offset->x, .y = cam.direction.y + offset->y};
    
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
            *texture_id = wall_map[(int) ray_point.y][(int) ray_point.x] - 1;
            *horizontal_position_on_wall = ray_point.y;
            distance = distance_between_points(&cam.position, &ray_point); 
            return distance;
        }

        ray_point.y += y_step_length;
        if (point_is_a_wall(&ray_point))
        {
            *texture_id = wall_map[(int) ray_point.y][(int) ray_point.x] - 1;
            *horizontal_position_on_wall = ray_point.x;
            distance = distance_between_points(&cam.position, &ray_point);
            return distance;
        }

        for (int sprite_index = 0; sprite_index < sprite_array.count; sprite_index++)
        {
            //  bricht ab wenn das sprite hinter der kamera ist
            if (sprite_array.sprites[sprite_index].is_behind_camera) continue;
            //  überprüft ob der getestete punkt im sprite liegt und ob er sich der optimalen mittellinie durch das sprite angenähert hat
            if (sprite_hit(&ray_point, &sprite_array.sprites[sprite_index]) && (distance_between_points(&sprite_array.sprites[sprite_index].position, &sprite_array.sprites[sprite_index].point_of_last_colision_with_ray) >= distance_between_points(&ray_point, &sprite_array.sprites[sprite_index].position) || !sprite_array.sprites[sprite_index].is_visible))
            {
                sprite_array.sprites[sprite_index].is_visible = true;
                sprite_array.sprites[sprite_index].point_of_last_colision_with_ray.x = ray_point.x;
                sprite_array.sprites[sprite_index].point_of_last_colision_with_ray.y = ray_point.y;
            }
        }
    }

    return -1;
}

/*
    malt eine vertikale reihe der sichtbaren sprites bei fensterx = i

    muss noch überarbeitet werden
    - verständlichere namen & struktur

    (ist jetzt nicht direkt hier, aber eine sprite map analog zu der map für wände wäre vielleicht praktisch um das raycasten zu beschleunigen)
*/
void draw_sprite_slice(uint8_t *pixels, int i, int sprite_index)
{
    int texture_y = 0, texture_x = 0;

    /*
        b ist ein probepunkt um zu schauen ob sich der punkt an dem das sprite getroffen wurde auf der rechten oder linken seite befindet.
        es wird ein bisschen von cam.plane auf den kollisionspunkt addiert und da cam.plane einigermaßen orthogonal zur richtung des strahls ist,
        entspricht seine richtung in etwa der der achse durch das sprite, auf dem die getroffenen punkte in etwa liegen.
        wenn der dadurch resultierende punkt also näher am mittelpunkt des sprites (sprite.position) ist, liegt der punkt auf der linken seite des sprites.
    */
    Vector b = {.x = cam.plane.x * RAY_STEP_LENGTH + sprite_array.sprites[sprite_index].position.x, .y = cam.plane.y * RAY_STEP_LENGTH + sprite_array.sprites[sprite_index].position.y};
    double d2;
    if (distance_between_points(&sprite_array.sprites[sprite_index].position, &sprite_array.sprites[sprite_index].point_of_last_colision_with_ray) <= distance_between_points(&b, &sprite_array.sprites[sprite_index].point_of_last_colision_with_ray))
    {
        d2 = SPRITE_WIDTH - distance_between_points(&sprite_array.sprites[sprite_index].position, &sprite_array.sprites[sprite_index].point_of_last_colision_with_ray);
    }
    else
    {
        d2 = SPRITE_WIDTH + distance_between_points(&sprite_array.sprites[sprite_index].position, &sprite_array.sprites[sprite_index].point_of_last_colision_with_ray);
    }
    for (double a = 0; a <= d2; a += SPRITE_PIXEL_WIDTH) texture_x += 1;
    // sonst wird texture_x irgendwie zu groß, hab absolut keinen plan warum und weiß nicht wie ich das wirklich fixen soll :/
    if (texture_x > 64) return;

    double draw_start = sprite_array.sprites[sprite_index].draw_start, draw_end = sprite_array.sprites[sprite_index].draw_end,
           sprite_section_height = sprite_array.sprites[sprite_index].sprite_section_height,
           end_of_current_sprite_section = sprite_array.sprites[sprite_index].end_of_current_sprite_section,
           light_intensity = sprite_array.sprites[sprite_index].light_intensity;

    if (draw_start < 0)
    {
        for (draw_start += sprite_section_height; draw_start < 0; draw_start += sprite_section_height)
        {
            texture_y++;
            end_of_current_sprite_section += sprite_section_height;
        }
        draw_start = 0;
    }
    if (draw_end > WINDOW_HEIGHT) draw_end = WINDOW_HEIGHT - 1;

    /*
        malt alle pixel zwischen draw_start und draw_end
    */
    for (int y = draw_start; y <= draw_end; y++)
    {
        if (y >= end_of_current_sprite_section)
        {
            texture_y++;
            end_of_current_sprite_section += sprite_section_height;
        }

        // umwandlung der 2d-textur-koordinaten zu 1d-index
        int position_on_texture = texture_y * TEXTURE_WIDTH + texture_x;
        
        // überspringt durchsichtige pixel
        if (sprite_array.sprites[sprite_index].texture[position_on_texture].a == 0) continue;
        
        // umwandlung der 2d-fenster-koordinaten zu 1d-indexs
        int position_in_pixel_array = (X_OFFSET + i + WINDOW_WIDTH * y) * 4;

        pixels[position_in_pixel_array] = sprite_array.sprites[sprite_index].texture[position_on_texture].r * light_intensity;
        pixels[position_in_pixel_array + 1] = sprite_array.sprites[sprite_index].texture[position_on_texture].g * light_intensity;
        pixels[position_in_pixel_array + 2] = sprite_array.sprites[sprite_index].texture[position_on_texture].b * light_intensity;
    }
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
        Vector direction_offset = {.x = cam.plane.x / (WINDOW_WIDTH / 2) * i, .y = cam.plane.y / (WINDOW_WIDTH / 2) * i};

        int texture_id = 0;
        double horizontal_position_on_wall; //  x bzw y koordinate des punkts in dem die wand getroffen wird

        double distance = raycast(&direction_offset, &texture_id, &horizontal_position_on_wall);

        if (distance < 0)
        {
            for (int j = 0; j < sprite_array.count; j++)
            {
                if (sprite_array.sprites[j].is_visible)
                {
                    draw_sprite_slice(pixels, i, j);
                    sprite_array.sprites[j].is_visible = false;
                }
            }
            continue;
        }

        /*
            weiter entfernte wände werden durch geringere lichtintensität dunkler gerendert
        */
        double light_intensity = 1;
        if (distance >= LIGHT_INTENSITY_MULTIPLIER) light_intensity = 1 / distance * LIGHT_INTENSITY_MULTIPLIER;

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

        for (int j = 0; j < sprite_array.count; j++)
        {
            if (sprite_array.sprites[j].is_visible)
            {
                draw_sprite_slice(pixels, i, j);
                sprite_array.sprites[j].is_visible = false;
            }
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

    // render_sprites(pixels);

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
    init_sprites(&cam.position);
    update_sprite_map();

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
            update_sprites();
            // update_sprite_map();
        }

        render(renderer, texture);
    }

    return 0;
}