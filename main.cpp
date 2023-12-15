#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>

const int W = 480;
const int H = 480;


// Function declaration.
sf::Texture Mandelbrot(double min_re, double max_re, double min_im, double max_im, int max_iteration, int escape_radius);
float Is_In_Mandelbrot_Set(double c_real, double c_imag, int max_iteration, int escape_radius);
sf::Color HSVtoRGB(float H, float S, float V);
void Zoom(double &min_re, double &max_re, double &min_im, double &max_im, int mx, int my, double z);

int main(void)
{
    sf::RenderWindow window(sf::VideoMode(W, H), "Mandelbrot");
    
    sf::Texture texture;
    sf::Sprite sprite;

    int max_iteration = 1<<8, escape_radius = 1<<2;
    double min_re = -2.0, max_re = 2.0;
    double min_im = -2.0, max_im = 2.0;

    double zoom = 1.0;

    texture = Mandelbrot(min_re, max_re, min_im, max_im, max_iteration, escape_radius);

    while (window.isOpen())
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed) window.close();

            // Handle mouse input.
            if (e.type == sf::Event::MouseButtonPressed)
            {
                int mx = e.mouseButton.x;               // mouse x-position on the screen.
                int my = e.mouseButton.y;               // mouse y-position on the screen.

                // Left Click to Zoom in.
                if (e.mouseButton.button == sf::Mouse::Left)
                {
                    Zoom(min_re, max_re, min_im, max_im, mx, my, 3.0);
                    texture = Mandelbrot(min_re, max_re, min_im, max_im, max_iteration, escape_radius);
                }

                // Right Click to Zoom out.
                if (e.mouseButton.button == sf::Mouse::Right)
                {
                    Zoom(min_re, max_re, min_im, max_im, mx, my, 1.0 / 3.0);
                    texture = Mandelbrot(min_re, max_re, min_im, max_im, max_iteration, escape_radius);
                }
            }
        }

        window.clear();
        sprite.setTexture(texture);
        window.draw(sprite);
        window.display();
    }
    return 0;
}

sf::Texture Mandelbrot(double min_re, double max_re, double min_im, double max_im, int max_iteration, int escape_radius)
{
    sf::Texture texture{};
    texture.create(W, H);

    sf::Uint8* pixels = new sf::Uint8[4 * W * H];                // set pixel array.


#pragma omp parallel for
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            int pixel_pos = 4 * (W * y + x);

            double cr = min_re + (max_re - min_re)*x / W;
            double ci = min_im + (max_im - min_im)*y / H;
            
            float smooth_iteration = Is_In_Mandelbrot_Set(cr, ci, max_iteration, escape_radius);

            float H = 360.0f * (smooth_iteration / max_iteration), S = 1.0, V = 0.0;

            if (floorf(smooth_iteration) < max_iteration)
            {
                V = 1.0;
            }

            sf::Color pixelColor = HSVtoRGB(H, S, V);

            pixels[pixel_pos]     = pixelColor.r;
            pixels[pixel_pos + 1] = pixelColor.g;
            pixels[pixel_pos + 2] = pixelColor.b;
            pixels[pixel_pos + 3] = pixelColor.a;
        }
    }
    texture.update(pixels, W, H, 0, 0);
    delete[] pixels;

    return texture;
}

float Is_In_Mandelbrot_Set(double c_real, double c_imag, int max_iteration, int escape_radius)
{
    double z0_real = 0.0, z0_imag = 0.0, zn_real = 0, zn_imag = 0, w = 0;

    float smooth_iteration = (float) max_iteration;

    int iteration = 0;

    while (z0_real+z0_imag <= escape_radius && iteration < max_iteration) {
        zn_real = z0_real - z0_imag + c_real;
        zn_imag = w - z0_real - z0_imag + c_imag;
        z0_real = zn_real * zn_real;
        z0_imag = zn_imag * zn_imag;
        w       = (zn_real+zn_imag) * (zn_real+zn_imag);
        iteration++;
    }

    if (iteration < max_iteration)
    {
        // float log_zn = logf(z0_real*z0_real + z0_imag*z0_imag) / 2.0;
        // float nu     = logf(log_zn / logf(2.0)) / logf(2.0);
        smooth_iteration = iteration + 1 - logf(log2f((z0_real*z0_real + z0_imag*z0_imag)));
    }

    return smooth_iteration;
}

sf::Color HSVtoRGB(float H, float S, float V) 
{
	if (H > 360.0f || H < 0.0f || S > 1.0f || S < 0.0f || V > 1.0f || V < 0.0f) {
		return sf::Color::Black;
	}

    float M = 255.0f * V;
    float m = M * (1.0 - S);

    float z = (M - m) * (1.0f - fabsf(fmodf(H/60.0, 2.0) - 1.0));
    
    float R, G, B;
    
    if (H > 0.0f && H < 60.0)
    {
        R = M;
        G = z + m;
        B = m;
    } 
    else if (H >= 60 && H < 120)
    {
        R = z + m;
        G = M;
        B = m;
    } 
    else if (H >= 120 && H < 180)
    {
        R = m;
        G = M;
        B = z + m;
    } 
    else if (H >= 180 && H < 240)
    {
        R = m;
        G = z + m;
        B = M;
    } 
    else if (H >= 240 && H < 300)
    {
        R = z + m;
        G = m;
        B = M;
    } 
    else {
        R = M;
        G = m;
        B = z + m;
    }   
    
    return sf::Color((sf::Uint8) roundf(R), (sf::Uint8) roundf(G), (sf::Uint8) roundf(B), 255);
}

void Zoom(double &min_re, double &max_re, double &min_im, double &max_im, int mx, int my, double z)
{
    // mouse point will be new center point
    double cr = min_re + (max_re - min_re)*mx / W;
    double ci = min_im + (max_im - min_im)*my / H;

    // zoom
    double new_min_re = cr - (max_re - min_re) / 2.0 / z;
    double new_max_re = cr + (max_re - min_re) / 2.0 / z;
    double new_min_im = ci - (max_im - min_im) / 2.0 / z;
    double new_max_im = ci + (max_im - min_im) / 2.0 / z;

    min_re = new_min_re;
    max_re = new_max_re;
    min_im = new_min_im;
    max_im = new_max_im;
}