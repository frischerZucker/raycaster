#define SPRITE_WIDTH .5
#define SPRITE_PIXEL_WIDTH (2 * SPRITE_WIDTH) / TEXTURE_WIDTH

#define SPRITE_ID_PURPLE_BLOB 2
#define SPRITE_ID_PLANT 3

typedef struct Sprite_
{
    Vector position, point_of_last_colision_with_ray;
    double distance_to_player, light_intensity, sprite_section_height, end_of_current_sprite_section, draw_start, draw_end;
    SDL_Color *texture;
    bool is_visible, is_behind_camera;
} Sprite;

struct sprite_arr_
{
    Sprite *sprites;
    int count;
} sprite_array;

void update_sprite(Sprite *sprite, Camera *cam)
{
    sprite->distance_to_player = distance_between_points(&sprite->position, &cam->position);

    //  über prüft ob sich das sprite vor oder hinter der kamera befindet
    Vector b;
    b.x = cam->position.x + cam->direction.x * RAY_STEP_LENGTH;
    b.y = cam->position.y + cam->direction.y * RAY_STEP_LENGTH;
    if (distance_between_points(&sprite->position, &cam->position) <= distance_between_points(&sprite->position, &b))
    {
        sprite->is_behind_camera = true;
        return;
    }
    else sprite->is_behind_camera = false;

    sprite->light_intensity = 1;
    if (sprite->distance_to_player >= LIGHT_INTENSITY_MULTIPLIER) sprite->light_intensity = 1 / sprite->distance_to_player * LIGHT_INTENSITY_MULTIPLIER;

    sprite->draw_start = draw_start_y(sprite->distance_to_player);
    sprite->draw_end = draw_end_y(sprite->distance_to_player);

    sprite->sprite_section_height = (sprite->draw_end - sprite->draw_start) / TEXTURE_HEIGHT;

    sprite->end_of_current_sprite_section = sprite->draw_start + sprite->sprite_section_height;
}

void add_sprite(int texture_id, double x_pos, double y_pos)
{
    sprite_array.count++;
    sprite_array.sprites = (sprite_array.sprites == NULL) ? malloc(sizeof(Sprite)) : realloc(sprite_array.sprites, sizeof(Sprite) * sprite_array.count);

    sprite_array.sprites[sprite_array.count - 1].position.x = x_pos;
    sprite_array.sprites[sprite_array.count - 1].position.y = y_pos;
    sprite_array.sprites[sprite_array.count - 1].texture = &texture_pointer[texture_pointer_offset(texture_id)];
    sprite_array.sprites[sprite_array.count - 1].is_visible = false;
    sprite_array.sprites[sprite_array.count - 1].is_behind_camera = false;
    sprite_array.sprites[sprite_array.count - 1].point_of_last_colision_with_ray.x = 0;
    sprite_array.sprites[sprite_array.count - 1].point_of_last_colision_with_ray.y = 0;
}

void init_sprites()
{
    sprite_array.count = 0;

    add_sprite(SPRITE_ID_PURPLE_BLOB, 3.5, 3.5);
    add_sprite(SPRITE_ID_PLANT, 1.25, 5.2);
}