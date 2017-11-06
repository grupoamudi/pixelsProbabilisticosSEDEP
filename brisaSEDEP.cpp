#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <limits>
#include <fstream>

using namespace std;
#define RASP_MODE       1

#if RASP_MODE
    #define WIDTH           656
    #define HEIGHT          416 
    #define N_BUFFERS       100
    #define PIXELS_PER_RUN  200
    #define CHANGE_N        1000
    #define TRANSITION_N    100
#else
    #define WIDTH           1280
    #define HEIGHT          1024
    #define N_BUFFERS       100
    #define PIXELS_PER_RUN  1000
    #define CHANGE_N        4000
    #define TRANSITION_N    250

#endif

#define SIZE_PIXELS     (WIDTH*HEIGHT*4)
#define SMOOTH_TRANSITION   0

#define TIME_DEBUG          0

#define GLOBAL_SIGMA        0


#define READ_SIZE       6
#define N_PATTERNS      11

#define MULTIPLE_GEOMETRIES     1
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
    uint8_t active;
} pixel;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

/*defined this strcut this way, because it refers to itself*/
typedef struct pattern {
    double      *std;
    uint16_t    *color;
    /*If next_pattern is NULL the next one will be random*/
    pattern     *next_pattern;
    /*If not is_first that means that this pattern can't be the called from a random change*/
    uint8_t     is_first;
    uint16_t    duration;
    uint16_t    transition;
} pattern;

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
    cout << "\n";
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

void addGeometricForm(uint8_t *pixels, uint16_t width, uint16_t height, pixel *px, geometric_form *form)
{
    int32_t start_x = px->x - form->center_x;
    int32_t start_y = px->y - form->center_y;
    int32_t pos_x, pos_y, step_y, step_x;
    uint8_t color[4] = {px->b, px->g, px->r, SDL_ALPHA_OPAQUE};
    for(step_y = 0; step_y < form->height; step_y++)
    {
        pos_y = start_y + step_y;
        if ((pos_y >= 0) && (pos_y < height))
        {
            for(step_x = 0; step_x < form->width; step_x++) 
            {
                pos_x = start_x + step_x;
                if (form->pattern[step_y*form->width + step_x])
                {
                    if ((pos_x >= 0) && (pos_x < width))
                    {
                        memcpy(&pixels[(width*4*pos_y) + pos_x*4], color, 4);
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
    static const double epsilon = numeric_limits<double>::min();
    static double z1;
	static bool generate;
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

double getPeopleCount(double a, double b, uint16_t counter)
{
    double porc;
    porc = counter*a +b;
    if (porc > 100)
    {
        porc = 100;
    }
    else if (porc < 0)
    {
        porc = 0;
    }

    return porc/100;
}

uint16_t getPeopleCount()
{
    ifstream myfile;
    myfile.open("counter.bin", ios::binary);
    char *buff;
    uint16_t counter;
    buff = (char*)&counter;
    myfile.read(buff, 2);
    myfile.close();

    return counter;
}

double getPeopleCount(double a, double b)
{
    return getPeopleCount(a, b, getPeopleCount());
}


int get_file_size(string filename) // path to file
{
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}

void create_flag(uint16_t *color_list, uint8_t color_len, uint16_t *colors)
{
    uint16_t color_lines = HEIGHT/color_len;
    for(uint16_t y = 0; y < HEIGHT; y++)
    {
        for(uint16_t x = 0; x < WIDTH; x++)
        {
            uint8_t color_pos = y/color_lines;
            if (color_pos >= color_len)
            {
                color_pos = color_len - 1;
            }
            colors[(y*WIDTH + x)*3 + 0] = color_list[color_pos*3 + 0];
            colors[(y*WIDTH + x)*3 + 1] = color_list[color_pos*3 + 1];
            colors[(y*WIDTH + x)*3 + 2] = color_list[color_pos*3 + 2];
        }
    }
}

void create_rainbow(uint16_t *color, uint16_t offset)
{
    for (uint16_t y = 0; y < HEIGHT; y++)
    {
        for(uint16_t x = 0; x < WIDTH; x++)
        {
            color[(y*WIDTH + x)*3 + 0] = ((int)(360*(((double)x)/WIDTH)) + offset)%360; 
            color[(y*WIDTH + x)*3 + 1] = 100;
            color[(y*WIDTH + x)*3 + 2] = 100;
        }
    }

}

pattern createPattern(double *sigma, uint16_t *color, uint16_t duration, uint16_t transition)
{
    pattern pat;
    pat.std = sigma;
    pat.color = color;
    pat.duration = duration;
    pat.transition = transition;
    pat.next_pattern = NULL;
    pat.is_first = 1;
    return pat;
}

pattern createPattern(double *sigma, uint16_t *color)
{
    return createPattern(sigma, color, CHANGE_N, TRANSITION_N);
}

void load_std(string filename, double *sigmas)
{
    /*Read file to load a pattern
      this file is generatad by running the scripts createImage.py  parseImages.py*/
    ifstream myfile;
    myfile.open(filename, ios::out | ios::app | ios::binary);
    uint16_t nlines, ncols;
    char *size_buff = (char*)&ncols;
    myfile.read(size_buff, 2);
    size_buff = (char*)&nlines;
    myfile.read(size_buff, 2);

    for(uint32_t readcntr = 0; readcntr < HEIGHT*WIDTH; readcntr++)
    {
        sigmas[readcntr] = 5;
    }

    for(uint16_t nline = 0; nline < nlines; nline++)
    {
        for(uint16_t ncol = 0; ncol < ncols; ncol++)
        {
            char buff[6];
            myfile.read(buff, 6);

            uint32_t std;
            uint16_t hue;
            memcpy(&std, &buff[0], 4);
            if (std > 100000)
            {
                std = 100000;
            }
            memcpy(&hue, &buff[4], 2);
            uint8_t temp = ((uint8_t *)&hue)[0];
            ((uint8_t *)&hue)[0] = ((uint8_t *)&hue)[1];
            ((uint8_t *)&hue)[1] = temp;
            sigmas[nline*WIDTH + ncol] = std;
        }
    }
    myfile.close();
}

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

    uint16_t *color = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));


    uint16_t *color_rainbow_1 = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t *color_rainbow_2 = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t *color_rainbow_3 = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    create_rainbow(color_rainbow_1, 0);
    create_rainbow(color_rainbow_2, 50);
    create_rainbow(color_rainbow_3, 100);
    
    uint16_t *color_flag_lgbt = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t color_list_lgbt[] = {\
        359, 85, 74,\
          6, 83, 94,\
         51,100,100,\
        141, 78, 51,\
        226, 64, 64,\
        294, 69, 54
    };
    create_flag(color_list_lgbt, 6, color_flag_lgbt);

    uint16_t *color_flag_bi = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t color_list_bi[] = {\
        332, 89, 85,\
        332, 89, 85,\
        269, 45, 58,\
        224, 79, 61,\
        224, 79, 61\
    };
    create_flag(color_list_bi, 5, color_flag_bi);

    uint16_t *color_flag_trans = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t color_list_trans[] = {\
        197, 60, 97,\
        347, 33, 97,\
        180,  1,100,\
        347, 33, 97,\
        197, 60, 97\
    };
    create_flag(color_list_trans, 5, color_flag_trans);

    uint16_t *color_flag_assex = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t color_list_assex[] = {\
          0,  0,  0,\
          0,  0, 64,\
          0,  0,100,\
        301,100, 51\
    };
    create_flag(color_list_assex, 4, color_flag_assex);

    uint16_t *color_flag_lgbt_2 = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t)*3);
    uint16_t color_list_lgbt_2[] = {\
          0,  0,  0,\
         35, 82, 47,\
          0,100,100,\
         33,100,100,\
         54,100,100,\
        117, 94, 62,\
        218, 98, 69,\
        291, 79, 86,\
    };
    create_flag(color_list_lgbt_2, 7, color_flag_lgbt_2);



    uint16_t *color_amudi = (uint16_t*)malloc(WIDTH*HEIGHT*sizeof(uint16_t));
    double *full_sigmas[N_PATTERNS];
    double *sigmas = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    
    double *sigmas_amudi = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    load_std("aMuDi.res", sigmas_amudi);

    double *sigmas_amudimon = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    load_std("fullamudi.res", sigmas_amudimon);

    double *sigmas_sedep = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    load_std("SEDEP.res", sigmas_sedep);

    double *sigmas_flag = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_base = (double*)malloc(WIDTH*HEIGHT*sizeof(double));

    pattern patterns[N_PATTERNS];

    pattern white_noise = createPattern(sigmas_base, color_rainbow_1);

    pattern lgbt_flag   = createPattern(sigmas_flag, color_flag_lgbt);
    pattern lgbt_2_flag = createPattern(sigmas_flag, color_flag_lgbt_2);
    pattern bi_flag     = createPattern(sigmas_flag, color_flag_bi);
    pattern trans_flag  = createPattern(sigmas_flag, color_flag_trans);
    pattern assex_flag  = createPattern(sigmas_flag, color_flag_assex);

    pattern rainbow1_amudi = createPattern(sigmas_amudimon, color_rainbow_1, CHANGE_N/3, 0);
    pattern rainbow2_amudi = createPattern(sigmas_amudimon, color_rainbow_2, CHANGE_N/3, 0);
    pattern rainbow3_amudi = createPattern(sigmas_amudimon, color_rainbow_3, CHANGE_N/3, 0);

    rainbow1_amudi.next_pattern = &rainbow2_amudi;
    rainbow2_amudi.next_pattern = &rainbow3_amudi;
    rainbow2_amudi.is_first = 0;
    rainbow3_amudi.is_first = 0;

    pattern rainbow1_SEDEP = createPattern(sigmas_sedep, color_rainbow_1, CHANGE_N/3, 0);
    pattern rainbow2_SEDEP = createPattern(sigmas_sedep, color_rainbow_2, CHANGE_N/3, 0);
    pattern rainbow3_SEDEP = createPattern(sigmas_sedep, color_rainbow_3, CHANGE_N/3, 0);

    rainbow1_SEDEP.next_pattern = &rainbow2_SEDEP;
    rainbow2_SEDEP.next_pattern = &rainbow3_SEDEP;
    rainbow2_SEDEP.is_first = 0;
    rainbow3_SEDEP.is_first = 0;

    patterns[0]   = white_noise;
    patterns[1]   = trans_flag;
    patterns[2]   = lgbt_flag;
    patterns[3]   = bi_flag;
    patterns[3]   = assex_flag;
    patterns[4]   = rainbow1_amudi;
    patterns[5]   = rainbow2_amudi;
    patterns[6]   = rainbow3_amudi;
    patterns[7]   = rainbow1_SEDEP;
    patterns[8]   = rainbow2_SEDEP;
    patterns[9]   = rainbow3_SEDEP;
    patterns[10]  = lgbt_2_flag;

    /* Check All sigmas and colors */
    for (uint8_t i = 0; i < N_PATTERNS; i++)
    {
        if ((patterns[i].std == NULL) || (patterns[i].color == NULL))
        {
            cout << "Problems allocating patterns" << endl;
            return -1;
        }
    }



    /*Create some other patterns*/
    for (uint16_t y = 0; y < HEIGHT; y++)
    {
        for(uint16_t x = 0; x < WIDTH; x++)
        {
            color[y*WIDTH + x] = 180;
            sigmas_flag[y*WIDTH + x] = 5;//pow(2,10*(((double)x)/WIDTH));
            sigmas_base[y*WIDTH + x] = 10000;
        }
    }
    /*Initialize SDL things*/
    SDL_Init( SDL_INIT_EVERYTHING );
    TTF_Init();

    TTF_Font* Sans = TTF_OpenFont("DejaVuSansMono.ttf", 60); //this opens a font style and sets a size
    SDL_Color White = {255, 255, 255};  // this is the color in rgb format, maxing out all would give you the color white, and it will be your text's color
    SDL_Rect Message_rect; //create a rect
    Message_rect.x = 0;  //controls the rect's x coordinate 
    Message_rect.y = 0; // controls the rect's y coordinte
    Message_rect.w = 100; // controls the width of the rect
    Message_rect.h = 60; // controls the height of the rect

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
    if (pixels == NULL)
    {
        cout << "Problems allocationg pixel" << endl;
        return -1;
    }


    /*final_pixels are the actually 1920x1080 pixel description*/
    uint8_t *final_pixels = (uint8_t*)malloc(SIZE_PIXELS);
    if (final_pixels == NULL)
    {
        cout << "Problems allocationg final pixel" << endl;
        return -1;
    }

    memset(final_pixels, 0, SIZE_PIXELS);
    memset(pixels, 0, PIXELS_PER_RUN*sizeof(pixel)*N_BUFFERS);

  
    bool running = true;
    uint16_t cntr = 0;
    uint32_t full_cntr = 0;

    /*Load the initial pattern*/
    double sigma_effect = 1000;
    double count_A = 0.5, count_B = 0;
    uint8_t o_show_type = 0, fake_mode = 0, pause_mode = 0, shift_on = 0, white_noise_mode = 1;
    uint16_t fake_count = 20;
    pattern *pattern_ptr = &patterns[4];
    while( running )
    {


#if TIME_DEBUG
        const Uint64 start = SDL_GetPerformanceCounter();
        cout << "pos_start" << endl;
#endif
        /*Change to the next pattern*/
        if (((full_cntr)%pattern_ptr->duration) == 0)
        {
            sigma_effect = 1000;
            if (pattern_ptr->next_pattern == NULL)
            {
                if(!white_noise_mode%2 || rand()%4 == 0)
                {
                    do
                    {
                        uint16_t sigma_state = rand() % N_PATTERNS;
                        cout << "sigma_state: " << sigma_state << "\n";
                        pattern_ptr = &patterns[sigma_state];
                    }while(!pattern_ptr->is_first);
                } else {
                    pattern_ptr = &patterns[0];
                }
            }
            else
            {
                pattern_ptr = pattern_ptr->next_pattern;
            }
#if not SMOOTH_TRANSITION
            for(uint32_t sigma_cntr = 0; sigma_cntr < HEIGHT*WIDTH; sigma_cntr += 1)
            {
                sigmas[sigma_cntr] = pattern_ptr->std[sigma_cntr]; 
                color[sigma_cntr*3 + 0]  = pattern_ptr->color[sigma_cntr*3 + 0]; 
                color[sigma_cntr*3 + 1]  = pattern_ptr->color[sigma_cntr*3 + 1]; 
                color[sigma_cntr*3 + 2]  = pattern_ptr->color[sigma_cntr*3 + 2]; 
            }
#endif
        }
#if SMOOTH_TRANSITION
        /*This creates a soft pattern change, by applying the pattern slowly on top of the old one*/
        for(uint32_t sigma_cntr = 0; sigma_cntr < HEIGHT*WIDTH; sigma_cntr += 1)
        {
            sigmas[sigma_cntr] = sigmas[sigma_cntr]*0.9 + pattern_ptr->std[sigma_cntr]*0.1; 
            color[sigma_cntr*3 + 0]  = color[sigma_cntr*3 + 0]*0.95  + pattern_ptr->color[sigma_cntr*3 + 0]*0.05;
            color[sigma_cntr*3 + 1]  = color[sigma_cntr*3 + 1]*0.95  + pattern_ptr->color[sigma_cntr*3 + 1]*0.05;
            color[sigma_cntr*3 + 2]  = color[sigma_cntr*3 + 2]*0.95  + pattern_ptr->color[sigma_cntr*3 + 2]*0.05;
        }
#endif        

#if TIME_DEBUG
        const Uint64 pos1 = SDL_GetPerformanceCounter();
        cout << "pos1" << endl;
#endif

         /*Jump through the N_BUFFERS*/
        if (cntr >= N_BUFFERS)
        {
            cntr = 0;
        }       

        /*Place SDL background*/
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
        SDL_RenderClear( renderer );


#if TIME_DEBUG
        const Uint64 pos2 = SDL_GetPerformanceCounter();
        cout << "pos2" << endl;
#endif
        /*Poll for esc key*/
        while( SDL_PollEvent( &event ) )
        {
            if( ( SDL_QUIT == event.type ) ||
                ( SDL_KEYDOWN == event.type && SDL_SCANCODE_ESCAPE == event.key.keysym.scancode ) )
            {
                running = false;
                break;
            }
            else if (SDL_KEYDOWN == event.type && 225 == event.key.keysym.scancode )
            {
                shift_on = 1;
            }
            else if (SDL_KEYUP == event.type && 225 == event.key.keysym.scancode )
            {
                shift_on = 0;
            }
            else if (SDL_KEYDOWN == event.type && 4 == event.key.keysym.scancode )
            {
                if (shift_on)
                {
                    count_A += 0.01;
                }
                else
                {
                    count_A -= 0.01;
                    if (count_A < 0)
                    {
                        count_A = 0;
                    }
                }
            }
            else if (SDL_KEYDOWN == event.type && 5 == event.key.keysym.scancode )
            {
                if (shift_on)
                {
                    count_B += 0.25;
                }
                else
                {
                    count_B -= 0.25;
                    if (count_B < 0)
                    {
                        count_B = 0;
                    }
                }
            }
            else if (SDL_KEYDOWN == event.type && 18 == event.key.keysym.scancode )
            {
                o_show_type += 1;
            }
            else if (SDL_KEYDOWN == event.type && 9 == event.key.keysym.scancode )
            {
                fake_mode += 1;
            }
            else if (SDL_KEYDOWN == event.type && (87 == event.key.keysym.scancode || 46 == event.key.keysym.scancode))
            {
                fake_count += 1;
            }
            else if (SDL_KEYDOWN == event.type && (86 == event.key.keysym.scancode || 45 == event.key.keysym.scancode))
            {
                if (fake_count > 0)
                {
                    fake_count -= 1;
                }
            }
            else if (SDL_KEYDOWN == event.type && 19 == event.key.keysym.scancode )
            {
                pause_mode += 1;
            }
            else if (SDL_KEYDOWN == event.type && 22 == event.key.keysym.scancode )
            {
                white_noise_mode += 1;
            }
            cout << SDL_KEYDOWN << "," << event.type << "," << event.key.keysym.scancode << ", " << shift_on << "," << white_noise_mode << "\n";
        }

        if (pause_mode%2)
        {
            /*Clear the final buffer*/
            memset(final_pixels, 0, SIZE_PIXELS);

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
            continue;
        }


        /*Create the random pixels*/
        hsv hsv_val;
        rgb rgb_val;

        if (((full_cntr+1)%pattern_ptr->duration) < pattern_ptr->transition)
        {
            sigma_effect -= 999.0/pattern_ptr->transition;
        }
        else if (((full_cntr+1)%pattern_ptr->duration) > (pattern_ptr->duration - pattern_ptr->transition))
        {
            sigma_effect += 999.0/pattern_ptr->transition;
        }
        else
        {
            sigma_effect = 1;
        }
        if (sigma_effect < 1)
        {
            sigma_effect = 1;
        }
#if GLOBAL_SIGMA
        double sigma = pow(2,abs((double)full_cntr - 1000.0)/100);
#endif  
        uint16_t count_raw = 0;
        if(fake_mode%2)
        {
            count_raw = fake_count;
        }
        else 
        {
            count_raw = getPeopleCount();
        }
        uint16_t count = PIXELS_PER_RUN*getPeopleCount(count_A, count_B, count_raw);
        if (count > PIXELS_PER_RUN)
        {
            count = PIXELS_PER_RUN;
        }
        for( unsigned int i = 0; i < count; i++ )
        {
            /*Get a random position within the image*/
            const unsigned int x = rand() % WIDTH;
            const unsigned int y = rand() % HEIGHT;

            /*Get the sigma from table*/
#if not GLOBAL_SIGMA
            double sigma = sigmas[y*WIDTH + x]; 
#endif
            /*Get a random hue value and convert it to rgb*/

            sigma *= sqrt(sigma_effect);
            if (sigma > 1000)
            {
                sigma = 1000;
            }
            hsv_val.h = getRandom(color[(y*WIDTH + x)*3 + 0],sigma,0,360); 
            if (sigma > 100)
            {
                hsv_val.s = 1;
                hsv_val.v = 1;
            }
            else
            {
                hsv_val.s = ((double)color[(y*WIDTH + x)*3 + 1])/100.0;
                hsv_val.v = ((double)color[(y*WIDTH + x)*3 + 2])/100.0;
            }
            rgb_val = hsv2rgb(hsv_val);

            /*Store the value in the buffer*/
            pixels[PIXELS_PER_RUN*cntr + i].b = (int)(rgb_val.b*255);
            pixels[PIXELS_PER_RUN*cntr + i].g = (int)(rgb_val.g*255);       
            pixels[PIXELS_PER_RUN*cntr + i].r = (int)(rgb_val.r*255);
            pixels[PIXELS_PER_RUN*cntr + i].x = x;
            pixels[PIXELS_PER_RUN*cntr + i].y = y;
            pixels[PIXELS_PER_RUN*cntr + i].format = rand()%N_FORMS;
            pixels[PIXELS_PER_RUN*cntr + i].active = 1;
        }
        for (uint16_t i = count; i < PIXELS_PER_RUN; i++)
        {
            pixels[PIXELS_PER_RUN*cntr + i].active = 0;
        }

#if TIME_DEBUG
        const Uint64 pos3 = SDL_GetPerformanceCounter();
        cout << "pos3" << endl;
#endif

        /*Clear the final buffer*/
        memset(final_pixels, 0, SIZE_PIXELS);
        /*Fill final_pixels with the pixels described at each buffer*/
        for (uint16_t sub_cntr = 0; sub_cntr < N_BUFFERS; sub_cntr++)
        {
            for (uint16_t i = 0; i <  PIXELS_PER_RUN; i++)
            {
                pixel px = pixels[PIXELS_PER_RUN*((sub_cntr + cntr + 1)%N_BUFFERS) + i];
                if (px.active)
                    addGeometricForm(final_pixels, WIDTH, HEIGHT, &px, &forms[px.format]);
                else
                    break;

            }
        }

#if TIME_DEBUG
        const Uint64 pos4 = SDL_GetPerformanceCounter();
        cout << "pos4" << endl;
#endif

        /*Update and render the screen*/
        SDL_UpdateTexture
            (
            texture,
            NULL,
            &final_pixels[0],
            WIDTH * 4
            );
        SDL_RenderCopy( renderer, texture, NULL, NULL );


        
        if (o_show_type%3 != 2 && Sans != NULL)
        {
            string peopleCount_str = "";

            if (o_show_type%3 == 0)
            {
                peopleCount_str = to_string(count);
                Message_rect.w = 100; // controls the width of the rect
            }
            else if (o_show_type%3 == 1)
            {
                peopleCount_str = to_string(count) + "/" + to_string(count_raw);
                Message_rect.w = 200; // controls the width of the rect
            }
            SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, peopleCount_str.c_str(), White); // as TTF_RenderText_Solid could only be used on SDL_Surface then you have to create the surface first
            
            SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage); //now you can convert it into a texture
            
            if (Message != NULL && surfaceMessage != NULL)
            {
                SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
            }
            SDL_DestroyTexture(Message);
            SDL_FreeSurface(surfaceMessage);
        }

        SDL_RenderPresent( renderer );
#if TIME_DEBUG
        const Uint64 end = SDL_GetPerformanceCounter();
        const static Uint64 freq = SDL_GetPerformanceFrequency();
        const double seconds1 = ( pos1 - start ) / static_cast< double >( freq );
        const double seconds2 = ( pos2 - pos1 ) / static_cast< double >( freq );
        const double seconds3 = ( pos3 - pos2 ) / static_cast< double >( freq );
        const double seconds4 = ( pos4 - pos3 ) / static_cast< double >( freq );
        const double seconds5 = ( end - pos4 ) / static_cast< double >( freq );
        const double seconds = ( end - start ) / static_cast< double >( freq );
        cout << "\rFrame time: " << seconds1*1000.0 << "|" << seconds2*1000.0 << "|" << seconds3*1000.0 << "|" << seconds4*1000.0 << "|" << seconds5*1000.0 << "|" << seconds * 1000.0 << "ms           " << endl;
#endif
        cntr += 1;
        full_cntr += 1;
    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
}
