/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/micro/examples/hello_world/output_handler.h"

#include "LCD_DISCO_F769NI.h"
#include "tensorflow/lite/micro/examples/hello_world/constants.h"

// The LCD driver
LCD_DISCO_F769NI lcd;

DigitalOut led_red(LED1);
DigitalOut led_green(LED2);

InterruptIn button1(USER_BUTTON);
volatile bool button1_pressed = false;  // Used in the main loop
volatile bool button1_enabled = true;   // Used for debouncing
Timeout button1_timeout;                // Used for debouncing

// The colors we'll draw
const uint32_t background_color = LCD_COLOR_LIGHTYELLOW;
const uint32_t foreground_color = LCD_COLOR_DARKBLUE;
// The size of the dot we'll draw
const int dot_radius = 10;
// Track whether the function has run at least once
bool initialized = false;
// Size of the drawable area
int width;
int height;
// Midpoint of the y axis
int midpoint;
// Pixels per unit of x_value
int x_increment;

// Enables button when bouncing is over
void button1_enabled_cb(void) { button1_enabled = true; }

// ISR handling button pressed event
void button1_onpressed_cb(void) {
  if (button1_enabled) {  // Disabled while the button is bouncing
    button1_enabled = false;
    button1_pressed = true;  // To be read by the main loop
    button1_timeout.attach(callback(button1_enabled_cb),
                           0.3);  // Debounce time 300 ms
  }
}

// Animates a dot across the screen to represent the current x and y values
void HandleOutput(tflite::ErrorReporter* error_reporter, float x_value,
                  float y_value) {
  // Do this only once
  if (!initialized) {
    // Set the background and foreground colors
    lcd.Clear(background_color);
    lcd.SetTextColor(foreground_color);
    // Calculate the drawable area to avoid drawing off the edges
    width = lcd.GetXSize() - (dot_radius * 2);
    height = lcd.GetYSize() - (dot_radius * 2);
    // Calculate the y axis midpoint
    midpoint = height / 2;
    // Calculate fractional pixels per unit of x_value
    x_increment = static_cast<float>(width) / kXrange;
    initialized = true;

    TF_LITE_REPORT_ERROR(error_reporter, "lcd x size: %d, lcd y size: %d\n",
                         lcd.GetXSize(), lcd.GetYSize());

    lcd.DrawRect(0, 0, lcd.GetXSize(), lcd.GetYSize());

    // button1.mode(PullUp); // Activate pull-up
    button1.fall(callback(
        button1_onpressed_cb));  // Attach ISR to handle button press event
  }

  // Log the current X and Y values
  TF_LITE_REPORT_ERROR(error_reporter, "x_value: %f, y_value: %f\n", x_value,
                       y_value);

  // Clear the previous drawing
  if (x_value == 0)
    ;//lcd.Clear(background_color);

  // Calculate x position, ensuring the dot is not partially offscreen,
  // which causes artifacts and crashes
  int x_pos = (dot_radius * 2) + static_cast<int>(x_value * x_increment);

  // Calculate y position, ensuring the dot is not partially offscreen
  int y_pos;
  if (y_value >= 0) {
    // Since the display's y runs from the top down, invert y_value
    y_pos = dot_radius + static_cast<int>(midpoint * (1.f - y_value));
  } else {
    // For any negative y_value, start drawing from the midpoint
    y_pos = dot_radius + midpoint + static_cast<int>(midpoint * (0.f - y_value));
  }

  if (y_pos < dot_radius * 2) {
    y_pos = dot_radius * 2;
  }
  else if (y_pos > height) {
    y_pos = height;
    led_green = !led_green;
  }
    
  // Draw the dot
  lcd.FillCircle(x_pos, y_pos, dot_radius);

  // Set when button is pressed
  if (button1_pressed) {  
    button1_pressed = false;
    printf("Button pressed\n");
    led_red = !led_red;
  }

}
