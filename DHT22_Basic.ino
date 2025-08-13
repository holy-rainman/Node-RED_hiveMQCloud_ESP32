#include "DHTesp.h"
#include <Ticker.h>

DHTesp dht;
void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();

TaskHandle_t tempTaskHandle = NULL;
Ticker tempTicker;
bool tasksEnabled = false;
int dhtPin = 27;

bool initTemp() 
{ byte resultValue = 0;
 	dht.setup(dhtPin, DHTesp::DHT22);
	Serial.println("DHT initiated");

  // Start task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,                       /* Function to implement the task */
			"tempTask ",                    /* Name of the task */
			4000,                           /* Stack size in words */
			NULL,                           /* Task input parameter */
			5,                              /* Priority of the task */
			&tempTaskHandle,                /* Task handle. */
			1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) 
  { Serial.println("Failed to start task for temperature update");
    return false;
  } 
  else 
    tempTicker.attach(1, triggerGetTemp);
  return true;
}
void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
	   xTaskResumeFromISR(tempTaskHandle);
  }
}
void tempTask(void *pvParameters) 
{ Serial.println("tempTask loop started");
	while(1)
  { if (tasksEnabled)
			getTemperature();
		vTaskSuspend(NULL);
	}
}
bool getTemperature() 
{ TempAndHumidity newValues = dht.getTempAndHumidity();
	if (dht.getStatus() != 0) 
  {	Serial.println("DHT11 error status: " + String(dht.getStatusString()));
		return false;
	}
  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity));
	return true;
}

void setup()
{ Serial.begin(9600);
  Serial.println();
  Serial.println("DHT ESP32 example with tasks");
  initTemp();
  tasksEnabled = true;
}

void loop() 
{ if (!tasksEnabled) 
  { delay(2000);
    tasksEnabled = true;
    if (tempTaskHandle != NULL)
			vTaskResume(tempTaskHandle);
  }
  yield();
}
