# Smart BMI Measurement System

This project is an Arduino-based Smart BMI Measurement System that automatically measures a person’s height, weight, temperature, and heart rate, and calculates the Body Mass Index (BMI). The system integrates multiple sensors with an Arduino Nano to provide a simple health monitoring setup suitable for demonstration or prototype healthcare applications.

## Features

* Automatic height measurement using VL53L0X distance sensor
* Weight measurement using load cell with HX711 amplifier
* Body temperature monitoring using DS18B20 sensor
* Heart rate monitoring using a pulse sensor
* BMI calculation based on measured height and weight
* Real-time data display on a 16x2 LCD
* Audio feedback using MP3-TF-16P module and speaker
* Button-based control for measurement stages

## Components Used

* Arduino Nano
* VL53L0X Distance Sensor
* HX711 Load Cell Amplifier with Load Cell
* DS18B20 Temperature Sensor
* Pulse Sensor
* 16x2 LCD with I2C module
* MP3-TF-16P Audio Module
* Push Button
* Speaker
* Breadboard and jumper wires

## Working

The system first measures the user's height using the VL53L0X distance sensor. Once the height is captured, the user stands on the load cell platform to measure weight. The system also records body temperature and pulse rate. Using the measured height and weight, BMI is calculated and the results are displayed on the LCD. Audio prompts guide the user through the measurement process.

## Applications

* Basic health monitoring systems
* Smart health kiosks
* Educational and prototype healthcare projects
