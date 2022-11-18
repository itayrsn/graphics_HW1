//#define GLEW_STATIC
// #define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"
#include "stb_image.h"
#include "../res/includes/glad/include/glad/glad.h"
#include <iostream>
#include <fstream>
#include <cmath>

unsigned char* edge_detect(unsigned char* img, int& w, int& h)
{
    // sobel operators
    char kx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    char ky[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };
    float gauss[3][3] = { {1 / 16.0f,2 / 16.0f,1 / 16.0f}, {2 / 16.0f,4 / 16.0f,2 / 16.0f}, {1 / 16.0f,2 / 16.0f,1 / 16.0f} };
    int k_size = 3 / 2;

    // TODO change to dyanmic size using std array
    unsigned char data[256][256];
    unsigned char data_blur[256][256];
    unsigned char edges[256][256];
    float theta[256][256];
    unsigned char max_edges[256][256];
    unsigned char low = 85, high = 200; // double thresholds
    unsigned char weak = 25, strong = 255; // double thresholds
    unsigned char* output = new unsigned char[w*h*4];

    // copy to 2d
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int idx = (j + i * w) * 4;
            data[i][j] = (img[idx] + img[idx + 1] + img[idx + 2]) / 3;
        }
    }

    // blur
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            float sum = 0;

            for (int m = 0; m < 3; m++)
            {
                for (int n = 0; n < 3; n++)
                {
                    int row = i - k_size + m;
                    int col = j - k_size + n;
                    if (row >= 0 && row < h && col >= 0 && col < w)
                    {
                        sum += gauss[m][n] * (float)data[row][col];
                    }
                }
            }

            data_blur[i][j] = sum;
        }
    }

    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int gx = 0;
            int gy = 0;

            for (int m = 0; m < 3; m++)
            {
                for (int n = 0; n < 3; n++)
                {
                    int row = i - k_size + m;
                    int col = j - k_size + n;
                    if (row >= 0 && row < h && col >= 0 && col < w)
                    {
                        gx += kx[m][n] * data_blur[row][col];
                        gy += ky[m][n] * data_blur[row][col];
                    }
                }
            }

            edges[i][j] = sqrt(gx * gx + gy * gy);
            theta[i][j] = atan2(gy, gx) * 180.0f / 3.1415926f;
            if (theta[i][j] > 180) theta[i][j] += 180;
        }
    }

    for (int i = 1; i < h-1; i++)
    {
        for (int j = 1; j < w-1; j++)
        {
            unsigned char q = 255;
            unsigned char r = 255;
            
            if (0 <= theta[i][j] < 22.5 || 157.5 <= theta[i][j] <= 180)
            {
                q = edges[i][j + 1];
                r = edges[i][j - 1];
            }
            else if (22.5 <= theta[i][j] < 67.5)
            {
                q = edges[i+1][j - 1];
                r = edges[i-1][j + 1];
            }
            else if (67.5 <= theta[i][j] < 112.5)
            {
                q = edges[i + 1][j];
                r = edges[i - 1][j];
            }
            else if (112.5 <= theta[i][j] < 157.5)
            {
                q = edges[i - 1][j - 1];
                r = edges[i + 1][j + 1];
            }

            if (edges[i][j] >= q && edges[i][j] >= r)
                max_edges[i][j] = edges[i][j];
            else
                max_edges[i][j] = 0;
        }
    }

    for (int i = 1; i < h - 1; i++)
    {
        for (int j = 1; j < w - 1; j++)
        {
            if (max_edges[i][j] < low) max_edges[i][j] = 0;
            else if (max_edges[i][j] < high) max_edges[i][j] = weak;
            else max_edges[i][j] = strong;
        }
    }

    for (int i = 1; i < h - 1; i++)
    {
        for (int j = 1; j < w - 1; j++)
        {
            if (max_edges[i][j] == weak)
            {
                if (max_edges[i+1][j-1] == strong || max_edges[i+1][j] == strong || max_edges[i+1][j+1] == strong ||
                    max_edges[i][j-1] == strong || max_edges[i][j+1] == strong || max_edges[i-1][j-1] == strong ||
                    max_edges[i-1][j] == strong || max_edges[i-1][j+1] == strong )
                {
                    max_edges[i][j] = strong;
                }
                else
                {
                    max_edges[i][j] = 0;
                }
            }
        }
    }

    std::ofstream tfile("img4.txt");

    // return to 1d
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int idx = (j + i * w) * 4;
            output[idx] = max_edges[i][j];
            output[idx+1] = max_edges[i][j];
            output[idx+2] = max_edges[i][j];
            output[idx+3] = 255;

            tfile << max_edges[i][j]/255.0f << ",";
        }
    }

    tfile.close();
    return output;
}

unsigned char* halftone(unsigned char* img, int& w, int& h)
{
    // TODO change to dyanmic size using std array
    unsigned char data[256*2][256*2];
    unsigned char* output = new unsigned char[w*2 * h*2 * 4];

    // copy to 2d
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int idx = (j + i * w) * 4;
            data[i*2][j*2] = (img[idx] + img[idx + 1] + img[idx + 2]) / 3;
        }
    }

    for (int i = 0; i < h*2; i+=2)
    {
        for (int j = 0; j < w*2; j+=2)
        {
            float I = (float)data[i][j] / 255.0f / 1;
            
            if (I < 0.2)
            {
                data[i][j] = 0;
                data[i][j + 1] = 0;
                data[i + 1][j] = 0;
                data[i + 1][j + 1] = 0;
            }
            else if (I < 0.4)
            {
                data[i][j] = 0;
                data[i][j + 1] = 0;
                data[i + 1][j] = 255;
                data[i + 1][j + 1] = 0;
            }
            else if (I < 0.6)
            {
                data[i][j] = 0;
                data[i][j + 1] = 255;
                data[i + 1][j] = 255;
                data[i + 1][j + 1] = 0;
            }
            else if (I < 0.8)
            {
                data[i][j] = 0;
                data[i][j + 1] = 255;
                data[i + 1][j] = 255;
                data[i + 1][j + 1] = 255;
            }
            else
            {
                data[i][j] = 255;
                data[i][j + 1] = 255;
                data[i + 1][j] = 255;
                data[i + 1][j + 1] = 255;
            }
        }
    }

    // return to 1d
    
    w *= 2;
    h *= 2;

    std::ofstream tfile("img5.txt");

    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int idx = (j + i * w) * 4;
            output[idx] = data[i][j];
            output[idx + 1] = data[i][j];
            output[idx + 2] = data[i][j];
            output[idx + 3] = 255;

            tfile << data[i][j] / 255.0f << ",";
        }
    }
    tfile.close();
    return output;
}

// convert 256 grayscale to 16 grayscale 
unsigned char find_closest_palette_color(unsigned char c)
{
    if (c > 255) c = 255;
    else if (c < 0) c = 0;
    return round(c / 16.0f) * 16;
}

unsigned char* floyd_steinberg_dithering(unsigned char* img, int& w, int& h)
{
    // TODO change to dyanmic size using std array
    unsigned char data[256][256];
    unsigned char* output = new unsigned char[w * h * 4];

    // copy to 2d
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int idx = (j + i * w) * 4;
            data[i][j] = (img[idx] + img[idx + 1] + img[idx + 2]) / 3;
        }
    }

    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            unsigned char pixel = data[i][j];
            unsigned char new_pixel = find_closest_palette_color(pixel);
            float e = pixel - new_pixel;
            data[i][j] = new_pixel;

            if (j < w - 1)
                data[i][j + 1] += e * 7.0f / 16.0f;
            if (i < h - 1 && j > 0)
                data[i + 1][j - 1] += e * 3.0f / 16.0f;
            if (i < h - 1)
                data[i + 1][j] += e * 5.0f / 16.0f;
            if (i < h - 1 && j < w - 1)
                data[i + 1][j + 1] += e * 1.0f / 16.0f;
        }
    }

    std::ofstream tfile("img6.txt");

    // return to 1d
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            int idx = (j + i * w) * 4;
            output[idx] = data[i][j];
            output[idx + 1] = data[i][j];
            output[idx + 2] = data[i][j];
            output[idx + 3] = 255;

            tfile << round(data[i][j] / 16.0f) << ",";
        }
    }

    tfile.close();
    return output;
}

Texture::Texture(const std::string& fileName, Effect effect)
{
	int width, height, numComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load((fileName).c_str(), &width, &height, &numComponents, 4);
	
    if(data == NULL)
		std::cerr << "Unable to load texture: " << fileName << std::endl;
    
    if (data)
    {
        unsigned char* new_data = nullptr;

        switch (effect)
        {
            case Edges:
                new_data = edge_detect(data, width, height);
                free(data);
                data = new_data;
                break;
            case FSDithering:
                new_data = floyd_steinberg_dithering(data, width, height);
                free(data);
                data = new_data;
                break;
            case Halftone:
                new_data = halftone(data, width, height);
                free(data);
                data = new_data;
                break;
            default:
                break;
        }
    }

    glGenTextures(1, &m_texture);
    Bind(m_texture);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_LOD_BIAS,-0.4f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}

Texture::Texture(int width,int height,unsigned char *data)
{
    glGenTextures(1, &m_texture);
    Bind(m_texture);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_texture);
}

void Texture::Bind(int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}

