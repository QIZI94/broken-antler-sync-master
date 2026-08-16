#include <Arduino.h>
#include <driver/uart.h>

#include "externaldev.h"
#include "wifimessaging.h"


#define RX1_PIN 5
#define TX1_PIN 4

static constexpr uart_port_t UART_PORT = UART_NUM_1;

void setup_uart()
{
  // 2. Communication port for the other chip (UART1)
  // Parameters: Baud rate, config, RX pin, TX pin
  Serial1.begin(250000, SERIAL_8N1, RX1_PIN, TX1_PIN);
  
	gpio_sleep_set_direction(GPIO_NUM_5, GPIO_MODE_INPUT);
	gpio_sleep_set_pull_mode(GPIO_NUM_5, GPIO_PULLUP_ONLY);
    /*
        Wake after this many RX edges/characters.
        1 is most responsive but may wake on noise.
        3-5 is often a good compromise.
    */
    uart_set_wakeup_threshold(UART_PORT, 3);

    /*
        Enable UART as light sleep wake source
    */
    esp_sleep_enable_uart_wakeup(UART_PORT);
}


void print_wakeup_reason()
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause)
    {
        case ESP_SLEEP_WAKEUP_UART:
            Serial.println("Wakeup: UART");
			/*pinMode(LED_BUILTIN, OUTPUT);
			digitalWrite(LED_BUILTIN, HIGH);
			
			//delay(100);
			digitalWrite(LED_BUILTIN, LOW);
			pinMode(LED_BUILTIN, INPUT);*/

            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup: TIMER");
            break;

        default:
            Serial.printf("Wakeup: %d\n", cause);
            break;
    }
}




uint32_t keepAwakelastTime = 0;
void setup() {
  // 1. Debugging port (USB CDC)
  	Serial.begin(115200); 
	setup_uart();
  
	esp_sleep_enable_timer_wakeup(5ULL * 1000ULL * 1000ULL);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	
	
	delay(3000);
	digitalWrite(LED_BUILTIN, LOW);
	pinMode(LED_BUILTIN, INPUT);
	initWifiMessageHandling();
	initExternalDevices();
	delay(1000);
	uint8_t brokenFrame[] = {0x5B, 0x20, 0x0, 0x0, 0x0};
	//Serial1.write(brokenFrame, sizeof(brokenFrame));

	

	keepAwakelastTime = millis();
	//sendMessageUART(antler::Message::Request{.requestedMessageType = antler::Message::Type::TIME_SYNC});

}

void loop() {
	//return;
  //Serial.println("Debug: System is running..."); // Goes to your PC
	
  
  //Serial1.println("COMMAND_TO_CHIP");            // Goes to the other chip
  
  	
	//sendMessageUART(antler::Message{.data = {.request = {.requestedMessageType = antler::Message::Type::TIME_SYNC}}, .type = antler::Message::Type::REQUEST});
	
	/*
        CPU pauses here until:
        - UART activity
        - timer expires
    */

   
   //delay(100);

   
	//delayMicroseconds(200);
	//delay(1);

	

	communicateWithExternalDevices();
	
	Serial.flush();
	if((millis() - keepAwakelastTime) >= 930){
		uartMessageManager.handler.sendDeferredMessage(UniformMessage::Type::TIME_SYNC);
		wifiMessageHandler.sendDeferredMessage(UniformMessage::TimeSync{.newTime = 0});
		Serial.println("here");
		keepAwakelastTime = millis();
		//esp_light_sleep_start();
		
		//Serial.println("Awake\n---------------");
		//print_wakeup_reason();
	}

	//delayMicroseconds(200);
	//delay(500);
	//delay(1000);
	
}

