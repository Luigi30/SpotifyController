/*
 * SpotifyController.cpp
 *
 * Created: 1/4/2014 6:41:43 PM
 *  Author: Luigi
 */ 

#define F_CPU 16000000

#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "lcd_controller.h"

bool repeat = 0;

//The LCD's inputs is hooked up on these digital ports
LCD lcd(PORTB0, PORTB1, PORTD4, PORTD5, PORTD6, PORTD7);

//The LCD's output buttons are on analog input AD0.

char command_buffer[64] = "";

uint16_t ReadADC(uint8_t __channel)
{
	ADMUX = (ADMUX & 0xf0) |(__channel & 0x0f); // Channel selection
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADC;
}

int main(void)
{
	USART_Init(MYUBRR); // Initializes the serial communication
	
	lcd.init();
	
	lcd.set_text_mode();
	
	// initialize Timer1
	cli();			// disable global interrupts
	TCCR1A = 0;		// set entire TCCR1A register to 0
	TCCR1B = 0;		// same for TCCR1B

	//Clock speed of ATMega: 16MHz
	//Timer counts from 0 to 65535, incrementing every clock tick (16,000,000 times per second)
	//	Timer takes 65535 / 16000000 = ~0.0041s to send an interrupt in overflow mode
	//
	//Use the timer in compare mode with the prescaler to get 1 second:
	//	(15625 / 16000000) * 1024 = 1 second
	//	CS10/CS12 high in TCCR1B control register prescales the timer (makes it take longer to tick) by 1024, giving us 15625 ticks per second

	// set compare match register to desired timer count:
	OCR1A = 7812; //.5 seconds to update
	// turn on CTC mode:
	TCCR1B |= (1 << WGM12);
	// Set CS10 and CS12 bits for 1024 prescaler:
	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << CS12);
	// enable timer compare interrupt:
	TIMSK1 |= (1 << OCIE1A);

	//We're ready! Start and enable interrupts!
	sei();

	lcd.put_string("Waiting for", LINE_1);
	lcd.put_string("Python connect", LINE_2);
	
	//Analog setup
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); //Enable ADC and set 128 prescale
	ADMUX |= (1 << REFS0); // Set ADC reference to AVCC 
	
	while(1){
		//LCD keypad ADC data:
		//	No buttons - 1023
		//	L - 500-510
		//	U - 140-150
		//	R - 0
		//	D - 320-330
		//
		//	Select - 740-750
		uint16_t adc_data = ReadADC(0); //ADC0
		
		if(!repeat){
			if(adc_data > 740 && adc_data < 750){ //the select button is pressed
				USART_Sendbyte('P'); //send the pause command back to the PC
				repeat = 1; //don't let this repeat
				_delay_ms(25); //debounce
			}	
		} else {
			if(adc_data > 1000){ //if no buttons are pressed
				repeat = 0; //we can press a button again
				_delay_ms(25); //debounce
			}
		}
	}
}


void process_command(char cmd[]){
	/* Commands we will process:
		LC - Clear LCD.
		LP1 - Print to line1 of the LCD.
		LP2 - Print to line2 of the LCD. */
	
	if (strstr(cmd, "LC")) {
		lcd.clear_display();
	} else if(strstr(cmd, "LP1")){
		char lcd_buffer[40];
		strcpy(lcd_buffer, (cmd+3)); //everything after the command is put into the LCD data buffer
		strcpy(lcd.Line1.buf, (cmd+3)); //send data to the Line buffer
		lcd.Line1.length = strlen(lcd.Line1.buf); //and the length
		lcd.Line1.pos = 0; //reset the position
		lcd.Line1.ticks_at_start = 0;
		lcd.Line1.ticks_at_end = 0;
		lcd.put_string(lcd_buffer, LINE_1); //write it to the screen
	} else if(strstr(cmd, "LP2")){
		char lcd_buffer[40];
		strcpy(lcd_buffer, (cmd+3)); //everything after the command
		strcpy(lcd.Line2.buf, (cmd+3));
		lcd.Line2.length = strlen(lcd.Line2.buf);
		lcd.Line2.pos = 0;
		lcd.Line2.ticks_at_start = 0;
		lcd.Line2.ticks_at_end = 0;
		lcd.put_string(lcd_buffer, LINE_2);
	} else {
		//USART_Send_string("Invalid command.");
	}
}

/* Interrupt service routines */

ISR(USART_RX_vect){ //ISR for handling data received over the wire.
	char data[1] = {UDR0};
	
	//unsigned char c = UDR0; //UDR = serial buffer
	//USART_Sendbyte(data[0]);
	
	if(!(data[0] == '\r' || data[0] == '\n')){
		strncat(command_buffer, data, 1); //don't add CRLF to the command buffer...
	}
	
	if(data[0] == '\n'){ //on CRLF, process string
		process_command(command_buffer);
		memset(command_buffer, 0, 64);
	}
}

ISR(TIMER1_COMPA_vect)
{
	//USART_Sendbyte('T');
	
	lcd.put_string(lcd.Line1.buf+lcd.Line1.pos, LINE_1);
	lcd.put_string(lcd.Line2.buf+lcd.Line2.pos, LINE_2);
	
	if(lcd.Line1.pos == lcd.Line1.length - 16){ //if we've reached the end of the text...
		if(lcd.Line1.ticks_at_end == 4){ //and we've been here for 4 ticks...
			lcd.Line1.pos = 0; //reset to the beginning of the line
			lcd.Line1.ticks_at_end = 0;
			lcd.Line1.ticks_at_start = 0;
		} else {
			lcd.Line1.ticks_at_end++; //or count another tick at the end
		}
	}
	
	if(lcd.Line2.pos == lcd.Line2.length - 16){
		if(lcd.Line2.ticks_at_end == 4){
			lcd.Line2.pos = 0;
			lcd.Line2.ticks_at_end = 0;
			lcd.Line2.ticks_at_start = 0;
		} else {
			lcd.Line2.ticks_at_end++;
		}
	}
	
	if(lcd.Line1.pos == 0 && lcd.Line1.ticks_at_start < 4){ //have we been at the start for 4 ticks yet?
		lcd.Line1.ticks_at_start++; //if not, keep counting ticks
	}
	
	if(lcd.Line2.pos == 0 && lcd.Line2.ticks_at_start < 4){
		lcd.Line2.ticks_at_start++;
	}
	
	if(lcd.Line1.length > 16 && lcd.Line1.ticks_at_end == 0 && lcd.Line1.ticks_at_start == 4){ //have we been at the start for over 4 ticks or are we in the middle?
		lcd.Line1.pos++; //keep going!
	}
	
	if(lcd.Line2.length > 16 && lcd.Line2.ticks_at_end == 0 && lcd.Line2.ticks_at_start == 4){
		lcd.Line2.pos++;
	}

	//TODO: advance the LCD text on the MCU instead of just updating the display from the PC.
}