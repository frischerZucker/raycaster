#define TEXTURE_HEIGHT 64
#define TEXTURE_WIDTH 64
#define SIZEOF_TEXTURE sizeof(struct SDL_Color) * TEXTURE_WIDTH * TEXTURE_HEIGHT

#define texture_pointer_offset(index) (index) * TEXTURE_HEIGHT * TEXTURE_WIDTH 

SDL_Color *texture_pointer = NULL;

/*
    lädt farbwerte des bilds in das array für die textur 
*/
void load_texture(char *img_path)
{
    static int number_of_textures = 0;

    number_of_textures++;
    texture_pointer = (texture_pointer == NULL) ? malloc(SIZEOF_TEXTURE) : realloc(texture_pointer, SIZEOF_TEXTURE * number_of_textures);

    SDL_Surface *surface = IMG_Load(img_path);

    const uint8_t bytes_per_pixel = surface->format->BytesPerPixel;

    uint8_t *pixels = surface->pixels;

    for (int y = 0; y < TEXTURE_HEIGHT; y++)
    {
        for (int x = 0; x < TEXTURE_WIDTH; x++)
        {
            uint32_t pixel_data = *(uint32_t*) (pixels + y * surface->pitch + x * bytes_per_pixel);

            SDL_Color color = {0, 0, 0, SDL_ALPHA_OPAQUE};

            SDL_GetRGBA(pixel_data, surface->format, &color.r, &color.g, &color.b, &color.a);

            int position_on_texture = texture_pointer_offset(number_of_textures - 1) + y * TEXTURE_WIDTH + x;

            texture_pointer[position_on_texture].r = color.r;
            texture_pointer[position_on_texture].g = color.g;
            texture_pointer[position_on_texture].b = color.b;
            texture_pointer[position_on_texture].a = color.a;
        }
    }

    SDL_FreeSurface(surface);
}

void init_textures()
{
    load_texture("wood.png");
    load_texture("bricks.png");
    load_texture("purple_blob.png");
    load_texture("plant.png");
}