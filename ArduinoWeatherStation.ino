#include "DHT.h" // By Adafruit
#include <IRremote.h> // By Shirriff
#include <LiquidCrystal.h> // By Arduino

// Pin variables
const int receiver_pin = 4;
const int dht11_pin = 2;
const int photo_resistor_pin = A0;
const int sample_delay = 30 * 60 * 1000; // 1800000 milliseconds or 30 minutes
// Initialize LiquidCrystal with the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Initialize IR remote receiver with the interface pin
IRrecv irrecv(receiver_pin);
decode_results results;

// Weather station variables
int current_temp = 0;
int current_hum = 0;
int current_brightness = 0;
int current_button = 16724175;
int current_weather_type = 1;
int current_weather_value = 4;
int sample_temp[12] = {};
int sample_hum[12] = {};
int sample_brightness[12] = {};
int current_sample_iteration = 0;
unsigned long last_sample;
boolean button_change = true;
DHT dht = DHT(dht11_pin, DHT11);

void setup() {
  // Start the IR receiver
  irrecv.enableIRIn();
  // Setup the LCD's columns and rows
  lcd.begin(16, 2);
  // Set default sample update. Note: we must wait a bit otherwise 
  // we get values of zero so we need to let the sensors prepare.
  last_sample = -sample_delay + 2000;
}

void loop() {
  // When the allotted time has expired
  if(millis() - last_sample > sample_delay) {
    // Sample the temperature, humidity, and brightness
    int temp = read_temperature(current_temp);
    int hum = read_humidity(current_hum);
    int brightness = read_brightness();
    
    // When we have 12 or more samples
    if(current_sample_iteration >= 11) {
      // Shift all samples by 1 (effectively replacing samples)
      for(int i = 0; i < current_sample_iteration-1; i++) {
        sample_hum[i] = sample_hum[i+1];
        sample_temp[i] = sample_temp[i+1];
        sample_brightness[i] = sample_brightness[i+1];
      }
    } else { // Otherwise, we need more samples
      // Add sample to sampling set
      sample_hum[current_sample_iteration] = hum;
      sample_temp[current_sample_iteration] = temp;
      sample_brightness[current_sample_iteration] = brightness;
      // Until we have 12 samples, we need to keep track of how many we have
      current_sample_iteration++;
    }
    
    // Update the last sample so we have to wait for the next sampling
    last_sample = millis();
  }
  
  // Get current temperature, humidity, and brightness
  current_temp = read_temperature(current_temp);
  current_hum = read_humidity(current_hum);
  current_brightness = read_brightness();

  // When we get a valid IR remote result
  if (irrecv.decode(&results)) {
    // Save the current button for later
    current_button = results.value;
    // Wait to prevent repeat reads
    delay(200);
    // A new option might have been selected
    button_change = true;
    // Get ready to receive the next value
    irrecv.resume();
  }

  // Update selected options
  update_weather_options();
  // Update the options displayed
  update_lcd_display();

  delay(100);
}

// Read and return temperature
int read_temperature(int temp) {
  // When we read a valid temperature
  if(!isnan(dht.readTemperature())) {
    // Set the new temperature
    temp = (int)dht.readTemperature();
  }
  return temp;
}

// Read and return humidity
int read_humidity(int hum) {
  // When we read a valid humidity
  if(!isnan(dht.readHumidity())) {
    // Set the new humidity
    hum = (int)dht.readHumidity();
  }
  return hum;
}

// Read and return brightness
int read_brightness() {
  // Get brightness
  int brightness = analogRead(photo_resistor_pin);
  // Map from a resisted analog value to a percentage
  brightness = map(brightness, 0, 600, 0, 100);
  return brightness;
}

// Set the weather options
void update_weather_options() {
  if(button_change) {
    // Map the IR button press to a type and value
    switch (current_button) {
      case 16724175: // Button 1
        current_weather_type = 1;
        break;
      case 16718055: // Button 2
        current_weather_type = 2;
        break;
      case 16743045: // Button 3
        current_weather_type = 3;
        break;
      case 16716015: // Button 4
        current_weather_value = 4;
        break;
      case 16726215: // Button 5
        current_weather_value = 5;
        break;
      case 16734885: // Button 6
        current_weather_value = 6;
        break;
      case 16728765: // Button 7
        current_weather_value = 7;
        break;
      default:
       // Invalid button pressed (zeros denote error)
       current_weather_type = 0;
       current_weather_value = 0;
       break;
    }
    // Prevent needlessly re-running this switch statement
    button_change = false;
  }
}

// Update the LCD screen to match the selected options
void update_lcd_display() {
    // Clear the LCD screen
    lcd.clear();
    // Set lcd to row 0 for weather type
    lcd.setCursor(0, 0);

    // When we have one invalid button pressed
    if(current_weather_type == 0 && current_weather_value == 0) {
      // Inform user of their error
      lcd.print("Error: select");
      lcd.setCursor(0, 1);
      lcd.print("valid button");
      return;
    // When only the weather type is undefined
    } else if(current_weather_type == 0 && current_weather_value != 0) {
      // Provide a default weather type (humidity)
      current_weather_type = 1;
    // When only the weather value is undefined
    } else if(current_weather_type != 0 && current_weather_value == 0) {
      // Provide a default weather value (current value)
      current_weather_value = 4;
    }

    // Determine the type of weather that will be displayed
    switch(current_weather_type) {
      case 1:
        lcd.print("Humidity");
        break;
      case 2:
        lcd.print("Temperature");
        break;
      case 3:
        lcd.print("Brightness");
        break;
    }

    // Set lcd to row 1 for weather value
    lcd.setCursor(0, 1);

    // Determine the value of the weather that will be displayed
    switch(current_weather_value) {
      case 4: // display current
        lcd.print("Current: ");
        if(current_weather_type == 1) {
          lcd.print(current_hum);
        } else if(current_weather_type == 2) {
          lcd.print(current_temp);
        } else if(current_weather_type == 3) {
          lcd.print(current_brightness);
        }
        break;
      case 5: // display average
        lcd.print("Average: ");
        if(current_weather_type == 1) {
          lcd.print(get_average(sample_hum));
        } else if(current_weather_type == 2) {
          lcd.print(get_average(sample_temp));
        } else if(current_weather_type == 3) {
          lcd.print(get_average(sample_brightness));
        }
        break;
      case 6: // display low
        lcd.print("Low: ");
        if(current_weather_type == 1) {
          lcd.print(get_low(sample_hum));
        } else if(current_weather_type == 2) {
          lcd.print(get_low(sample_temp));
        } else if(current_weather_type == 3) {
          lcd.print(get_low(sample_brightness));
        }
        break;
      case 7: // display high
        lcd.print("High: ");
        if(current_weather_type == 1) {
          lcd.print(get_high(sample_hum));
        } else if(current_weather_type == 2) {
          lcd.print(get_high(sample_temp));
        } else if(current_weather_type == 3) {
          lcd.print(get_high(sample_brightness));
        }
        break;
    }

    // When the weather type is humidity or brightness
    if(current_weather_type == 1 || current_weather_type == 3) {
      lcd.print("%");
    // When the weather type is temperature
    } else if(current_weather_type == 2) {
      lcd.print("C");
    }
}

// Get and return the average of the aquired sampling
int get_average(int array[]) {
  int sum = 0;
  // Get the sum of the sampling
  for(int i = 0; i < current_sample_iteration; i++) {
    sum += array[i];
  }

  // When we have at least one sample
  if(current_sample_iteration > 0) {
    return sum/current_sample_iteration;
  } else { // Otherwise
    // Prevent division by zero and return the current sum
    return sum;
  }
}

// Get and return the lowest of the aquired sampling
int get_low(int array[]) {
  int lowest = array[0];
  for(int i = 0; i < current_sample_iteration; i++) {
    // When we get a new lowest sample
    if(lowest > array[i]) {
      // Update the lowest sample number
      lowest = array[i];
    }
  }
  return lowest;
}

// Get and return the highest of the aquired sampling
int get_high(int array[]) {
  int highest = array[0];
  for(int i = 0; i < current_sample_iteration; i++) {
    // When we get a new highest sample
    if(highest < array[i]) {
      // Update the highest sample number
      highest = array[i];
    }
  }
  return highest;
}


