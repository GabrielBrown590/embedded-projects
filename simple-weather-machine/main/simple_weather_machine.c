#include <stdio.h>
#include "dht11.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define DHTPIN 4

void app_main(void)
{
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));
        double temp;
        double humidity;
        int code = readDHT11(DHTPIN, &temp, &humidity);
        if(code == 0) printf("Current temp: %.1f C. Current humidity: %.1f %%\n", temp, humidity);
        else
        {
            printf("The driver returned an error!\n");
        }

    }
}
