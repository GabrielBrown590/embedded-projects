I made it more natural and personal, with less repetitive phrasing and fewer overly polished explanations. It should read more like a GitHub project README written by someone documenting their own work.

Copy everything inside the block into your `.md` file.


# Simple Weather Machine – DHT11 Driver from Scratch (ESP-IDF)

A small ESP32 project where I built my own DHT11 driver using the sensor's datasheet instead of relying on an existing library.

The goal was to read temperature and humidity directly from the sensor and understand how the communication works at a lower level. The driver handles the start signal, reads the 40-bit response using GPIO, and converts the received data into temperature and humidity values.

Example output:


```text
Temperature: 19.6 °C   Humidity: 35 %
```

## Why I built my own driver

Most DHT11 tutorials use a library that handles all the communication for you. I wanted to try implementing it myself to get a better understanding of how the sensor actually works.

The interesting part was figuring out how to read the data. The DHT11 uses the length of HIGH pulses to distinguish between 0s and 1s, so timing is pretty important. I had to work out how to generate the start signal, detect the sensor's response, and read each bit at the right time.

## Hardware

| ESP32 (Elegoo ESP-WROOM-32) | DHT11 |
| --- | --- |
| 3V3 | VCC |
| GND | GND |
| GPIO 4 | DATA |

A couple of notes about the wiring:

- The data line needs a 4.7–10 kΩ pull-up resistor connected to 3V3. My three-pin DHT11 module already has one built in.
- I went with GPIO 4 instead of GPIO 2 because GPIO 2 is a strapping pin and can affect the ESP32's boot behavior.

## How the communication works

The DHT11 uses a single data wire to communicate with the ESP32. It doesn't send data like a typical serial connection. Instead, each bit is represented by the length of a HIGH pulse.

The basic sequence is:

```text
1. The ESP32 pulls the data line LOW for 30 ms,
   then releases it HIGH.

2. The sensor responds by pulling the line LOW.
   The ESP32 waits 40 µs and checks for this response.

3. The sensor starts sending 40 bits of data.
   Each bit starts with a LOW pulse, followed by a HIGH pulse.

   Short HIGH pulse (~26–28 µs) = 0
   Long HIGH pulse (~70 µs)     = 1
```

The 40 bits are split into five bytes:

1. Humidity (integer)
2. Humidity (decimal)
3. Temperature (integer)
4. Temperature (decimal)
5. Checksum

My driver stores the bits in a 5-by-8 array. It then converts the first four bytes into decimal values. The integer and decimal portions are added together to get the final temperature and humidity readings.

The fifth byte is read but I haven't implemented checksum verification yet.

## How I implemented the driver

I kept the implementation fairly simple and split it into a few functions.

- **GPIO setup:** I use `GPIO_MODE_INPUT_OUTPUT_OD` to configure the data pin as open-drain. This lets the ESP32 send the start signal and read the sensor using the same pin.
- **Microsecond delays:** I wrote my own `waituS()` function using `esp_timer_get_time()`. It continuously checks the elapsed time until the requested delay has passed.
- **Starting communication:** `sendStart()` pulls the data line LOW for 30 ms using `vTaskDelay()`, then sets it HIGH. After waiting 40 µs, it checks whether the sensor has responded.
- **Reading the bits:** `getReading()` handles the 40-bit response. It waits for the data line to go LOW and then HIGH. Once the HIGH pulse starts, it waits 45 µs and samples the pin. This tells me whether the current bit is a 0 or a 1.
- **Converting the data:** `binArrtoInts()` takes the binary array and converts the first four bytes into temperature and humidity values. It uses the standard binary weights (128, 64, 32, etc.) to calculate each byte.
- **Returning the results:** The conversion function allocates memory for the two readings using `calloc()`. `getReading()` copies those values into the output pointers and frees the allocated memory.

For now, error handling is pretty basic. If the sensor doesn't respond during the initial handshake, the driver returns an error. I haven't added timeouts to the data-reading loops yet.

## API

The main function exposed by the driver is:

c

```c
int readDHT11(gpio_num_t pin, double* temp, double* humid);
```

It takes the GPIO pin connected to the DHT11 and pointers where the temperature and humidity readings will be stored.

| Return value | Meaning |
| --- | --- |
| `0` | Reading completed successfully |
| `1` | Sensor didn't respond during the initial handshake |

## Project structure

```text
simple-weather-machine/
├── components/
│   └── dht11/
│       ├── include/dht11.h
│       ├── dht11.c
│       └── CMakeLists.txt
└── main/
    └── simple_weather_machine.c
```

## Building and flashing

Built with ESP-IDF v6.1.

bash

```bash
idf.py set-target esp32
idf.py build flash monitor
```

## Things I'd like to improve

There are a few things I haven't implemented yet that I'd like to come back to:

- **Checksum verification:** I'm currently reading all 40 bits but ignoring the checksum byte. Adding a checksum check would let the driver detect corrupted readings.
- **Timeouts:** Right now, the driver waits in polling loops for the GPIO signal to change. If the sensor stops responding halfway through a reading, the ESP32 can get stuck waiting. Adding timeouts would fix this.
- **RMT peripheral:** The current approach uses busy-waiting to measure pulse lengths. I'd like to try using the ESP32's RMT peripheral to capture the pulses in hardware instead.
- **DHT22 support:** It would be interesting to extend the driver to support the DHT22 as well. It uses a similar communication protocol, although the data format is different.

## What I learned

This was a fun project because it forced me to work with the hardware instead of just calling a library function. I got to experiment with GPIO configuration, open-drain mode, timing with esp_timer, and converting raw binary data into usable measurements.

I also have a much better idea of what's actually happening when an ESP32 communicates with a sensor like the DHT11.
