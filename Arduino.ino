
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>



Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define size 6

int totalDevices = 0;
bool displayingStudentID = false;
bool displayNeedsUpdate = true;
int currentDevice = 0;


// -----------------------
// FREERAM EXTENSION
// -----------------------

extern unsigned int __heap_start;
extern void *__brkval;

/*
 * The free list structure as maintained by the
 * avr-libc memory allocation routines.
 */
struct __freelist
{
  size_t sz;
  struct __freelist *nx;
};

/* The head of the free list structure */
extern struct __freelist *__flp;



/* Calculates the size of the free list */
int freeListSize()
{
  struct __freelist* current;
  int total = 0;
  for (current = __flp; current; current = current->nx)
  {
    total += 2; /* Add two bytes for the memory block's header  */
    total += (int) current->sz;
  }

  return total;
}

int freeMemory()
{
  int free_memory;
  if ((int)__brkval == 0)
  {
    free_memory = ((int)&free_memory) - ((int)&__heap_start);
  }
  else
  {
    free_memory = ((int)&free_memory) - ((int)__brkval);
    free_memory += freeListSize();
  }
  return free_memory;
}

// -----------------
// UDCHARS EXTENSION
// -----------------

byte upArrow[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
  0b00000
};

byte downArrow[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100
};

// -----------------------
// STRUCTS AND ENUMERATIONS
// -----------------------

struct Device {
  String id;
  char type;
  String location;
  String state;
  int outputPower;
  int temp;
  
};

//The array where all the devices will be added to
Device devices[size];

enum State {
    Synchronization,
    Main,
    HciRightButton,
    HciLeftButton,
    
};
State currentState = Synchronization;






// -------------------
// HANDLE SERIAL INPUTS
// -------------------


// This function handles the sorting of the array of devices
void sortDevices() {
  int i, j;
  Device temp;
  for (i = 0; i < totalDevices-1; i++) {     
    for (j = 0; j < totalDevices-i-1; j++) { 
      if (devices[j].id > devices[j+1].id) {
        temp = devices[j];
        devices[j] = devices[j+1];
        devices[j+1] = temp;
      }
    }
  }
}
// This function handles the addition of a new device to the array of devices
void handleAddDevice(const String& input) {
  // Count the number of '-' characters in the input
  int dashCount = 0;
  for (char c : input) {
    if (c == '-') {
      ++dashCount;
    }
  }

  // Ensure there are exactly 3 dashes
  if (dashCount != 3) {
    Serial.print("Error: Invalid format.");
    return;
  }

  // find the position of the '-' character in the input string
  int firstDash = input.indexOf('-');
  int secondDash = input.indexOf('-', firstDash + 1);
  int thirdDash = input.indexOf('-', secondDash + 1);

  // extract the device id, type, and location from the input string
  String id = input.substring(firstDash + 1, secondDash);
  char type = input.charAt(secondDash + 1);
  String location = input.substring(thirdDash + 1);

  // Add device ID validation here
  if (id.length() != 3) {
    Serial.print("Error: Device ID should be exactly 3 characters.");
    return;
  }

  for (int i = 0; i < 3; ++i) {
    char c = id.charAt(i);
    if (!(c >= 'A' && c <= 'Z')) {
      Serial.print("Error: Device ID should contain only uppercase letters A-Z.");
      return;
    }
  }

  // If the location string is longer than 15 characters, truncate it
  if (location.length() > 15) {
    location = location.substring(0, 15);
  }

  for (char c : location) {
    if (!isAlphaNumeric(c)) {
      Serial.println("Error: Location should only contain alphanumeric characters (A-Z and 0-9) with no spaces.");
      return;
    }
  }

  // Check if any of the parts are empty
  if (id.length() == 0 || location.length() == 0) {
    Serial.print("Error: Invalid format.");
    return;
  }

  // Check if type is valid
  if (type != 'S' && type != 'T' && type != 'O' && type != 'L' && type != 'C') {
    Serial.print("Error: Invalid device type.");
    return;
  }

  int deviceIndex = -1;
  for (int i = 0; i < totalDevices; ++i) {
    if (devices[i].id == id) {
      deviceIndex = i;
      break;
    }
  }

  if (deviceIndex == -1) { // If device not already exists
    for (int i = 0; i < size; ++i) {
      if (devices[i].id == "") {
        deviceIndex = i;
        totalDevices++;
        break;
      }
    }
  }

  if (deviceIndex != -1) {
    devices[deviceIndex].id = id;
    devices[deviceIndex].type = type;
    devices[deviceIndex].location = location;
    devices[deviceIndex].state = "OFF"; 
    devices[deviceIndex].outputPower = 0;
    devices[deviceIndex].temp = 9;

    Serial.print("Device added ");

    displayNeedsUpdate = true;
  }

  sortDevices();
}
void handleUpdateState(const String& input) {
  
  // Find the position of the '-' character in the input string.
  // This position is used to split the input into the device id and the new state.
  int dash = input.indexOf('-');
  String deviceId = input.substring(0, dash);
  String newState = input.substring(dash + 1);



  if (newState == "ON" || newState == "OFF") {
    for (int i = 0; i < size; ++i) {
      if (devices[i].id == deviceId) {
        devices[i].state = newState;
        Serial.print("Device state updated ");
        
        
       
        
      }
    }
  } else {
    
    Serial.print("Error: Invalid state. State must be either 'ON' or 'OFF'.");
  }
 

  displayNeedsUpdate = true;
 
}
// This function handles the updating of the Output power
void handleUpdateOutputPower(const String& input) {
  // Find the index of the first '-' character
  int dash = input.indexOf('-');
  
  // Extracts the device ID from the beginning of the string up to the first dash
  String deviceId = input.substring(0, dash);
  
  // Extract the new output power value from the string following the first dash
  String newOutputPowerStr = input.substring(dash + 1);

  // Check if the string is not empty and can be converted to an integer
  if (newOutputPowerStr.length() > 0 && isDigit(newOutputPowerStr[0])) {
    int newOutputPower = newOutputPowerStr.toInt();

    for (int i = 0; i < size; ++i) {
      if (devices[i].id == deviceId) {
        switch (devices[i].type) {
          case 'S': // If the device is a Speaker
            // Update the speaker's output power with the new value
            updateSpeakerOutputPower(i, newOutputPower);
            break;
          case 'L': // If the device is a Light
            // Update the light's output power with the new value
            updateLightOutputPower(i, newOutputPower);
            break;
          case 'T': // If the device is a Thermostat
            // Update the thermostat's temperature with the new value
            updateThermostatTemperature(i, newOutputPower);
            break;
          case 'O':
            Serial.print("Error: Socket devices does not support power output");
            break;
          case 'C':
            Serial.print("Error: Camera devices does not support power output");
            break;

    
          default:
            
            break;
        }
        break; 
      }
    }
  } else {
    
    Serial.print("Error: No value passed for output power.");
  }

  displayNeedsUpdate = true;
}
// This function handles the removing of a device 
void handleRemoveDevice(const String& input) {
  
  String deviceId = input;
  bool removedCurrentDevice = false;

  
  for (int i = 0; i < size; ++i) {
    
    if (devices[i].id == deviceId) {
      
      
      
      // If the device being removed is the current device, set a flag
      if (i == currentDevice) {
        removedCurrentDevice = true;
      }
      
      // Clear the data for the removed device
      devices[i].id = "";
      devices[i].type = '\0';
      devices[i].location = "";
      devices[i].state = "";
      devices[i].outputPower = 0;
      devices[i].temp = 0;
      
      
      break;
    }
  }
  
  
  totalDevices--;

  
  // If the removed device was the current device, adjust the current device index if necessary
  if (removedCurrentDevice) {
    if (currentDevice > 0) {
      currentDevice--;
    } else if (currentDevice < totalDevices - 1) {
      currentDevice++;
    }
  }
 
  
  updateCurrentDevice();
  
  
  displayNeedsUpdate = true;
}


// ----------------
// DEVICE OPERATIONS
// ----------------

//This function takes two parameters then updates the output power of the speaker device at the specified index in the devices array
void updateSpeakerOutputPower(int deviceIndex, int newOutputPower) {
  
  if (newOutputPower >= 0 && newOutputPower <= 100) {
    // Update the output power of the device at the given index
    devices[deviceIndex].outputPower = newOutputPower;
    
    
    Serial.print("Device output power updated");
    
   
    
    
    
  }else {
    // Handle invalid temperature error
    Serial.print("Error: Invalid Input. Must be within the range 0 - 100.");
  }
  
  
  displayNeedsUpdate = true;
}
//This function takes two parameters then updates the output power of the light device at the specified index in the devices array
void updateLightOutputPower(int deviceIndex, int newOutputPower) {
  
  if (newOutputPower >= 0 && newOutputPower <= 100) {
    // Update the output power of the device at the given index
    devices[deviceIndex].outputPower = newOutputPower;
    
    
    Serial.print("Device output power updated ");
    
    
    
    
  }else {
    // Handle invalid temperature error
    Serial.print("Error: Invalid Input. Must be within the range 0 - 100.");
  }
  
  
  displayNeedsUpdate = true;
}
//This function takes two parameters then updates the output power of the Thermostat device at the specified index in the devices array
void updateThermostatTemperature(int deviceIndex, int newTemperature) {
  // If the new temperature is within the valid range (9 to 32)
  if (newTemperature >= 9 && newTemperature <= 32) {
    // Update the temperature of the device at the given index
    devices[deviceIndex].temp = newTemperature;
    
    
    Serial.print("Device temperature updated  ");
    
    
    
    
    
    
  }else {
    // Handle invalid temperature error
    Serial.print("Error: Invalid temperature. Temperature must be within the range 9 - 32.");
  }
  
  
  displayNeedsUpdate = true;
}


// ---------
// UI HELPERS
// ---------
// Handle scrolling for device location if it's longer than 11 characters.
void scrolling() {
  static int count = 0;
  static unsigned long lastChecked = 0;
  unsigned long currentMillis = millis();

  if (devices[currentDevice].location.length() > 11) {
    if (currentMillis - lastChecked >= 500) {
      String scrollLocation = devices[currentDevice].location.substring(count % devices[currentDevice].location.length(), (count % devices[currentDevice].location.length()) + 11);
      
      int scrollLength = scrollLocation.length();
      if (scrollLength < 11) {
        // Add the first part of the location string to the end if it's shorter than 11 characters
        scrollLocation += " " + devices[currentDevice].location.substring(0, 11 - scrollLength);
      }

      lcd.setCursor(5, 0);
      lcd.print(scrollLocation);

      for (int i = 0; i < (11 - scrollLength); i++) {
        lcd.print(" ");
      }

      count++;
      lastChecked = currentMillis;
    }
  } else {
    lcd.setCursor(5, 0);
    lcd.print(devices[currentDevice].location);
  }
}
// Function to display the location of a device on the LCD screen
void displayLocation() {
  bool deviceFound = false;

  for (int i = currentDevice; i < size; ++i) {
    if (devices[i].state == "OFF") {
      deviceFound = true;
      currentDevice = i;
      break;
    }else if (devices[i].state == "ON") {
      deviceFound = true; // Set the flag to true if we find an off device
      currentDevice = i;
      break;
    }
  }


  if (!displayingStudentID ) {
    if (devices[currentDevice].location.length() > 11){
      scrolling(); // If location is longer than 11 characters, use the scrolling function
    } else {
      lcd.setCursor(5, 0);
      lcd.print(devices[currentDevice].location.substring(0, 11)); // Display only the first 11 characters of the location
      for (int i = 0; i < (11 - devices[currentDevice].location.length()); i++) {
        lcd.print(" "); // Clear remaining spaces
      }
    }
  }
}
void displayLocationHciLeft() {
  bool deviceFound = false; // Add a flag to track whether we've found any off devices

  // Loop to check if there are any "OFF" devices
  for (int i = currentDevice; i < size; ++i) {
    if (devices[i].state == "OFF") {
      deviceFound = true; // Set the flag to true if we find an off device
      currentDevice = i;
      break;
    }
  }

  // If we didn't find any off devices, return immediately
  if (!deviceFound) {
    return;
  }

  if (!displayingStudentID) {
    if (devices[currentDevice].location.length() > 11){
      scrolling(); // If location is longer than 11 characters, use the scrolling function
    } else {
      lcd.setCursor(5, 0);
      lcd.print(devices[currentDevice].location.substring(0, 11)); // Display only the first 11 characters of the location
      for (int i = 0; i < (11 - devices[currentDevice].location.length()); i++) {
        lcd.print(" "); // Clear remaining spaces
      }
    }
  }
  
}
void displayLocationHciRight() {
  bool deviceFound = false; // Add a flag to track whether we've found any off devices

  // Loop to check if there are any "OFF" devices
  for (int i = currentDevice; i < size; ++i) {
    if (devices[i].state == "ON") {
      deviceFound = true; // Set the flag to true if we find an off device
      currentDevice = i;
      break;
    }
  }

  // If we didn't find any off devices, return immediately
  if (!deviceFound) {
    return;
  }

  if (!displayingStudentID) {
    if (devices[currentDevice].location.length() > 11){
      scrolling(); // If location is longer than 11 characters, use the scrolling function
    } else {
      lcd.setCursor(5, 0);
      lcd.print(devices[currentDevice].location.substring(0, 11)); // Display only the first 11 characters of the location
      for (int i = 0; i < (11 - devices[currentDevice].location.length()); i++) {
        lcd.print(" "); // Clear remaining spaces
      }
    }
  }
  
  // The rest of your function here...
}
// Function to display devices that are "OFF".
void displayOffDevices() {
  bool deviceFound = false;
  // Loop to search for "OFF" devices from current device to end of list
  for (int i = currentDevice; i < size; ++i) {
    if (devices[i].state == "OFF") {
      deviceFound = true;
      currentDevice = i;
      break;
    }
  }

  // If no "OFF" device is found in above loop, search from start of list to current device
  if (!deviceFound) {
    for (int i = 0; i < currentDevice; ++i) {
      if (devices[i].state == "OFF") {
        deviceFound = true;
        currentDevice = i;
        break;
      }
    }
  }

  // If no "OFF" device is found in either loop, display "NOTHING'S OFF"
  if (!deviceFound) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NOTHING'S OFF");
  } else {
 
    displayDeviceList();
    
  }
}
// Similar to displayOffDevices, but for devices that are "ON".
void displayOnDevices() {
  
  bool deviceFound = false;
  for (int i = currentDevice; i < size; ++i) {
    if (devices[i].state == "ON") {
      deviceFound = true;
      currentDevice = i;
      break;
    }
  }

  if (!deviceFound) {
    for (int i = 0; i < currentDevice; ++i) {
      if (devices[i].state == "ON") {
        deviceFound = true;
        currentDevice = i;
        break;
      }
    }
  }

  
  if (!deviceFound) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NOTHING'S ON");
  } else {
    displayDeviceList();
  }
}
// Update current device index if the current device has been removed.
void updateCurrentDevice() {
  while (currentDevice > 0 && devices[currentDevice].id == "") {
    currentDevice--;
  }
}
// Update the LCD backlight based on the state of the current device.
void updateBacklight() {
  if (devices[currentDevice].state == "OFF") {
    lcd.setBacklight(3); // Yellow
  } else if (devices[currentDevice].state == "ON") {
    lcd.setBacklight(2); // Green
  } else {
    lcd.setBacklight(7); // White (default)
  }
}
// Function to display the list of devices on the LCD screen
void displayDeviceList() {
  if (!displayingStudentID) {
  
    if (displayingStudentID) {
      return; // Do not update the display if the student ID is being shown
    }
    if (currentDevice < 0 || currentDevice >= size) return;
    if (!displayNeedsUpdate) {
      return; // Do not update the display if there is no need
    }
    displayNeedsUpdate = false;
    

    Device device = devices[currentDevice];

    if (device.id == "") {
      // If the device has an empty ID, it is not a valid device, so return without displaying anything
      lcd.clear();
      return;
    }

    lcd.clear();
    // First row: ID and Location
    lcd.setCursor(0, 0);
    if (currentDevice > 0) {
      lcd.write(byte(0));
    }
      
    lcd.setCursor(1, 0); 
    lcd.print(device.id); 
    

    // Second row: Type, State, Output Power or Temperature
    lcd.setCursor(0, 1);
    if (currentDevice < totalDevices - 1) {
      lcd.write(byte(1));
    }

    lcd.setCursor(1, 1);
    lcd.print(device.type);
    lcd.print(" ");
    lcd.print(device.state);
    if (device.type == 'S' || device.type == 'L') {
      lcd.print(" ");
      lcd.print(device.outputPower);
      lcd.print("%");
    } else if (device.type == 'T') {
      lcd.print(" ");
      if (device.temp < 10) {
        lcd.print("");
      }
      lcd.print(device.temp);
      lcd.print((char)223);
      lcd.print("C");
    }
    for (int i = 0; i < (5 - device.state.length()); i++) {
      lcd.print(" "); // Clear remaining spaces
    }
    updateBacklight();
  }
  
}
// Function to handle the pressing of the select button on the LCD screen
void handleSelectButton() {
   Serial.println("Entering handleSelectButton function");
  if (!displayingStudentID) {
    displayingStudentID = true;
    delay(1000);
    lcd.clear();
    lcd.setBacklight(5);
    lcd.setCursor(0, 0);
    lcd.print("F213826");
    lcd.setCursor(0, 1);
    lcd.print("SRAM LEFT:");
    lcd.print(freeMemory());
  }
}
// Function to handle the pressing of the up button on the LCD screen
void handleUpButton() {
  if (currentDevice > 0 && totalDevices > 1) {
    currentDevice--;
    displayNeedsUpdate = true;
  }
}
// Function to handle the pressing of the down button on the LCD screen
void handleDownButton() {
  if (currentDevice < totalDevices - 1) {
    currentDevice++;
    displayNeedsUpdate = true;
  }
}
void handleHciRightButton() {
  // Increase currentDevice index
  currentDevice++;

  // If currentDevice index is more than the last index in the array, reset it to 0
  if (currentDevice >= totalDevices) {
    currentDevice = 0;
  }

  // Set displayNeedsUpdate to true so the display will be updated
  displayNeedsUpdate = true;

  // Change the state to Main
  currentState = Main;
}
void handleHciLeftButton() {
  // Decrease currentDevice index
  currentDevice--;

  // If currentDevice index is less than 0, set it to the last index in the array
  if (currentDevice < 0) {
    currentDevice = totalDevices - 1;
  }
  
  // Set displayNeedsUpdate to true so the display will be updated
  displayNeedsUpdate = true;

  // Change the state to Main
  currentState = Main;
}
// Function to handle the pressing of the right button on the LCD screen
void handleRightButton() {
  currentState = HciRightButton;
  displayNeedsUpdate = true;
 
}
// Function to handle the pressing of the left button on the LCD screen
void handleLeftButton() {
  currentState = HciLeftButton;
  displayNeedsUpdate = true;
}
// Function to handle returning to the main screen on the LCD screen
void handleReturnToMain() {
  currentState = Main;
  displayNeedsUpdate = true;
 
}
// Function to update the state based on the button presses on the LCD screen
void updateStateFromButtons() {
  static unsigned long lastButtonPressTime = 0;
  unsigned long currentTime = millis();
  static bool rightButtonPreviouslyReleased = true;
  static bool leftButtonPreviouslyReleased = true;

  if (currentTime - lastButtonPressTime < 50) {
    // Debounce delay - ignore button presses within 50 ms of the last press
    return;
  }

  uint8_t buttons = lcd.readButtons();

  if (buttons & BUTTON_SELECT) {
    handleSelectButton();
  } else if (displayingStudentID) {
    displayingStudentID = false;
    displayNeedsUpdate = true;
    
  }
  
 

  // Handling button presses based on the current state
  if (currentState == Main) {
    if (buttons & BUTTON_UP) {
      handleUpButton();
    }
    if (buttons & BUTTON_DOWN) {
      handleDownButton();
    }
    if (buttons & BUTTON_RIGHT) {
      rightButtonPreviouslyReleased = false;
    } else {
      if (!rightButtonPreviouslyReleased) {
        rightButtonPreviouslyReleased = true;
        handleRightButton();
      }
    }
    if (buttons & BUTTON_LEFT) {
      leftButtonPreviouslyReleased = false;
    } else {
      if (!leftButtonPreviouslyReleased) {
        leftButtonPreviouslyReleased = true;
        handleLeftButton();
      }
    }
  } else if (currentState == HciRightButton) {
    if (buttons & BUTTON_RIGHT) {
      rightButtonPreviouslyReleased = false;
    } else {
      if (!rightButtonPreviouslyReleased) {
        rightButtonPreviouslyReleased = true;
        handleReturnToMain();
      }
    }
  } else if (currentState == HciLeftButton) {
    
    if (buttons & BUTTON_LEFT) {
      leftButtonPreviouslyReleased = false;
    } else {
      if (!leftButtonPreviouslyReleased) {
        leftButtonPreviouslyReleased = true;
        handleReturnToMain();
      }
    }
  }
   
  lastButtonPressTime = currentTime;
}


// ---------
// ARDUINO SETUP AND LOOP
// ---------

void setup(){
  
  Serial.begin(9600);
  

}

void loop() {
  unsigned long lastButtonPress = 0;  // Variable to store the timestamp of the last button press
  const unsigned long debounceDelay = 200; // 200 milliseconds delay for button debounce

  unsigned long currentTime = millis(); // Fetch current time
  updateStateFromButtons(); // Update the state based on the pressed buttons
  uint8_t buttonsRight = lcd.readButtons(); // Read the right button
  uint8_t buttonsLeft = lcd.readButtons();  // Read the left button
   

  switch (currentState) {
    case Synchronization: // Synchronization state
      lcd.begin(16, 2); // Initialize LCD with 16 columns and 2 rows
      lcd.createChar(0, upArrow); // Create up arrow character   
      lcd.createChar(1, downArrow); // Create down arrow character
  
      

      char receivedChar; // Variable to store received character from serial communication
      do {
        receivedChar = Serial.read(); // Read character from serial
        lcd.setBacklight(5); // Set LCD backlight to purple
        Serial.print('Q'); // Send 'Q' to serial
        delay(1000); // Wait for 1 second
      } while (receivedChar != 'X' && receivedChar != 'x'); // Continue loop until 'X' or 'x' is received

      lcd.setBacklight(7); // Set LCD backlight to white
      Serial.print(" FREERAM, UDCHARS, HCI, SCROLL"); 

      // Transition to the Main state after completing the Synchronization phase
      currentState = Main;
      
      break;
       

    case Main: // Main state
      displayLocation();
     
     
      if (Serial.available() > 0) { // Check if there's data available on the serial port
        String input = Serial.readStringUntil('\n'); // Read the incoming data until newline
        char command = input[0]; // Extract the command
        String parameters = input.substring(1); // Extract the parameters
        

        switch (command) { // Execute corresponding function based on the command
          case 'A':
            handleAddDevice(parameters);
            break;
          case 'S':
           handleUpdateState(input.substring(2));
           break;
          case 'P':
           handleUpdateOutputPower(input.substring(2));
           break;
          case 'R':
            handleRemoveDevice(input.substring(2));
            break;
          default:
           break;
        }
      }
      
      // Display the list of devices 
      displayDeviceList();
       
     
      
      
      
      break;
    case HciRightButton: // Right button state
      displayLocationHciRight();
      
      
      if (displayNeedsUpdate) { // If the display needs an update
        lcd.clear(); // Clear the display
        currentDevice = 0; // Reset currentDevice to 0
        displayOnDevices(); // Display devices that are on
        displayNeedsUpdate = false;
      }

      // Handle the right button with debouncing
      if (currentTime - lastButtonPress > debounceDelay) {
        if (buttonsRight & BUTTON_UP) {
          handleUpButton();
          lastButtonPress = currentTime;
          displayOnDevices();
        } else if (buttonsRight & BUTTON_DOWN) {
          handleDownButton();
          lastButtonPress = currentTime;
          displayOnDevices();
        }
      }
      
      break;

    case HciLeftButton: // Left button state
      displayLocationHciLeft();
      
      if (displayNeedsUpdate) { // If the display needs an update
        
        lcd.clear(); // Clear the display
        currentDevice = 0; // Reset currentDevice to 0
        
        displayOffDevices(); // Display devices that are off
        
        displayNeedsUpdate = false;
      }

      // Handle the left button with debouncing
      if (currentTime - lastButtonPress > debounceDelay) {
        if (buttonsLeft & BUTTON_UP) { 
          handleUpButton(); // Handle the up button press
          lastButtonPress = currentTime; // Update the time of the last button press
          displayOffDevices(); // Display devices that are off
          
        } else if (buttonsLeft & BUTTON_DOWN) { 
          handleDownButton(); // Handle the down button press
          lastButtonPress = currentTime; // Update the time of the last button press
          displayOffDevices(); // Display devices that are off
          
        }
      }
      
      break;
  

      
      
     

  }
}
