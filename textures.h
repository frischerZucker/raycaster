#define TEXTURE_HEIGHT 64
#define TEXTURE_WIDTH 64
#define NUMBER_OF_TEXTURES 2

SDL_Color textures[NUMBER_OF_TEXTURES][TEXTURE_WIDTH * TEXTURE_HEIGHT];

/*
    lädt farbwerte des bilds in das array für die textur 
*/
void load_texture(SDL_Color *texture, char *img_path)
{
    SDL_Surface *surface = IMG_Load(img_path);

    const uint8_t bytes_per_pixel = surface->format->BytesPerPixel;

    uint8_t *pixels = surface->pixels;

    for (int i = 0; i < TEXTURE_HEIGHT; i++)
    {
        for (int j = 0; j < TEXTURE_WIDTH; j++)
        {
            uint32_t pixel_data = *(uint32_t*) (pixels + i * surface->pitch + j * bytes_per_pixel);

            SDL_Color color = {0, 0, 0, SDL_ALPHA_OPAQUE};

            SDL_GetRGB(pixel_data, surface->format, &color.r, &color.g, &color.b);

            texture[i * TEXTURE_WIDTH + j].r = color.r;
            texture[i * TEXTURE_WIDTH + j].g = color.g;
            texture[i * TEXTURE_WIDTH + j].b = color.b;
        }
    }

    SDL_FreeSurface(surface);
}

void init_textures()
{
    load_texture(textures[0], "wood.png");
    load_texture(textures[1], "bricks.png");
}