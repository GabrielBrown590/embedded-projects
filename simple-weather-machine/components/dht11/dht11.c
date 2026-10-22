#include "driver/gpio.h"
#include "esp_timer.h"
#include <math.h>
#include <stdlib.h>

//starts communication with DHT. Error codes: 1-got no response from DHT
int sendStart(gpio_num_t pin)
{
    //send start signal 
    gpio_set_level(pin, 0);
    waituS(25);
    gpio_set_level(pin, 1);

    //listen for confirmation
    waituS(5);
    int hearsMessage = !gpio_get_level(pin);
    if(hearsMessage)
    {
        waituS(75);
    }
    else
    {
        return 1;
    }
    return 0;

}

int getReading(gpio_num_t pin, float* temp, float* humid)
{
    //array to store a the binary reading from the DHT11
    int binResultArr[2][16];

    int attempts = 0;
    while(attempts < 3) //make 3 attempts to get a reading, reports error code if 3rd attempt fails
    {
        //listen for start of a new report from DHT
        int pinLevel = gpio_get_level(pin);
        int curSignalStartsReport = 1; // true if cur signal is signal is the 50us message signalling the starting of a new transmission
        for(int i = 0; i < 9; i++)
        {
            if(pinLevel == 1) curSignalStartsReport = 0;
            pinLevel = gpio_get_level(pin);
        }
        
        if(!curSignalStartsReport) continue; //failed to hear the start of a new report for whatever reason, try again
        else //successfully got the start signal, ready to read data.
        {
            int curmeasurment = 0; //keeps track of what we're measuring
            
            for(int i = 0; i < 32; i++)
            {
                //wait till level is back to 0 as this signals the start of a bit transmission
                while(gpio_get_level(pin))
                {
                    
                }
                waituS(80);
                binResultArr[curmeasurment][i%16] = gpio_get_level(pin);
                if(i != 0 && i % 16 == 0) curmeasurment++;
            }
        }
        
        //got a measurement, just need to interpret and return
        double* interpretedResults = binArrtoInts(binResultArr);
        *temp = interpretedResults[0];
        *humid = interpretedResults[1];
        free(interpretedResults);
        return 0; //success
    }
}

//waits time picoseconds.
void waituS(int time)
{
int64_t initialTime = esp_timer_get_time();
int64_t goalTime = initialTime + time;
while(esp_timer_get_time() < goalTime)
{
}
}

//converts a binary array passed as argument to 2 different doubles. Requires freeing the return array.
double* binArrtoInts(int binArr[][16])
{
    double* ret = calloc(sizeof(double), 2);
    union {
        unsigned int x;
        double y;
    } converter;

    converter.x = binArr[0];
    ret[0] = converter.y;

    converter.x = binArr[1];
    ret[1] = converter.y;
    return ret;
}

//sets temp and humid to correct values based on DHT11 readings. Error codes:0-success, 1-got no response from DHT
int readDHT11(gpio_num_t pin, double* temp, double* humid)
{
    //set up pin and error code handler
    int code = 0;
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);

    //send start signal to begin communication with DHT
    code = sendStart(pin);
    if(code != 0) return code;

    //get reading and return
    return getReading(pin, temp, humid);
}




