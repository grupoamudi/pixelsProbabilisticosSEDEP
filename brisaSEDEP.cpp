#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <iostream>
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <limits>
#include <fstream>

using namespace std;

#define WIDTH           1920
#define HEIGHT          1080 
#define N_BUFFERS       100
#define SIZE_PIXELS     (WIDTH*HEIGHT*4)
#define PIXELS_PER_RUN  250

#define GLOBAL_SIGMA    0

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t format;
    uint16_t x;
    uint16_t y;
} pixel;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t center_x;
    uint8_t center_y;
    uint8_t *pattern;
} geometric_form;

static rgb   hsv2rgb(hsv in);
double getRandom(double mu, double sigma, double min, double max);

void testForm(geometric_form *form)
{
    for(uint8_t y = 0; y < form->height; y++)
    {
        for(uint8_t x = 0; x < form->width; x++)
        {
            cout << form->pattern[y*form->width + x]/0xFF << ",";
        }
        cout << "\n";
    }
}

void createTriangle(uint8_t radius, geometric_form *form)
{
    /* First allocate space to create the pattern */
    form->width = 2*radius + 1;
    form->height = 2*radius + 1;
    form->center_x = radius;
    form->center_y = radius;
    form->pattern = (uint8_t *)malloc(form->width*form->height);
    memset(form->pattern, 0, (2*radius + 1)*(2*radius + 1));
    uint8_t x = 0;
    for(uint8_t y = (form->height-1); y >= radius; y--)
    {
        memset(&form->pattern[y*form->width + x], 0xFF, y-x+1);
        x++;
    }
}

void createSquare(uint8_t radius, geometric_form *form)
{
    /* First allocate space to create the pattern */
    form->width = 2*radius + 1;
    form->height = 2*radius + 1;
    form->center_x = radius;
    form->center_y = radius;
    form->pattern = (uint8_t *)malloc(form->width*form->height);
    memset(form->pattern, 0xFF, (2*radius + 1)*(2*radius + 1));
}

void createCircle(uint8_t radius, geometric_form *form)
{
    /* First allocate space to create the pattern */
    form->width = 2*radius + 1;
    form->height = 2*radius + 1;
    form->center_x = radius;
    form->center_y = radius;
    form->pattern = (uint8_t *)malloc(form->width*form->height);
    memset(form->pattern, 0, (2*radius + 1)*(2*radius + 1));
    
    /* Now calculate the form */
    for(uint8_t y = 0; y < form->height; y++)
    {
        for(uint8_t x = 0; x < form->width; x++)
        {
            /* center of this form is (radius, radius) */
            /* Calculate the distance from the radius    */
            float dist = pow(((float)x) - ((float)radius), 2);
            dist += pow(((float)y) - ((float)radius), 2);
            dist = sqrt(dist);
            if(dist <= ((float)radius))
            {
                form->pattern[y*form->width + x] = 0xFF;
            }
        }
    }
}

void addGeometricForm(uint8_t *pixels, uint16_t width, uint16_t height, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, geometric_form *form)
{
    int32_t start_x = x - ((int)(form->width/2));
    int32_t start_y = y - ((int)(form->height/2));
    int32_t pos_x, pos_y;
    for(uint8_t step_y = 0; step_y < form->height; step_y++)
    {
        pos_y = start_y + step_y;
        if ((pos_y >= 0) && (pos_y < height))
        {
            for(uint8_t step_x = 0; step_x < form->width; step_x++) 
            {
                pos_x = start_x + step_x;
                if (form->pattern[step_y*form->width + step_x])
                {
                    if ((pos_x >= 0) && (pos_x < width))
                    {
                        pixels[(width*4*pos_y) + pos_x*4 + 0] = b;
                        pixels[(width*4*pos_y) + pos_x*4 + 1] = g;
                        pixels[(width*4*pos_y) + pos_x*4 + 2] = r;
                        pixels[(width*4*pos_y) + pos_x*4 + 3] = SDL_ALPHA_OPAQUE;
                    }
                }
            }
        }
    }
}

/*This function converts a color described in the HSV format to RGB format*/
rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

/*This function return a constrained random value with normal distribution*/
/*It uses box-muller to convert from the constrained uniform distribution of the rand() function*/
double getRandom(double mu, double sigma, double min, double max)
{
    static const double epsilon = std::numeric_limits<double>::min();
    thread_local double z1;
	thread_local bool generate;
	generate = !generate;
    z1 = z1 * sigma + mu;
	if ((!generate)) 
    {
        if ((z1 <= max) && (z1 >= min))
        {
	        return z1;
        } else {
	        generate = !generate;
        }
    }
    double z0;
    double u1, u2;
    do
    {
	    do
	    {
	      u1 = rand() * (1.0 / RAND_MAX);
	      u2 = rand() * (1.0 / RAND_MAX);
	    }
	    while ( u1 <= epsilon );
        z0 = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
        z1 = sqrt(-2.0 * log(u1)) * sin(2 * M_PI * u2);
        z0 = z0 * sigma + mu;
    }
    while ( (z0 > max) || (z0 < min));
    return z0;
}

#define READ_SIZE       6
#define N_PATTERNS      15

#define MULTIPLE_GEOMETRIES     0
#define MULTIPLE_SIZES          0









#define N_GEOMETRIES    3
#define N_SIZES         5

#if (MULTIPLE_GEOMETRIES*MULTIPLE_SIZES)
#define N_FORMS         (N_GEOMETRIES*N_SIZES)
#elif MULTIPLE_GEOMETRIES
#define N_FORMS         N_GEOMETRIES
#elif MULTIPLE_SIZES
#define N_FORMS         N_SIZES
#else
#define N_FORMS         1
#endif


int main( int argc, char** argv )
{
    srand(time(NULL));

    geometric_form forms[N_FORMS];
#if (MULTIPLE_GEOMETRIES*MULTIPLE_SIZES)
    createTriangle(1,  &forms[0]);
    createTriangle(3,  &forms[1]);
    createTriangle(5,  &forms[2]);
    createTriangle(7,  &forms[3]);
    createTriangle(10, &forms[4]);
    createSquare(1,  &forms[5]);
    createSquare(3,  &forms[6]);
    createSquare(5,  &forms[7]);
    createSquare(7,  &forms[8]);
    createSquare(10, &forms[9]);
    createCircle(1,  &forms[10]);
    createCircle(3,  &forms[11]);
    createCircle(5,  &forms[12]);
    createCircle(7,  &forms[13]);
    createCircle(10, &forms[14]);
#elif MULTIPLE_GEOMETRIES
    createTriangle(3,  &forms[0]);
    createSquare(3,  &forms[1]);
    createCircle(3,  &forms[2]);
#elif MULTIPLE_SIZES
    createCircle(1,  &forms[0]);
    createCircle(3,  &forms[1]);
    createCircle(5,  &forms[2]);
    createCircle(7,  &forms[3]);
    createCircle(10, &forms[4]);
#else
    createCircle(3,  &forms[0]);
#endif

    for(uint8_t i = 0; i < N_FORMS; i++)
    {
        testForm(&forms[i]);
    }

    /* Allocate all the pattern buffers and place them in the desired order */
    /* color buffers are for the hue value of HSV (ranging from 0 to 360)*/
    /* sigma buffers are for the standard deviation, they are double */
    uint16_t *full_color[N_PATTERNS];
    uint16_t *color = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *color_flag_1 = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *color_flag_2 = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *color_flag_3 = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *color_amudi = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    double *full_sigmas[N_PATTERNS];
    double *sigmas = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_amudi = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_flag = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_base = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    full_color[0] = color_flag_1;
    full_color[1] = color_flag_2;
    full_color[2] = color_flag_3;
    full_color[3] = color_flag_2;
    full_color[4] = color_flag_1;
    full_color[5] = color_flag_2;
    full_color[6] = color_flag_3;
    full_color[7] = color_flag_2;
    full_color[8] = color_flag_1;
    full_color[9] = color_flag_2;
    full_color[10] = color_flag_3;
    full_color[11] = color_flag_2;
    full_color[12] = color_flag_1;
    full_color[13] = color_flag_2;
    full_color[14] = color_flag_3;
    full_sigmas[0] = sigmas_base;
    full_sigmas[1] = sigmas_flag;
    full_sigmas[2] = sigmas_flag;
    full_sigmas[3] = sigmas_flag;
    full_sigmas[4] = sigmas_flag;
    full_sigmas[5] = sigmas_flag;
    full_sigmas[6] = sigmas_amudi;
    full_sigmas[7] = sigmas_base;
    full_sigmas[8] = sigmas_flag;
    full_sigmas[9] = sigmas_amudi;
    full_sigmas[10] = sigmas_flag;
    full_sigmas[11] = sigmas_flag;
    full_sigmas[12] = sigmas_base;
    full_sigmas[13] = sigmas_amudi;
    full_sigmas[14] = sigmas_flag;

    /*Read file to load a pattern
      this file is generatad by running the scripts createImage.py  parseImages.py*/
    ifstream myfile;
    myfile.open("test.res", ios::out | ios::app | ios::binary);
    for(uint32_t readcntr = 0; readcntr < WIDTH*HEIGHT; readcntr++)
    {
        char buff[READ_SIZE];
        myfile.read(buff, READ_SIZE);
        for(uint16_t i = 0; i < READ_SIZE; i+=6)
        {
            uint32_t std;
            uint16_t hue;
            memcpy(&std, &buff[i], 4);
            if (std > 100000)
            {
                std = 100000;
            }
            memcpy(&hue, &buff[i+4], 2);
            uint8_t temp = ((uint8_t *)&hue)[0];
            ((uint8_t *)&hue)[0] = ((uint8_t *)&hue)[1];
            ((uint8_t *)&hue)[1] = temp;
            color[3*readcntr + 0] = hue;
            color[3*readcntr + 1] = 1;
            color[3*readcntr + 2] = 1;
            sigmas_amudi[readcntr] = std;

        }
    }
    myfile.close();
    /*Create some other patterns*/
    for (uint16_t y = 0; y < HEIGHT; y++)
    {
        for(uint16_t x = 0; x < WIDTH; x++)
        {
            color_flag_1[3*(y*WIDTH + x) + 0] = 360*(((double)x)/WIDTH); 
            color_flag_2[3*(y*WIDTH + x) + 0] = ((int)(360*(((double)x)/WIDTH)) + 50)%360; 
            color_flag_3[3*(y*WIDTH + x) + 0] = ((int)(360*(((double)x)/WIDTH)) + 100)%360; 
            sigmas_flag[y*WIDTH + x] = 5;//pow(2,10*(((double)x)/WIDTH));
            sigmas_base[y*WIDTH + x] = 10000;
        }
    }

    /*Initialize SDL things*/
    SDL_Init( SDL_INIT_EVERYTHING );
    atexit( SDL_Quit );

    SDL_Window* window = SDL_CreateWindow
        (
        "SDL2",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN
        );

    SDL_Renderer* renderer = SDL_CreateRenderer
        (
        window,
        -1,
        SDL_RENDERER_ACCELERATED
        );

    SDL_RendererInfo info;
    SDL_GetRendererInfo( renderer, &info );
    SDL_Texture* texture = SDL_CreateTexture
        (
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT
        );
    SDL_Event event;


    /*pixels will hold all the N_BUFFERS of pixel to draw*/
    pixel *pixels = (pixel*)malloc(PIXELS_PER_RUN*sizeof(pixel)*N_BUFFERS);
    memset(pixels, 0, PIXELS_PER_RUN*sizeof(pixel)*N_BUFFERS);

    /*final_pixels are the actually 1920x1080 pixel description*/
    uint8_t *final_pixels = (uint8_t*)malloc(SIZE_PIXELS);
    memset(final_pixels, 0, SIZE_PIXELS);

  
    bool running = true;
    uint16_t cntr = 0;
    uint32_t full_cntr = 0;
    uint16_t sigma_state = 0;

    /*Load the initial pattern*/
    for(uint32_t sigma_cntr = 0; sigma_cntr < HEIGHT*WIDTH; sigma_cntr += 1)
    {
        sigmas[sigma_cntr] = full_sigmas[sigma_state][sigma_cntr]; 
        color[sigma_cntr*3 + 0] = full_color[sigma_state][sigma_cntr*3 + 0]; 
    }

    while( running )
    {


        /*Change to the next pattern*/
        if (((full_cntr+1)%150) == 0)
        {
            sigma_state += 1;
            if(sigma_state == N_PATTERNS)
            {
                sigma_state = 0;
            }
        }
        /*This creates a soft pattern change, by applying the pattern slowly on top of the old one*/
        for(uint32_t sigma_cntr = 0; sigma_cntr < HEIGHT*WIDTH; sigma_cntr += 1)
        {
            sigmas[sigma_cntr] = sigmas[sigma_cntr]*0.9 + full_sigmas[sigma_state][sigma_cntr]*0.1; 
            color[sigma_cntr*3 + 0] = color[sigma_cntr*3 + 0]*0.9 + full_color[sigma_state][sigma_cntr*3 + 0]*0.1;
        }

         /*Jump through the N_BUFFERS*/
        if (cntr >= N_BUFFERS)
        {
            cntr = 0;
        }       
        /*Clear the buffer for this run*/
        memset(&pixels[cntr*PIXELS_PER_RUN*sizeof(pixel)], 0, PIXELS_PER_RUN*sizeof(pixels));

        /*Place SDL background*/
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
        SDL_RenderClear( renderer );

        /*Poll for esc key*/
        while( SDL_PollEvent( &event ) )
        {
            if( ( SDL_QUIT == event.type ) ||
                ( SDL_KEYDOWN == event.type && SDL_SCANCODE_ESCAPE == event.key.keysym.scancode ) )
            {
                running = false;
                break;
            }
        }

        /*Create the random pixels*/
        hsv hsv_val;
        rgb rgb_val;
        hsv_val.s = 1.0;
        hsv_val.v = 1.0;
        cout << sigma_state << "; " << full_cntr << "                                     \r";
#if GLOBAL_SIGMA
        double sigma = pow(2,abs((double)full_cntr - 1000.0)/100);
#endif  
        for( unsigned int i = 0; i < PIXELS_PER_RUN; i++ )
        {
            /*Get a random position within the image*/
            const unsigned int x = rand() % WIDTH;
            const unsigned int y = rand() % HEIGHT;

            /*Get the sigma from table*/
#if not GLOBAL_SIGMA
            double sigma = sigmas[y*WIDTH + x]; 
#endif
            /*Get a random hue value and convert it to rgb*/
            hsv_val.h = getRandom(color[3*(y*WIDTH + x) + 0],sigma,0,360); 
            rgb_val = hsv2rgb(hsv_val);

            /*Store the value in the buuffer*/
            pixels[PIXELS_PER_RUN*cntr + i].b = (int)(rgb_val.b*255);
            pixels[PIXELS_PER_RUN*cntr + i].g = (int)(rgb_val.g*255);       
            pixels[PIXELS_PER_RUN*cntr + i].r = (int)(rgb_val.r*255);
            pixels[PIXELS_PER_RUN*cntr + i].x = x;
            pixels[PIXELS_PER_RUN*cntr + i].y = y;
            pixels[PIXELS_PER_RUN*cntr + i].format = rand()%N_FORMS;
        }

        /*Clear the final buffer*/
        memset(final_pixels, 0, SIZE_PIXELS);
        /*Fill final_pixels with the pixels described at each buffer*/
        for (uint16_t sub_cntr = 0; sub_cntr < N_BUFFERS; sub_cntr++)
        {
            for (uint16_t i = 0; i <  PIXELS_PER_RUN; i++)
            {
                const uint16_t x = pixels[PIXELS_PER_RUN*sub_cntr + i].x;
                const uint16_t y = pixels[PIXELS_PER_RUN*sub_cntr + i].y;
                if ((x < WIDTH) && (y < HEIGHT))
                {
                    //final_pixels[(WIDTH*4*y) + x*4 + 0] = pixels[PIXELS_PER_RUN*sub_cntr + i].b;
                    //final_pixels[(WIDTH*4*y) + x*4 + 1] = pixels[PIXELS_PER_RUN*sub_cntr + i].g;
                    //final_pixels[(WIDTH*4*y) + x*4 + 2] = pixels[PIXELS_PER_RUN*sub_cntr + i].r;
                    //final_pixels[(WIDTH*4*y) + x*4 + 3] = SDL_ALPHA_OPAQUE;
                    addGeometricForm(final_pixels, WIDTH, HEIGHT, x, y, pixels[PIXELS_PER_RUN*sub_cntr + i].r, pixels[PIXELS_PER_RUN*sub_cntr + i].g, pixels[PIXELS_PER_RUN*sub_cntr + i].b, &forms[pixels[PIXELS_PER_RUN*sub_cntr + i].format]);
                }
            }
        }

        /*Update and render the screen*/
        SDL_UpdateTexture
            (
            texture,
            NULL,
            &final_pixels[0],
            WIDTH * 4
            );
        SDL_RenderCopy( renderer, texture, NULL, NULL );
        SDL_RenderPresent( renderer );

        cntr += 1;
        full_cntr += 1;
    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
}
