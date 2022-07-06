
#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).

#define HEATING_PIN 4
#define LED_PIN     5
#define COOLING_PIN 6

#define TEMP_PIN    A0

#define BLINK_SLOW  30
#define BLINK_MID   20
#define BLINK_FAST  10

#define DISP_STATUS_COOL "COLD"
#define DISP_STATUS_MID  "Normal"
#define DISP_STATUS_HOT  "Alert"

#define COOLING_RESPONSE 20
#define READING_REFRESH_TIME 10
#define DISP_REFRESH_TIME 50

#define DATA_COLUMN 7
#define ADC_RESOLUTION 1023
#define MAP_MV_MAX_RANGE 5000

#define TEMP 0
#define FAN 1
#define HEAT 2
#define LED 3

#define TEMP_LOW 25
#define TEMP_HIGH 45

uint16_t uint16BlinkSpeed = BLINK_SLOW;
String msg2disp[4];
bool cool = true;
bool heat = false;

// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;

//LiquidCrystal_I2C lcd(0x20, 20, 4);

// define one Task for AnalogRead
void TaskAnalogRead( void *pvParameters );
void TaskLedBlink  ( void *pvParameters );
void TaskCooling   ( void *pvParameters );
void TaskHeating   ( void *pvParameters );
void TaskDisplaying( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(COOLING_PIN, OUTPUT);
  pinMode(HEATING_PIN, OUTPUT);

 /*
     Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
     because it is sharing a resource, such as the Serial port.
     Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  */
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }

  xTaskCreate(
    TaskAnalogRead
    ,  "AnalogRead" // A name just for humans
    ,  128  // Stack size
    ,  NULL //Parameters for the task
    ,  2  // Priority
    ,  NULL
  ); //Task Handle

  xTaskCreate(
    TaskLedBlink
    ,  "LedBlink" // A name just for humans
    ,  128  // Stack size
    ,  NULL //Parameters for the task
    ,  4  // Priority
    ,  NULL
  ); //Task Handle
  xTaskCreate(
    TaskCooling
    ,  "Cooling" // A name just for humans
    ,  128  // Stack size
    ,  NULL //Parameters for the task
    ,  1  // Priority
    ,  NULL
  ); //Task Handle

  xTaskCreate(
    TaskDisplaying
    ,  "Displaying" // A name just for humans
    ,  128  // Stack size
    ,  NULL //Parameters for the task
    ,  3  // Priority
    ,  NULL
  ); //Task Handle
  
  xTaskCreate(
    TaskHeating
    ,  "Heating" // A name just for humans
    ,  128  // Stack size
    ,  NULL //Parameters for the task
    ,  2  // Priority
    ,  NULL
  ); //Task Handle

  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.

}

void loop()
{
  // Empty. Things are done in Tasks.
}


/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskAnalogRead( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;)
  {
    // read the input on analog pin 0:
    int sensorValue = analogRead(TEMP_PIN);
    float analogValue;
    float tempValue;
    // See if we can obtain or "Take" the Serial Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      // We were able to obtain or "Take" the semaphore and can now access the shared resource.
      // We want to have the Serial Port for us alone, as it takes some time to print,
      // so we don't want it getting stolen during the middle of a conversion.
      // print out the value you read:
      //sensorValue on range 0-1023 digital
      //Serial.println("ADC " + String(sensorValue));
      analogValue = (sensorValue * 4.88 );
      tempValue = (analogValue / 10 );
      //Serial.println(String(tempValue) + " C");

      msg2disp[TEMP] = String(tempValue);

      if ((int)tempValue < TEMP_LOW) {
        uint16BlinkSpeed = BLINK_SLOW;
        msg2disp[LED] = "Cold";
        msg2disp[FAN] = "OFF";
        msg2disp[HEAT] = "ON";
        cool = true;
        heat = false;
      }
      else if ((int)tempValue < TEMP_HIGH ) {
        uint16BlinkSpeed = BLINK_MID;
        msg2disp[LED] = "Normal";
        msg2disp[FAN] = "OFF";
        msg2disp[HEAT] = "OFF";
        cool = false;
        heat = false;
      }
      else if ((int)tempValue >= TEMP_HIGH ) {
        uint16BlinkSpeed = BLINK_FAST;
        msg2disp[LED] = "Alert";
        msg2disp[FAN] = "ON";
        msg2disp[HEAT] = "OFF";
        cool = false;
        heat = true;
      }
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }
    vTaskDelay(READING_REFRESH_TIME);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskLedBlink( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;)
  {
    // See if we can obtain or "Take" the Serial Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      // We were able to obtain or "Take" the semaphore and can now access the shared resource.
      // We want to have the Serial Port for us alone, as it takes some time to print,
      // so we don't want it getting stolen during the middle of a conversion.
      // print out the value you read:
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }
    vTaskDelay(uint16BlinkSpeed);  // one tick delay (15ms) in between reads for stability

  }
}


void TaskCooling( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;)
  {
    // See if we can obtain or "Take" the Serial Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      if ( (cool == false) && (heat == true) ){
        digitalWrite(COOLING_PIN , HIGH);
      }
       else {
        digitalWrite(COOLING_PIN , LOW);
        }
      // We were able to obtain or "Take" the semaphore and can now access the shared resource.
      // We want to have the Serial Port for us alone, as it takes some time to print,
      // so we don't want it getting stolen during the middle of a conversion.
      // print out the value you read:
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }
    vTaskDelay(COOLING_RESPONSE);  // one tick delay (15ms) in between reads for stability
  }
}


void TaskHeating( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;)
  {
    // See if we can obtain or "Take" the Serial Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      if ( (cool == true) && (heat == false) ){
        digitalWrite(HEATING_PIN , HIGH);
      }
      else {
        digitalWrite(HEATING_PIN , LOW);
        }
      // We were able to obtain or "Take" the semaphore and can now access the shared resource.
      // We want to have the Serial Port for us alone, as it takes some time to print,
      // so we don't want it getting stolen during the middle of a conversion.
      // print out the value you read:
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }
    vTaskDelay(COOLING_RESPONSE);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskDisplaying( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;)
  {
    // See if we can obtain or "Take" the Serial Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      Serial.println("Temp: "+String(msg2disp[TEMP]));
      Serial.println("FAN : "+String(msg2disp[FAN]));
      Serial.println("HEAT: "+String(msg2disp[HEAT]));
      Serial.println("LED : "+String(msg2disp[LED]));
      // We were able to obtain or "Take" the semaphore and can now access the shared resource.
      // We want to have the Serial Port for us alone, as it takes some time to print,
      // so we don't want it getting stolen during the middle of a conversion.
      // print out the value you read:
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }
    vTaskDelay(DISP_REFRESH_TIME);  // one tick delay (15ms) in between reads for stability
  }
}
