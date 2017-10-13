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
#define PIXELS_PER_RUN  1000

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
    uint16_t x;
    uint16_t y;
} pixel;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static rgb   hsv2rgb(hsv in);

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

#define READ_SIZE   6
#define N_PATTERNS  15
int main( int argc, char** argv )
{
    srand(time(NULL));

    ifstream myfile;
    myfile.open("test.res", ios::out | ios::app | ios::binary);
    uint16_t *full_mus[N_PATTERNS];
    uint16_t *mus = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *mus_flag_1 = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *mus_flag_2 = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *mus_flag_3 = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    uint16_t *mus_amudi = (uint16_t*)malloc(WIDTH*HEIGHT*3*sizeof(uint16_t));
    double *full_sigmas[N_PATTERNS];
    double *sigmas = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_amudi = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_flag = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    double *sigmas_base = (double*)malloc(WIDTH*HEIGHT*sizeof(double));
    full_mus[0] = mus_flag_1;
    full_mus[1] = mus_flag_2;
    full_mus[2] = mus_flag_3;
    full_mus[3] = mus_flag_2;
    full_mus[4] = mus_flag_1;
    full_mus[5] = mus_flag_2;
    full_mus[6] = mus_flag_3;
    full_mus[7] = mus_flag_2;
    full_mus[8] = mus_flag_1;
    full_mus[9] = mus_flag_2;
    full_mus[10] = mus_flag_3;
    full_mus[11] = mus_flag_2;
    full_mus[12] = mus_flag_1;
    full_mus[13] = mus_flag_2;
    full_mus[14] = mus_flag_3;
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
            mus[3*readcntr + 0] = hue;
            mus[3*readcntr + 1] = 1;
            mus[3*readcntr + 2] = 1;
            sigmas_amudi[readcntr] = std;

        }
    }
    myfile.close();
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
    cout << "Renderer name: " << info.name << endl;
    cout << "Texture formats: " << endl;
    for( Uint32 i = 0; i < info.num_texture_formats; i++ )
    {
        cout << SDL_GetPixelFormatName( info.texture_formats[i] ) << endl;
    }

    SDL_Texture* texture = SDL_CreateTexture
        (
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT
        );

    //vector< unsigned char > pixels( texWidth * texHeight * 4, 0 );

    pixel *pixels = (pixel*)malloc(PIXELS_PER_RUN*sizeof(pixel)*N_BUFFERS);
    memset(pixels, 0, PIXELS_PER_RUN*sizeof(pixel)*N_BUFFERS);
    uint8_t *final_pixels = (uint8_t*)malloc(SIZE_PIXELS);


    for (uint16_t y = 0; y < HEIGHT; y++)
    {
        for(uint16_t x = 0; x < WIDTH; x++)
        {
            mus_flag_1[3*(y*WIDTH + x) + 0] = 360*(((double)x)/WIDTH); 
            mus_flag_2[3*(y*WIDTH + x) + 0] = ((int)(360*(((double)x)/WIDTH)) + 50)%360; 
            mus_flag_3[3*(y*WIDTH + x) + 0] = ((int)(360*(((double)x)/WIDTH)) + 100)%360; 
            sigmas_flag[y*WIDTH + x] = 5;//pow(2,10*(((double)x)/WIDTH));
            sigmas_base[y*WIDTH + x] = 10000;
        }
    }  
    SDL_Event event;
    bool running = true;
    uint16_t cntr = 0;
    uint32_t full_cntr = 0;
    uint16_t sigma_state = 0, last_sigma_state = 0;
    for(uint32_t sigma_cntr = 0; sigma_cntr < HEIGHT*WIDTH; sigma_cntr += 1)
    {
        sigmas[sigma_cntr] = full_sigmas[sigma_state][sigma_cntr]; 
        mus[sigma_cntr*3 + 0] = full_mus[sigma_state][sigma_cntr*3 + 0]; 
    }
    while( running )
    {
        const Uint64 start = SDL_GetPerformanceCounter();

        if (cntr >= N_BUFFERS)
        {
            cntr = 0;
        }
        if (((full_cntr+1)%150) == 0)
        {
            sigma_state += 1;
            if(sigma_state == N_PATTERNS)
            {
                sigma_state = 0;
            }
        }
        for(uint32_t sigma_cntr = 0; sigma_cntr < HEIGHT*WIDTH; sigma_cntr += 1)
        {
            sigmas[sigma_cntr] = sigmas[sigma_cntr]*0.9 + full_sigmas[sigma_state][sigma_cntr]*0.1; 
            mus[sigma_cntr*3 + 0] = mus[sigma_cntr*3 + 0]*0.9 + full_mus[sigma_state][sigma_cntr*3 + 0]*0.1;
        }
        memset(&pixels[cntr*PIXELS_PER_RUN*sizeof(pixel)], 0, PIXELS_PER_RUN*sizeof(pixels));

        SDL_SetRenderDrawColor( renderer, 0, 0, 0, SDL_ALPHA_OPAQUE );
        SDL_RenderClear( renderer );

        while( SDL_PollEvent( &event ) )
        {
            if( ( SDL_QUIT == event.type ) ||
                ( SDL_KEYDOWN == event.type && SDL_SCANCODE_ESCAPE == event.key.keysym.scancode ) )
            {
                running = false;
                break;
            }
        }

        // splat down some random pixels
        hsv hsv_val;
        rgb rgb_val;
        hsv_val.s = 1.0;
        hsv_val.v = 1.0;
        cout << sigma_state << "; " << full_cntr << "                                     \r";
#if GLOBAL_SIGMA
        if (full_cntr > 2000)
        {
            cout << ":(\n";
            break;
        }
        double sigma = pow(2,abs((double)full_cntr - 1000.0)/100);
#endif  
        for( unsigned int i = 0; i < PIXELS_PER_RUN; i++ )
        {
            const unsigned int x = rand() % WIDTH;
            const unsigned int y = rand() % HEIGHT;

            //const unsigned int offset = SIZE_PIXELS + ( WIDTH * 4 * y ) + x * 4;
#if not GLOBAL_SIGMA
            double sigma = sigmas[y*WIDTH + x]; 
#endif
            hsv_val.h = getRandom(mus[3*(y*WIDTH + x) + 0],sigma,0,360); 
            rgb_val = hsv2rgb(hsv_val);

            pixels[PIXELS_PER_RUN*cntr + i].b = (int)(rgb_val.b*255); /*b*/
            pixels[PIXELS_PER_RUN*cntr + i].g = (int)(rgb_val.g*255); /*g*/       
            pixels[PIXELS_PER_RUN*cntr + i].r = (int)(rgb_val.r*255); /*r*/
            pixels[PIXELS_PER_RUN*cntr + i].x = x;
            pixels[PIXELS_PER_RUN*cntr + i].y = y;
            //SDL_APLHA_OPAQUE
        }

        //unsigned char* lockedPixels;
        //int pitch;
        //SDL_LockTexture
        //    (
        //    texture,
        //    NULL,
        //    reinterpret_cast< void** >( &lockedPixels ),
        //    &pitch
        //    );
        //std::copy( pixels.begin(), pixels.end(), lockedPixels );
        //SDL_UnlockTexture( texture );
        memset(final_pixels, 0, SIZE_PIXELS);
        for (uint16_t sub_cntr = 0; sub_cntr < N_BUFFERS; sub_cntr++)
        {
            for (uint16_t i = 0; i <  PIXELS_PER_RUN; i++)
            {
                const uint16_t x = pixels[PIXELS_PER_RUN*sub_cntr + i].x;
                const uint16_t y = pixels[PIXELS_PER_RUN*sub_cntr + i].y;
                if ((x < WIDTH) && (y < HEIGHT))
                {
                    final_pixels[(WIDTH*4*y) + x*4 + 0] = pixels[PIXELS_PER_RUN*sub_cntr + i].b;
                    final_pixels[(WIDTH*4*y) + x*4 + 1] = pixels[PIXELS_PER_RUN*sub_cntr + i].g;
                    final_pixels[(WIDTH*4*y) + x*4 + 2] = pixels[PIXELS_PER_RUN*sub_cntr + i].r;
                    final_pixels[(WIDTH*4*y) + x*4 + 3] = SDL_ALPHA_OPAQUE;
                }
            }
        }
        SDL_UpdateTexture
            (
            texture,
            NULL,
            &final_pixels[0],
            WIDTH * 4
            );

        SDL_RenderCopy( renderer, texture, NULL, NULL );
        SDL_RenderPresent( renderer );

        const Uint64 end = SDL_GetPerformanceCounter();
        const static Uint64 freq = SDL_GetPerformanceFrequency();
        const double seconds = ( end - start ) / static_cast< double >( freq );
        //cout << "Frame time: " << seconds * 1000.0 << "ms" << endl;
        cntr += 1;
        full_cntr += 1;
    }

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( window );
    SDL_Quit();
}
