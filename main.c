/*
https://en.wikipedia.org/wiki/K-means_clustering
*/

#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define da_append(array, item)                                               \
  do                                                                         \
  {                                                                          \
    if ((array)->count >= (array)->capacity)                                 \
    {                                                                        \
      (array)->capacity =                                                    \
          (array)->capacity == 0 ? INITIAL_CAPACITY : (array)->capacity * 2; \
      (array)->items = realloc((array)->items,                               \
                               (array)->capacity * sizeof(*(array)->items)); \
      assert((array)->items != NULL && "Buy more RAM lol");                  \
    }                                                                        \
    (array)->items[(array)->count++] = (item);                               \
  } while (0)

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define INITIAL_CAPACITY 10
#define SAMPLE_RADIUS 5
#define SAMPLE_COLOR RED
#define CENTROID_RADIUS 10
#define CENTROID_COLOR BLACK

typedef struct
{
  float mean_x;
  float mean_y;
  int total;
} Mean;

typedef struct
{
  float x;
  float y;
  int cluster;
} Sample;

typedef struct
{
  Sample *items;
  int count;
  int capacity;
} Samples;

typedef struct
{
  Vector2 *items;
  int count;
  int capacity;
} Centroids;

Color centroids_colors[] = {
    RED,
    GREEN,
    YELLOW
};

//--------------------------------------------------
// Helper function to generate random float between min and max
//--------------------------------------------------
float get_random_float(float min, float max)
{
  return (float)rand() / (float)(RAND_MAX / (max - min)) + min;
}

//--------------------------------------------------
// Helper function for generating random samples
//--------------------------------------------------
void generate_samples(Samples *s, Vector2 center, size_t num_samples, double radius)
{
  Sample next_sample = {0};
  for (size_t i = 0; i < num_samples; i++)
  {
    next_sample.x = center.x + get_random_float(-radius, radius);
    next_sample.y = center.y + get_random_float(-radius, radius);
    next_sample.cluster = -1; // We don't know the centroid to which it belongs yet
    da_append(s, next_sample);
  }
}

//--------------------------------------------------
// Draw samples on window
//--------------------------------------------------
void draw_samples(Samples *s)
{
  for (int i = 0; i < s->count; i++)
  {
    Sample sample = s->items[i];
    Color color;
    if (sample.cluster == -1)
      color = PINK;
    else
      color = centroids_colors[sample.cluster];

    DrawCircle(sample.x, sample.y, SAMPLE_RADIUS, color);
  }
}

//--------------------------------------------------
// Randomly initialize centroids centers
//--------------------------------------------------
void create_centroids(Centroids *c, int k)
{
  float x_pad = WINDOW_WIDTH / 10;
  float y_pad = WINDOW_HEIGHT / 10;
  float x_separation = WINDOW_WIDTH / k;
  float y_separation = WINDOW_HEIGHT / k;
  Vector2 position = {0};
  for (int i = 0; i < k; i++)
  {
    position.x = get_random_float(x_separation * i, x_separation * (i + 1));
    position.y = get_random_float(y_separation * i, y_separation * (i + 1));
    da_append(c, position);
  }
}

//--------------------------------------------------
// Draw centroids on window
//--------------------------------------------------
void draw_centroids(Centroids *c)
{
  for (size_t k = 0; k < c->count; k++)
  {
    Vector2 centroid = c->items[k];
    DrawCircle(centroid.x, centroid.y, CENTROID_RADIUS, centroids_colors[k]);
  }
}

//--------------------------------------------------
// Assigns each sample to the closest centroid
//--------------------------------------------------
void assign_step(Centroids *c, Samples *s)
{
  float curr_distance;
  for (int i = 0; i < s->count; i++)
  {
    Sample *sample = &s->items[i];
    float best_distance = __FLT_MAX__;
    for (int k = 0; k < c->count; k++)
    {
      Vector2 centroid = c->items[k];
      // Compute distance from point to centroid
      curr_distance = sqrtf((sample->x - centroid.x) * (sample->x - centroid.x) + (sample->y - centroid.y) * (sample->y - centroid.y));

      // Assign the minimun distance to the sample
      if (curr_distance < best_distance)
      {
        best_distance = curr_distance;
        sample->cluster = k;
      }
    }
  }
}

//--------------------------------------------------
// Updates centroids center based on its samples
//--------------------------------------------------
void update_step(Centroids *c, Samples *s)
{
  Mean *mean_array = malloc(c->count * sizeof(Mean));
  if (mean_array == NULL)
  {
    fprintf(stderr, "ERORR: Could not allocate memory for mean_array on update_step method\n");
    return;
  }

  for (int k = 0; k < c->count; k++)
  {
    Mean m = {0};
    mean_array[k] = m;
  }

  for (int i = 0; i < s->count; i++)
  {
    Sample sample = s->items[i];
    mean_array[sample.cluster].mean_x += sample.x;
    mean_array[sample.cluster].mean_y += sample.y;
    mean_array[sample.cluster].total += 1;
  }

  for (int k = 0; k < c->count; k++)
  {
    mean_array[k].mean_x /= mean_array[k].total;
    mean_array[k].mean_y /= mean_array[k].total;
    c->items[k].x = mean_array[k].mean_x;
    c->items[k].y = mean_array[k].mean_y;
  }

  free(mean_array);
}

//--------------------------------------------------
// Kmeans converge when centroids have not change
//--------------------------------------------------
bool converged(Centroids *previus, Centroids *centroids)
{
  if (previus->count != centroids->count)
    return false;

  const float EPSILON = 0.0001f;
  for (int i = 0; i < centroids->count; i++)
  {
    float dx = previus->items[i].x - centroids->items[i].x;
    float dy = previus->items[i].y - centroids->items[i].y;
    if (dx * dx + dy * dy > EPSILON)
      return false;
  }
  return true;
}

//--------------------------------------------------
// Run Kmeans
//--------------------------------------------------
void run_kmeans(Centroids *centroids, Samples *samples, float *time_between_updates)
{
  if (*time_between_updates < 1.0f)
    return;

  Centroids previous;
  previous.items = malloc(sizeof(Vector2) * centroids->capacity);
  previous.capacity = centroids->capacity;

  while (!converged(&previous, centroids))
  {
    previous.count = centroids->count;
    memcpy(previous.items, centroids->items,
           sizeof(Vector2) * centroids->count);

    assign_step(centroids, samples);
    update_step(centroids, samples);
  }

  *time_between_updates = 0.0f;
  free(previous.items);
}

//--------------------------------------------------
// Kmeans algorithm:
// 1. Create k initial centroids randomly
// 2. Create k clusters by associating each point with the nearest mean
// 3. Update the centroids
// 4. Repeat steps 2 and 3 until convergence
//--------------------------------------------------
int main()
{
  InitWindow(800, 600, "Kmeans");
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  size_t num_samples = 25;
  Vector2 center = (Vector2){
      .x = WINDOW_WIDTH / 2,
      .y = WINDOW_HEIGHT / 2};
  double radius = 50.0;
  Samples samples = {0};

  generate_samples(&samples, center, num_samples, radius);

  center.y += center.y / 2;
  generate_samples(&samples, center, num_samples, radius);

  center.x += center.x / 2;
  generate_samples(&samples, center, num_samples, radius);

  center.x -= center.x * 0.7;
  generate_samples(&samples, center, num_samples, radius);

  Centroids centroids = {0};
  create_centroids(&centroids, 3);

  float dt;
  float time_between_updates = 0.0f;

  Centroids previus;
  while (!WindowShouldClose())
  {
    dt = GetFrameTime();
    time_between_updates += dt;

    BeginDrawing();
    ClearBackground(RAYWHITE);
    draw_centroids(&centroids);
    draw_samples(&samples);
    run_kmeans(&centroids, &samples, &time_between_updates);
    EndDrawing();
  }

  CloseWindow();

  return 0;
}