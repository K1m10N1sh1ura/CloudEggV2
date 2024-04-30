Ultrasonic Sprint Timer named CloudEgg... the name has little to nothing to do with the project ^^

----------------------------------------

DESCRIPTION
------------

Sprint timer based on an ultrasonic barrier using an ESP32 microcontroller and an HC-SR04 ultrasonic sensor.
Runners can connect to the device via WLAN with their smartphone to start their measurements. The measured time is continuously updated and displayed on both the smartphone and the display on the microcontroller.
The measurement ends once the runner passes through the ultrasonic barrier.

FUNCTIONS
----------

- Ultrasonic Sensor: The HC-SR04 ultrasonic sensor is used to measure the distance between the sensor and the objects in front of it.
- ESP32 Microcontroller: The ESP32 controls the ultrasonic sensor and display. A web server is hosted on the ESP32 to provide a user interface for interacting with the sprint timer.
- Browser Interface: Users can connect to the sprint timer via a web browser, ideally on a smartphone, to start measurements and view results.
- Real-time Measurement: The ESP32 performs approximately 50 measurements per second to monitor the movement of the object and stop the timer when the object is within one meter or less from the sensor.
- Time Display: The measured time is displayed on both the browser window and a connected display on the ESP32.

HARDWARE REQUIREMENTS
-----------------------

- ESP32 Microcontroller
- HC-SR04 Ultrasonic Sensor
- Optional: LAFVIN 0.96 Inch OLED I2C Display

SETUP INSTRUCTIONS
---------------

1. Connect the HC-SR04 ultrasonic sensor to the ESP32 microcontroller: Echo to D33 and Trigger to D25.
2. Upload the corresponding code to the ESP32.
3. Power up the ESP32.
4. Search and connect your smartphone/PC via WLAN to the "CloudEgg network" (Password: cloudegg).
6. Open the IP address of the ESP32 (192.168.4.1) with a browser to access the user interface.
7. Start a measurement via the user interface. The process is self-explanatory. After an acoustic sequence, the measurement starts. Once you pass through the ultrasonic barrier, the measurement stops.
8. The measured time will be displayed on the browser window and the connected display.

NOTES
--------

- Ensure the ultrasonic sensor is properly aligned to obtain accurate measurements. There should be no objects in front of the sensor within a distance of 5 meters ideally.
- Open the address 192.168.4.1/validatePos to perform an automatic position validation.
- The accuracy of measurements may vary depending on the environment. Test the barrier with trial measurements before starting the actual training.

IMAGES
--------
- CloudEggV0

[Screenshot 2024-04-30 at 18 33 42](https://github.com/K1m10N1sh1ura/CloudEggV2/assets/54206499/2ffa8696-9d4e-4894-992a-00cc97a9642e)

- CloudEggV1

[Screenshot 2024-04-30 at 18 33 47](https://github.com/K1m10N1sh1ura/CloudEggV2/assets/54206499/3c84a235-f6ad-495a-906c-e558fc36be74)

[Screenshot 2024-04-30 at 18 33 52](https://github.com/K1m10N1sh1ura/CloudEggV2/assets/54206499/65cb7141-9545-4b2c-b4da-f35f5f6a1e2e)

- CloudEggV2 (under development with display, power switch, battery, and battery management)

AUTHORS
-------

- Kimio Nishiura (kimio.nishiura@me.com)
