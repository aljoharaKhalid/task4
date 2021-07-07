/* Copyright (c) 2007-2012 Eliot Eshelman
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "simplexnoise.h"

#include <cmath>

/* 2D, 3D and 4D Simplex Noise functions return 'random' values in (-1, 1).

This algorithm was originally designed by Ken Perlin, but my code has been
adapted from the implementation written by Stefan Gustavson (stegu@itn.liu.se)

Raw Simplex noise functions return the value generated by Ken's algorithm.

Scaled Raw Simplex noise functions adjust the range of values returned from the
traditional (-1, 1) to whichever bounds are passed to the function.

Multi-Octave Simplex noise functions combine multiple noise values to create a
more complex result. Each successive layer of noise is adjusted and scaled.

Scaled Multi-Octave Simplex noise functions scale the values returned from the
traditional (-1,1) range to whichever range is passed to the function.

In many cases, you may think you only need a 1D noise function, but in practice
2D  is almost always better.  For instance, if you're using the current frame
number  as the parameter for the noise, all objects will end up with the same
noise value  at each frame. By adding a second parameter on the second
dimension, you can ensure that each gets a unique noise value and they don't
all look identical.
*/

// 2D Multi-octave Simplex noise.
//
// For each octave, a higher frequency/lower amplitude function will be added to the original.
// The higher the persistence [0-1], the more of each succeeding octave will be added.
float octave_noise_2d( const float octaves, const float persistence, const float scale,
                       const float x, const float y )
{
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;

    // We have to keep track of the largest possible amplitude,
    // because each octave adds more, and we need a value in [-1, 1].
    float maxAmplitude = 0.0f;

    for( int i = 0; i < octaves; i++ ) {
        total += raw_noise_2d( x * frequency, y * frequency ) * amplitude;

        frequency *= 2;
        maxAmplitude += amplitude;
        amplitude *= persistence;
    }

    return total / maxAmplitude;
}

// 3D Multi-octave Simplex noise.
//
// For each octave, a higher frequency/lower amplitude function will be added to the original.
// The higher the persistence [0-1], the more of each succeeding octave will be added.
float octave_noise_3d( const float octaves, const float persistence, const float scale,
                       const float x, const float y, const float z )
{
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;

    // We have to keep track of the largest possible amplitude,
    // because each octave adds more, and we need a value in [-1, 1].
    float maxAmplitude = 0.0f;

    for( int i = 0; i < octaves; i++ ) {
        total += raw_noise_3d( x * frequency, y * frequency, z * frequency ) * amplitude;

        frequency *= 2;
        maxAmplitude += amplitude;
        amplitude *= persistence;
    }

    return total / maxAmplitude;
}

// 4D Multi-octave Simplex noise.
//
// For each octave, a higher frequency/lower amplitude function will be added to the original.
// The higher the persistence [0-1], the more of each succeeding octave will be added.
float octave_noise_4d( const float octaves, const float persistence, const float scale,
                       const float x, const float y, const float z, const float w )
{
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;

    // We have to keep track of the largest possible amplitude,
    // because each octave adds more, and we need a value in [-1, 1].
    float maxAmplitude = 0.0f;

    for( int i = 0; i < octaves; i++ ) {
        total += raw_noise_4d( x * frequency, y * frequency, z * frequency, w * frequency ) * amplitude;

        frequency *= 2;
        maxAmplitude += amplitude;
        amplitude *= persistence;
    }

    return total / maxAmplitude;
}

// 2D Scaled Multi-octave Simplex noise.
//
// Returned value will be between loBound and hiBound.
float scaled_octave_noise_2d( const float octaves, const float persistence, const float scale,
                              const float loBound, const float hiBound, const float x, const float y )
{
    return octave_noise_2d( octaves, persistence, scale, x,
                            y ) * ( hiBound - loBound ) / 2 + ( hiBound + loBound ) / 2;
}

// 3D Scaled Multi-octave Simplex noise.
//
// Returned value will be between loBound and hiBound.
float scaled_octave_noise_3d( const float octaves, const float persistence, const float scale,
                              const float loBound, const float hiBound, const float x, const float y, const float z )
{
    return octave_noise_3d( octaves, persistence, scale, x, y,
                            z ) * ( hiBound - loBound ) / 2 + ( hiBound + loBound ) / 2;
}

// 4D Scaled Multi-octave Simplex noise.
//
// Returned value will be between loBound and hiBound.
float scaled_octave_noise_4d( const float octaves, const float persistence, const float scale,
                              const float loBound, const float hiBound, const float x, const float y, const float z,
                              const float w )
{
    return octave_noise_4d( octaves, persistence, scale, x, y, z,
                            w ) * ( hiBound - loBound ) / 2 + ( hiBound + loBound ) / 2;
}

// 2D Scaled Simplex raw noise.
//
// Returned value will be between loBound and hiBound.
float scaled_raw_noise_2d( const float loBound, const float hiBound, const float x, const float y )
{
    return raw_noise_2d( x, y ) * ( hiBound - loBound ) / 2 + ( hiBound + loBound ) / 2;
}

// 3D Scaled Simplex raw noise.
//
// Returned value will be between loBound and hiBound.
float scaled_raw_noise_3d( const float loBound, const float hiBound, const float x, const float y,
                           const float z )
{
    return raw_noise_3d( x, y, z ) * ( hiBound - loBound ) / 2 + ( hiBound + loBound ) / 2;
}

// 4D Scaled Simplex raw noise.
//
// Returned value will be between loBound and hiBound.
float scaled_raw_noise_4d( const float loBound, const float hiBound, const float x, const float y,
                           const float z, const float w )
{
    return raw_noise_4d( x, y, z, w ) * ( hiBound - loBound ) / 2 + ( hiBound + loBound ) / 2;
}

// 2D raw Simplex noise
float raw_noise_2d( const float x, const float y )
{
    // Noise contributions from the three corners
    float n0 = 0.0f;
    float n1 = 0.0f;
    float n2 = 0.0f;

    // Skew the input space to determine which simplex cell we're in
    static const float F2 = 0.5f * ( std::sqrt( 3.0f ) - 1.0f );
    // Hairy factor for 2D
    float s = ( x + y ) * F2;
    int i = fastfloor( x + s );
    int j = fastfloor( y + s );

    static const float G2 = ( 3.0f - std::sqrt( 3.0f ) ) / 6.0f;
    float t = ( i + j ) * G2;
    // Unskew the cell origin back to (x,y) space
    float X0 = i - t;
    float Y0 = j - t;
    // The x,y distances from the cell origin
    float x0 = x - X0;
    float y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.

    // Offsets for second (middle) corner of simplex in (i,j) coordinates
    int i1;
    int j1;
    if( x0 > y0 ) {
        i1 = 1;    // lower triangle, XY order: (0,0)->(1,0)->(1,1)
        j1 = 0;
    } else {
        i1 = 0;    // upper triangle, YX order: (0,0)->(0,1)->(1,1)
        j1 = 1;
    }

    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6
    float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coordinates
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coordinates
    float y2 = y0 - 1.0f + 2.0f * G2;

    // Work out the hashed gradient indices of the three simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = perm[ii + perm[jj]] % 12;
    int gi1 = perm[ii + i1 + perm[jj + j1]] % 12;
    int gi2 = perm[ii + 1 + perm[jj + 1]] % 12;

    // Calculate the contribution from the three corners
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if( t0 < 0 ) {
        n0 = 0.0f;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot( grad3[gi0], x0, y0 ); // (x,y) of grad3 used for 2D gradient
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if( t1 < 0 ) {
        n1 = 0.0f;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot( grad3[gi1], x1, y1 );
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if( t2 < 0 ) {
        n2 = 0.0f;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot( grad3[gi2], x2, y2 );
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 70.0f * ( n0 + n1 + n2 );
}

// 3D raw Simplex noise
float raw_noise_3d( const float x, const float y, const float z )
{
    // Noise contributions from the four corners
    float n0;
    float n1;
    float n2;
    float n3;

    // Skew the input space to determine which simplex cell we're in
    float F3 = 1.0f / 3.0f;
    float s = ( x + y + z ) * F3; // Very nice and simple skew factor for 3D
    int i = fastfloor( x + s );
    int j = fastfloor( y + s );
    int k = fastfloor( z + s );

    float G3 = 1.0f / 6.0f; // Very nice and simple unskew factor, too
    float t = ( i + j + k ) * G3;
    float X0 = i - t; // Unskew the cell origin back to (x,y,z) space
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0; // The x,y,z distances from the cell origin
    float y0 = y - Y0;
    float z0 = z - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    int i1; // Offsets for second corner of simplex in (i,j,k) coordinates
    int j1;
    int k1;
    int i2; // Offsets for third corner of simplex in (i,j,k) coordinates
    int j2;
    int k2;

    if( x0 >= y0 ) {
        if( y0 >= z0 ) {
            i1 = 1;    // X Y Z order
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;
        } else if( x0 >= z0 ) {
            i1 = 1;    // X Z Y order
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 0;
            k2 = 1;
        } else {
            i1 = 0;    // Z X Y order
            j1 = 0;
            k1 = 1;
            i2 = 1;
            j2 = 0;
            k2 = 1;
        }
    } else { // x0<y0
        if( y0 < z0 ) {
            i1 = 0;    // Z Y X order
            j1 = 0;
            k1 = 1;
            i2 = 0;
            j2 = 1;
            k2 = 1;
        } else if( x0 < z0 ) {
            i1 = 0;    // Y Z X order
            j1 = 1;
            k1 = 0;
            i2 = 0;
            j2 = 1;
            k2 = 1;
        } else {
            i1 = 0;    // Y X Z order
            j1 = 1;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;
        }
    }

    // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
    // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
    // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
    // c = 1/6.
    float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coordinates
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f * G3; // Offsets for third corner in (x,y,z) coordinates
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3; // Offsets for last corner in (x,y,z) coordinates
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    // Work out the hashed gradient indices of the four simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
    int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
    int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
    int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

    // Calculate the contribution from the four corners
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if( t0 < 0 ) {
        n0 = 0.0f;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot( grad3[gi0], x0, y0, z0 );
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if( t1 < 0 ) {
        n1 = 0.0f;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot( grad3[gi1], x1, y1, z1 );
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if( t2 < 0 ) {
        n2 = 0.0f;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot( grad3[gi2], x2, y2, z2 );
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if( t3 < 0 ) {
        n3 = 0.0f;
    } else {
        t3 *= t3;
        n3 = t3 * t3 * dot( grad3[gi3], x3, y3, z3 );
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to stay just inside [-1,1]
    return 32.0f * ( n0 + n1 + n2 + n3 );
}

// 4D raw Simplex noise
float raw_noise_4d( const float x, const float y, const float z, const float w )
{
    // The skewing and unskewing factors are hairy again for the 4D case
    static const float F4 = ( std::sqrt( 5.0f ) - 1.0f ) / 4.0f;
    static const float G4 = ( 5.0f - std::sqrt( 5.0f ) ) / 20.0f;
    // Noise contributions from the five corners
    float n0;
    float n1;
    float n2;
    float n3;
    float n4;

    // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
    float s = ( x + y + z + w ) * F4; // Factor for 4D skewing
    int i = fastfloor( x + s );
    int j = fastfloor( y + s );
    int k = fastfloor( z + s );
    int l = fastfloor( w + s );
    float t = ( i + j + k + l ) * G4; // Factor for 4D unskewing
    float X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
    float Y0 = j - t;
    float Z0 = k - t;
    float W0 = l - t;

    float x0 = x - X0; // The x,y,z,w distances from the cell origin
    float y0 = y - Y0;
    float z0 = z - Z0;
    float w0 = w - W0;

    // For the 4D case, the simplex is a 4D shape I won't even try to describe.
    // To find out which of the 24 possible simplices we're in, we need to
    // determine the magnitude ordering of x0, y0, z0 and w0.
    // The method below is a good way of finding the ordering of x,y,z,w and
    // then find the correct traversal order for the simplex we're in.
    // First, six pair-wise comparisons are performed between each possible pair
    // of the four coordinates, and the results are used to add up binary bits
    // for an integer index.
    int c1 = ( x0 > y0 ) ? 32 : 0;
    int c2 = ( x0 > z0 ) ? 16 : 0;
    int c3 = ( y0 > z0 ) ? 8 : 0;
    int c4 = ( x0 > w0 ) ? 4 : 0;
    int c5 = ( y0 > w0 ) ? 2 : 0;
    int c6 = ( z0 > w0 ) ? 1 : 0;
    int c = c1 + c2 + c3 + c4 + c5 + c6;

    // The integer offsets for the second simplex corner
    int i1;
    int j1;
    int k1;
    int l1;
    // The integer offsets for the third simplex corner
    int i2;
    int j2;
    int k2;
    int l2;
    // The integer offsets for the fourth simplex corner
    int i3;
    int j3;
    int k3;
    int l3;

    // simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
    // Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
    // impossible. Only the 24 indices which have non-zero entries make any sense.
    // We use a thresholding to set the coordinates in turn from the largest magnitude.
    // The number 3 in the "simplex" array is at the position of the largest coordinate.
    i1 = simplex[c][0] >= 3 ? 1 : 0;
    j1 = simplex[c][1] >= 3 ? 1 : 0;
    k1 = simplex[c][2] >= 3 ? 1 : 0;
    l1 = simplex[c][3] >= 3 ? 1 : 0;
    // The number 2 in the "simplex" array is at the second largest coordinate.
    i2 = simplex[c][0] >= 2 ? 1 : 0;
    j2 = simplex[c][1] >= 2 ? 1 : 0;
    k2 = simplex[c][2] >= 2 ? 1 : 0;
    l2 = simplex[c][3] >= 2 ? 1 : 0;
    // The number 1 in the "simplex" array is at the second smallest coordinate.
    i3 = simplex[c][0] >= 1 ? 1 : 0;
    j3 = simplex[c][1] >= 1 ? 1 : 0;
    k3 = simplex[c][2] >= 1 ? 1 : 0;
    l3 = simplex[c][3] >= 1 ? 1 : 0;
    // The fifth corner has all coordinate offsets = 1, so no need to look that up.

    float x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coordinates
    float y1 = y0 - j1 + G4;
    float z1 = z0 - k1 + G4;
    float w1 = w0 - l1 + G4;
    float x2 = x0 - i2 + 2.0f * G4; // Offsets for third corner in (x,y,z,w) coordinates
    float y2 = y0 - j2 + 2.0f * G4;
    float z2 = z0 - k2 + 2.0f * G4;
    float w2 = w0 - l2 + 2.0f * G4;
    float x3 = x0 - i3 + 3.0f * G4; // Offsets for fourth corner in (x,y,z,w) coordinates
    float y3 = y0 - j3 + 3.0f * G4;
    float z3 = z0 - k3 + 3.0f * G4;
    float w3 = w0 - l3 + 3.0f * G4;
    float x4 = x0 - 1.0f + 4.0f * G4; // Offsets for last corner in (x,y,z,w) coordinates
    float y4 = y0 - 1.0f + 4.0f * G4;
    float z4 = z0 - 1.0f + 4.0f * G4;
    float w4 = w0 - 1.0f + 4.0f * G4;

    // Work out the hashed gradient indices of the five simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int ll = l & 255;
    int gi0 = perm[ii + perm[jj + perm[kk + perm[ll]]]] % 32;
    int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1 + perm[ll + l1]]]] % 32;
    int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2 + perm[ll + l2]]]] % 32;
    int gi3 = perm[ii + i3 + perm[jj + j3 + perm[kk + k3 + perm[ll + l3]]]] % 32;
    int gi4 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1 + perm[ll + 1]]]] % 32;

    // Calculate the contribution from the five corners
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0 - w0 * w0;
    if( t0 < 0 ) {
        n0 = 0.0f;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot( grad4[gi0], x0, y0, z0, w0 );
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1 - w1 * w1;
    if( t1 < 0 ) {
        n1 = 0.0f;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot( grad4[gi1], x1, y1, z1, w1 );
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2 - w2 * w2;
    if( t2 < 0 ) {
        n2 = 0.0f;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot( grad4[gi2], x2, y2, z2, w2 );
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3 - w3 * w3;
    if( t3 < 0 ) {
        n3 = 0.0f;
    } else {
        t3 *= t3;
        n3 = t3 * t3 * dot( grad4[gi3], x3, y3, z3, w3 );
    }

    float t4 = 0.6f - x4 * x4 - y4 * y4 - z4 * z4 - w4 * w4;
    if( t4 < 0 ) {
        n4 = 0.0f;
    } else {
        t4 *= t4;
        n4 = t4 * t4 * dot( grad4[gi4], x4, y4, z4, w4 );
    }

    // Sum up and scale the result to cover the range [-1,1]
    return 27.0f * ( n0 + n1 + n2 + n3 + n4 );
}

int fastfloor( const float x )
{
    return x > 0 ? static_cast<int>( x ) : static_cast<int>( x ) - 1;
}

float dot( const int *g, const float x, const float y )
{
    return g[0] * x + g[1] * y;
}
float dot( const int *g, const float x, const float y, const float z )
{
    return g[0] * x + g[1] * y + g[2] * z;
}
float dot( const int *g, const float x, const float y, const float z, const float w )
{
    return g[0] * x + g[1] * y + g[2] * z + g[3] * w;
}
