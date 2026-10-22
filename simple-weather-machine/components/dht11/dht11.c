#include "driver/gpio.h"
#include "esp_timer.h"
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


//waits time picoseconds.
void waituS(int time)
{
int64_t initialTime = esp_timer_get_time();
int64_t goalTime = initialTime + time;
while(esp_timer_get_time() < goalTime)
{
}
}

//starts communication with DHT. Error codes: 1-got no response from DHT
int sendStart(gpio_num_t pin)
{
    //send start signal 
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(30));
    gpio_set_level(pin, 1);

    //listen for confirmation
    waituS(40);
    int hearsMessage = !gpio_get_level(pin);
    if(hearsMessage)
    {
        while(!gpio_get_level(pin)) //wait for DHT response signal to finish
        {
            
        }
    }
    else
    {
        return 1;
    }
    return 0;

}

//converts a binary array passed as argument to 2 different doubles. Requires freeing the return array.
double* binArrtoInts(int binArr[5][8])
{
    double* ret = calloc(sizeof(double), 2);

    int weights[] = {128, 64, 32, 16, 8, 4, 2, 1};
    int j = 0;
    while (j < 4) //runs twice, once for j = 0 for humidity and once for j = 2 for temperature
    {
        //whole part
        for(int i = 0; i < 8; i++)
        {
            if(binArr[j][i]) ret[j/2]+= weights[i];
        }

        //decimal part
        int dec = 0;
        for(int i = 0; i < 8; i++)
        {
            if(binArr[j+1][i]) dec += weights[i];
        }
        ret[j/2]+=dec/10.0;
        j+=2;
    }
        
    return ret;
}


int getReading(gpio_num_t pin, double* temp, double* humid)
{
    //array to store a the binary reading from the DHT11
    int binResultArr[5][8];

    int pinLevel = gpio_get_level(pin);
    int curmeasurment = 0; //keeps track of what we're measuring
    
    for(int i = 0; i < 40; i++)
    {
        //wait till level is back to 0 as this signals the start of a bit transmission
        while(gpio_get_level(pin))
        {
            
        }

        //wait till level is back to 1, this is where we can differentiate between a 1 or 0
        while(!gpio_get_level(pin))
        {
            
        }
        waituS(45);

        if(i != 0 && i % 8 == 0) curmeasurment++;
        binResultArr[curmeasurment][i%8] = gpio_get_level(pin);
        
    }
    
    
    //got a measurement, just need to interpret and return
    double* interpretedResults = binArrtoInts(binResultArr);
    *temp = interpretedResults[1];
    *humid = interpretedResults[0];
    free(interpretedResults);
    return 0; //success
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




