#ifndef _WEATHER_GEN_H_
#define _WEATHER_GEN_H_

#include "weather.h"
#include "calendar.h"
#include "simplexnoise.h"

struct point;

struct w_point {
    double temperature;
    double humidity;
    double pressure;
    bool acidic;

    w_point(double t, double h, double p, bool a = false) : temperature(t), humidity(h), pressure(p), acidic(a) { }
};

class PerlinNoise;

class weather_generator
{
    unsigned SEED;
    // Data source: Wolfram Alpha
    const double base_t = 6.5; // Average temperature of New England
    const double base_h = 66.0; // Average humidity
    const double base_p = 1015.0; // Average atmospheric pressure
public:
    weather_generator();
    weather_generator(unsigned seed);
    w_point get_weather(const point &, const calendar &);
    weather_type get_weather_conditions(const point &, const calendar &);
    weather_type get_weather_conditions(const w_point &) const;
    void test_weather();
};
#endif
