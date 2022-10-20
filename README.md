# RUI3_RAK4631_LoRaWan_RAK1901_RAK12010

A small demo project, part of a much bigger effort, based on [WisBlock RAK4631](https://store.rakwireless.com/products/rak4631-lpwan-node) (nRF52840 + SX1262), [RAK1901](https://store.rakwireless.com/products/rak1901-shtc3-temperature-humidity-sensor) (Sensirion SHTC3 Temperature and Humidity Sensor) and [RAK12010](https://store.rakwireless.com/products/wisblock-ambient-light-sensor-rak12010) (VEML7700 Light Sensor).

I reads periodically datapoints from the two sensors, encodes them as 6 bytes, and sends them to the nearest gateway. The data is (hopefully) forwarded to [TTN](https://www.thethingsnetwork.org/), from where any number of post-processing can happen – in my case, webhooks send the decoded frame back to a server, hosted on a Raspberry Pi 3B+ with custom Python code that parses the data, saves it in an sqlite3 database, and CSV files for display in a dashboard.

But that's just me – you do you! :-)

Because this is an experiment, I bypassed CayenneLPP and used a very crude encoding format – at the expense of having more code on the server to recognize what the data is (I have other nodes sending different datapoints): in a more formal implementation, it would be better to have just one script parsing Cayenne data, and only one webhook. Also there's no checksum, which is probably not ideal. This'll come in time.

A `Secret.h` file is required, with the following content:

```c
#define OTAA_DEVEUI   {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}
#define OTAA_APPEUI   {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}
#define OTAA_APPKEY   {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}
```

These are the keys required by TTN to accept your device on the network. Use your own :-)