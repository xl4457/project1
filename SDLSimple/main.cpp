/**
- Author: Amy Li
- Assignment: Simple 2D Scene
- Date due: 2024-09-28, 11:58pm
- I pledge that I have completed this assignment without
- collaborating with anyone else, in conformance with the
- NYU School of Engineering Policies and Procedures on
- Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1,
                LEVEL_OF_DETAIL    = 0,
                TEXTURE_BORDER     = 0;

constexpr char SUN_FILEPATH[]    = "sun.png",
               MOON_FILEPATH[]   = "moon.png",
               EARTH_FILEPATH[]  = "earth.png",
               STARS_FILEPATH[]  = "stars.png";

constexpr glm::vec3 INIT_SCALE       = glm::vec3(2.0f, 3.02f, 0.0f),
                    INIT_POS_SUN     = glm::vec3(-1.0f, 0.0f, 0.0f),
                    INIT_POS_MOON    = glm::vec3(-2.0f, 2.0f, 0.0f),
                    INIT_POS_EARTH   = glm::vec3(2.5f, 2.5f, 0.0f),
                    INIT_POS_STARS   = glm::vec3(3.0f, 2.50f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;
constexpr float RADIUS_MOON = 2.0f;
constexpr float RADIUS_EARTH = 3.0f;

constexpr float MOON_ROT_SPEED = 0.5f;
constexpr float EARTH_ROT_SPEED = 0.7f;

constexpr float G_GROWTH_FACTOR = 1.10f;
constexpr float G_SHRINK_FACTOR = 0.90f;
constexpr int   G_MAX_FRAME     = 40;

int g_frame_counter = 0;
bool g_is_growing = true;

float g_angle_moon = 0.0f;
float g_angle_earth = 0.0f;

float g_x_coord_moon = RADIUS_MOON,
      g_y_coord_moon = 0.0f;
float g_x_coord_earth = RADIUS_EARTH,
      g_y_coord_earth = 0.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_sun_matrix,
          g_moon_matrix,
          g_earth_matrix,
          g_stars_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_sun  = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_sun_texture_id,
       g_moon_texture_id,
       g_earth_texture_id,
       g_stars_texture_id;


GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Planet Orbits",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_sun_matrix       = glm::mat4(1.0f);
    g_moon_matrix      = glm::mat4(1.0f);
    g_earth_matrix     = glm::mat4(1.0f);
    g_stars_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    g_sun_texture_id   = load_texture(SUN_FILEPATH);
    g_moon_texture_id = load_texture(MOON_FILEPATH);
    g_earth_texture_id = load_texture(EARTH_FILEPATH);
    g_stars_texture_id = load_texture(STARS_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_rotation_sun.y += ROT_INCREMENT * delta_time;

    g_sun_matrix = glm::mat4(1.0f);
    g_sun_matrix = glm::translate(g_sun_matrix, INIT_POS_SUN);
    g_sun_matrix = glm::rotate(g_sun_matrix,
                                 g_rotation_sun.y,
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    
    g_moon_matrix = glm::mat4(1.0f);
    g_moon_matrix = glm::translate(g_moon_matrix, INIT_POS_MOON);
    
    g_angle_moon += MOON_ROT_SPEED * delta_time;
    g_x_coord_moon = RADIUS_MOON * glm::cos(g_angle_moon);
    g_y_coord_moon = RADIUS_MOON * glm::sin(g_angle_moon);
    g_moon_matrix = glm::translate(g_moon_matrix, glm::vec3(g_x_coord_moon + 1, g_y_coord_moon - 2, 0.0f));
    
    g_earth_matrix = glm::mat4(1.0f);
    g_earth_matrix = glm::translate(g_earth_matrix, INIT_POS_EARTH);
    
    g_angle_earth += EARTH_ROT_SPEED * delta_time;
    g_x_coord_earth = RADIUS_EARTH * glm::cos(g_angle_earth);
    g_y_coord_earth = RADIUS_EARTH * glm::sin(g_angle_earth);
    g_earth_matrix = glm::translate(g_earth_matrix, glm::vec3(g_x_coord_earth - 3.5, g_y_coord_earth - 2.5, 0.0f));
    
    g_stars_matrix = glm::mat4(1.0f);
    g_stars_matrix = glm::translate(g_stars_matrix, INIT_POS_STARS);
    g_stars_matrix = glm::scale(g_stars_matrix, INIT_SCALE);
    
    g_frame_counter++;
    glm::vec3 scale_vector;
    
    if (g_frame_counter >= G_MAX_FRAME)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }
    
    scale_vector = glm::vec3(g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
                             g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
                             1.0f);
    g_stars_matrix = glm::scale(g_stars_matrix, scale_vector);
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_sun_matrix, g_sun_texture_id);
    draw_object(g_moon_matrix, g_moon_texture_id);
    draw_object(g_earth_matrix, g_earth_texture_id);
    draw_object(g_stars_matrix, g_stars_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
