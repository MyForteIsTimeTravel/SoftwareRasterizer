/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Software Rasterizer
 *  Ryan Needham
 *  2018
 *
 *  a single-file implementation of a slightly accelerated
 *  software rasterizer that generates PPM output
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>

static const float eps1 = 1e-7f;
static const float eps2 = 1e-10;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  vec3 operations
 *
 *  basic linear algebra operations on a 3D vector
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
typedef std::array<float, 3> vec3;
inline vec3 operator+ (const vec3& lhs, const vec3& rhs) { return { lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2] }; }
inline vec3 operator- (const vec3& lhs, const vec3& rhs) { return { lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2] }; }
inline vec3 operator/ (const vec3& lhs, const vec3& rhs) { return { lhs[0] / rhs[0], lhs[1] / rhs[1], lhs[2] / rhs[2] }; }
inline vec3 operator* (const vec3& lhs, const vec3& rhs) { return { lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2] }; }
inline vec3 operator/ (const vec3& lhs, const float rhs) { return { lhs[0] / rhs, lhs[1] / rhs, lhs[2] / rhs }; }
inline vec3 operator* (const vec3& lhs, const float rhs) { return { lhs[0] * rhs, lhs[1] * rhs, lhs[2] * rhs }; }
inline vec3 cross (const vec3& lhs, const vec3& rhs)
    { return { lhs[1] * rhs[2] - lhs[2] * rhs[1], lhs[2] * rhs[0] - lhs[0] * rhs[2], lhs[0] * rhs[1] - lhs[1] * rhs[0]}; }
inline float dot (const vec3& lhs, const vec3& rhs)
    { return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2]; }

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  framebuffer abstraction
 *
 *  a bare-bones implementation of a framebuffer with write
 *  to ppm fascilities
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
struct Frame
    {
    const uint32_t    WIDTH;
    const uint32_t    HEIGHT;
    std::vector<vec3> buffer;
    
    Frame (uint32_t width, uint32_t height, vec3 fill):
        WIDTH (width), HEIGHT (height)
        { buffer.assign(WIDTH * HEIGHT, fill); }
        
    inline void setPixel (uint32_t x, uint32_t y, vec3 colour)
        { buffer [x + y * WIDTH] = colour; }
        
    inline vec3 getPixel (uint32_t x, uint32_t y)
        { return buffer [x + y * WIDTH]; }
        
    void writeBuffer (const std::string& path)
        { // Frame :: writeBuffer
        std::ofstream out (path);
        out << "P3 " << WIDTH << " " << HEIGHT << " 255" << std::endl;
        for (int32_t y = HEIGHT - 1; y >= 0; --y)
            for (int32_t x = 0; x < WIDTH; ++x)
                { // write pixels to file
                out << (int)(getPixel(x, y)[0] * 255) << " ";
                out << (int)(getPixel(x, y)[1] * 255) << " ";
                out << (int)(getPixel(x, y)[2] * 255) << std::endl;
                } // write pixels to file
        out.close();
        } // Frame :: writeBuffer
    
    };

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  main
 *
 *  runs the rasterizer for a hard-coded test triangle and
 *  writes the output to "ppm/triangle.ppm"
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int main(int argc, const char * argv[])
    { // SoftwareRasterizer :: main
    
    // our output will be written to a ppm file on disk
    // using the above abstraction of an image/framebuffer
    Frame frame (1024, 1024, { 0.32, 0.32, 0.32 });
    
    // define a basic triangle for us to test the
    // rasterizer with, using different colours
    // at each vertex to render interpolation
    struct Triangle {
    std::array<vec3, 3> vertices;
    std::array<vec3, 3> colors;
    } tri;
    
    tri.vertices[0] = { 80,  80,  100 }; tri.colors[0] = { 0.84, 0.84,  0.0 };
    tri.vertices[1] = { 160, 800, 100 }; tri.colors[1] = { 0.0,  0.84, 0.84 };
    tri.vertices[2] = { 480, 320, 100 }; tri.colors[2] = { 0.84, 0.0,  0.84 };
    
    // we now loop through all the pixels of the image and
    // perform intersection tests with the triangle at each
    // pixel to determine which should be coloured/rasterized
    for (uint32_t y = 0; y < frame.HEIGHT; ++y)
        for (uint32_t x = 0; x < frame.WIDTH; ++x)
            { // for each pixel
            
            // define our point and line through it
            const vec3 pix = { (float)x, (float)y, 0.0f };
            const vec3 dir = { 0.0f, 0.0f, 1.0f };
            
            // define the plane on which the triangle lies
            const vec3 u = tri.vertices[1] - tri.vertices[0];
            const vec3 v = tri.vertices[2] - tri.vertices[0];
            const vec3 n = cross(dir, v);
            
            // compute the intersection of the line and the plane
            // and find the barycentric coordinates of this intersection
            // with respect to our triangle
            const float a = dot (u, n);
            const vec3  s = pix - tri.vertices[0];
            const vec3  r = cross(s, u);
            const float d = dot (v, r) / a;
            
            std::array<float, 3> barycentrics;
            barycentrics[1] = dot (s,   n) / a;
            barycentrics[2] = dot (dir, r) / a;
            barycentrics[0] = 1.0f - (barycentrics[1] + barycentrics[2]);
            
            // see if the intersection point of the view ray and the
            // triangle plane is ~inside~ the triangle and render it
            bool offAlpha = (barycentrics[0] < -eps2);
            bool offBeta  = (barycentrics[1] < -eps2);
            bool offGamma = (barycentrics[2] < -eps2);
            bool zeroArea = (a <= eps1);
            
            if (!(zeroArea || offAlpha || offBeta || offGamma || (d <= 0.1f)))
                { // we hit a triangle
                frame.setPixel(x, y,
                    tri.colors[0] * barycentrics[0] +
                    tri.colors[1] * barycentrics[1] +
                    tri.colors[2] * barycentrics[2]);
                } // we hit a triangle
            
            } // for each pixel
    
    frame.writeBuffer("ppm/triangle.ppm");
    return 0;
    } // SoftwareRasterizer :: main
