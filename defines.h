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
#ifndef M_PI_4
#define M_PI_4 M_PI / 4
#endif

#define MAP_HEIGHT 10
#define MAP_WIDTH 10

#define TICKS_PER_SECOND 30
#define MOVEMENT_SPEED .04
#define ROTATION_SPEED M_PI / 200
#define FOV 40
#define LIGHT_INTENSITY_MULTIPLIER 5