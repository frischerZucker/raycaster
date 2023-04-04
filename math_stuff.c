#define draw_start_y(distance) (Y_OFFSET - WINDOW_HEIGHT / distance)
#define draw_end_y(distance) (Y_OFFSET + WINDOW_HEIGHT / distance)

typedef struct Vector_
{
    double x, y;
} Vector;

typedef struct Camera_
{
    Vector position, direction, plane;
} Camera;

// gibt den von den beiden übergebenen vektoren eingeschlossenen winkel zurück
double angle_between_vectors(Vector *v1, Vector *v2)
{
    return acos(fabs(v1->x * v2->x + v1->y * v2->y) / (sqrt(v1->x * v1->x + v1->y * v1->y) * sqrt(v2->x * v2->x + v2->y * v2->y)));
}

double distance_between_points(Vector *p1, Vector *p2)
{
    double delta_x = fabs(p1->x - p2->x);
    double delta_y = fabs(p1->y - p2->y);

    return sqrt((delta_x * delta_x) + (delta_y * delta_y));
}