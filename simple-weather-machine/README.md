I made it more natural and personal, with less repetitive phrasing and fewer overly polished explanations. It should read more like a GitHub project README written by someone documenting their own work.

Copy everything inside the block into your `.md` file.

Writing

# Simple Weather Machine – DHT11 Driver from Scratch (ESP-IDF)

A small ESP32 project where I built my own DHT11 driver using the sensor's datasheet instead of relying on an existing library.

The goal was to read temperature and humidity directly from the sensor and understand how the communication works at a lower level. The driver handles the start signal, reads the 40-bit response using GPIO, and converts the received data into temperature and humidity values.

Example output:

text

```text
Temperature: 19.6 °C   Humidity: 35 %
```

\<\!-- Add a photo of the wiring here and a screenshot of the serial monitor output. --\>

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

text

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

text

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

This was a fun project because it forced me to work with the hardware instead of just calling a library function. I got to experiment with GPIO configuration, open-drain mode, timing with `esp_timer`, and converting raw binary data into usable measurements.

I also have a much better idea of what's actually happening when an ESP32 communicates with a sensor like the DHT11.
A from-scratch DHT11 driver for the ESP32, implemented using the sensor's datasheet without relying on third-party libraries.

The project reads temperature and humidity from a DHT11 sensor using its single-wire communication protocol. The driver generates the start signal, measures incoming pulses using microsecond timing, converts the received bits into readable values, and exposes a simple function for applications to retrieve sensor readings.

Example serial output:
Temperature: 19.6 °C   Humidity: 35 %

Why build the driver from scratch?

Most DHT11 tutorials rely on existing libraries that handle communication behind the scenes. I wanted to understand how the sensor works by implementing the communication process myself, directly from its datasheet.

This meant handling the timing of the start signal, detecting the sensor's response, reading individual bits based on pulse duration, and converting the received binary data into temperature and humidity values.

The result is a simple driver that gives me direct control over communication with the sensor without depending on an external library.

Hardware setup
ESP32 (Elegoo ESP-WROOM-32)	DHT11
3V3	VCC
GND	GND
GPIO 4	DATA

A couple of wiring considerations:

The data line requires a pull-up resistor (4.7–10 kΩ connected to 3V3). Most three-pin DHT11 modules already include one.
I chose GPIO 4 instead of GPIO 2 because GPIO 2 is an ESP32 strapping pin that can affect the boot process.
Understanding the DHT11 protocol

The DHT11 communicates with the ESP32 through a single data line. Information is encoded through the duration of signal pulses rather than conventional serial communication.

The communication process has three main stages:

text
1. Start signal:
   ESP32 pulls the data line LOW for approximately 30 ms,
   then releases it HIGH.

2. Sensor response:
   ESP32 waits 40 µs and checks whether the sensor
   has pulled the data line LOW.
   If a response is detected, it waits for the LOW
   response pulse to finish.

3. Data transmission:
   Sensor sends 40 bits.

   Each bit begins with a LOW pulse, followed by a HIGH pulse.
   The duration of the HIGH pulse determines the bit value.


The 40 received bits are organized into five bytes:

Humidity (integer)
Humidity (decimal)
Temperature (integer)
Temperature (decimal)
Checksum

My implementation processes the first four bytes to calculate the temperature and humidity. Each byte is converted from binary using positional weights, and the decimal byte is divided by 10 before being added to the integer portion.

The fifth byte is currently collected but not used for checksum verification.

Driver implementation

The driver is built around a few straightforward design decisions:

Open-drain GPIO: I use GPIO_MODE_INPUT_OUTPUT_OD so the same GPIO pin can send the start signal and receive the sensor's response. The pin is configured using gpio_reset_pin() and gpio_set_direction().
Microsecond timing: I implemented a custom waituS() function using esp_timer_get_time(). It continuously checks the elapsed time until the requested delay has passed. This is used to wait 40 µs after the start signal and 45 µs when determining the value of each received bit.
Start signal and response detection: The sendStart() function pulls the data line LOW for 30 ms using vTaskDelay(), then releases it HIGH. After waiting 40 µs, it checks whether the sensor has responded by pulling the line LOW.
Bit-by-bit data reception: The getReading() function reads all 40 bits individually. It waits for the signal to return LOW, then waits for it to go HIGH. After 45 µs, it samples the GPIO level to determine whether the current bit is a 0 or a 1.
Binary-to-decimal conversion: The binArrtoInts() function converts the received binary data into temperature and humidity values. It uses the first four bytes, combining the integer and decimal portions into two double values.
Dynamic memory management: The conversion function allocates space for the two resulting values using calloc(). The caller retrieves the results and frees the allocated memory afterward.
Basic error handling: The sendStart() function returns an error if the sensor does not respond during the initial response check. This error is passed back through readDHT11() to the application.
Public API

The main function exposed by the driver is:

c
int readDHT11(gpio_num_t pin, double* temp, double* humid);


It accepts the GPIO pin connected to the DHT11 and pointers to two double variables where the temperature and humidity readings will be stored.

Return value	Description
0	Reading completed successfully.
1	No sensor response detected during the initial communication check.
Project structure
text
simple-weather-machine/
├── components/
│   └── dht11/
│       ├── include/dht11.h      Public API
│       ├── dht11.c              Driver implementation
│       └── CMakeLists.txt       Component configuration
└── main/
    └── simple_weather_machine.c
                                Example application

Building and flashing

The project was developed using ESP-IDF v6.1.

To configure, build, flash, and monitor the ESP32:

bash
idf.py set-target esp32
idf.py build flash monitor

Limitations and future improvements

There are a few areas I would like to improve in future iterations:

Add checksum verification: Although the driver currently receives all 40 bits, it does not yet validate the fifth byte against the sum of the first four bytes. Implementing this would help detect corrupted readings.
Add timeout handling: The current implementation uses polling loops to wait for GPIO transitions. If the sensor stops responding during data transmission, these loops can wait indefinitely. Adding timeouts would make the driver more robust.
Improve timing efficiency: The waituS() function uses busy-waiting to measure microsecond intervals. Exploring the ESP32's RMT peripheral would allow pulse timing to be handled in hardware rather than relying entirely on software polling.
Add DHT22 support: The current implementation is designed for the DHT11. Supporting the DHT22 would require adapting the data interpretation to its different measurement format.
What I learned

Building this driver gave me practical experience working with GPIO configuration, open-drain signaling, microsecond timing, bitwise data interpretation, and ESP-IDF's FreeRTOS integration.

More importantly, it helped me understand what happens behind the scenes when a sensor library communicates with hardware. Instead of treating the DHT11 as a black box, I implemented the communication process myself, from generating the start signal to interpreting the received data.
