/*
 * lcd_controller.cpp
 *
 * Created: 1/6/2014 1:49:11 AM
 *  Author: Luigi
 */ 

#define F_CPU 16000000

#include <avr/io.h>
#include <util/delay.h>

#include "lcd_controller.h"

/*	Pin configuration:

	RS	= PORTB0
	E	= PORTB1
	
	D0	= PORTD4
	D1	= PORTD5
	D2	= PORTD6
	D3	= PORTD7	*/

void LCD::init(){
	
	_delay_ms(25);
	
	DDRB |= (1 << LCD_RS); //RS bit
	DDRB |= (1 << LCD_E); //Enable bit
	
	DDRD |= (1 << LCD_D0); //D0
	DDRD |= (1 << LCD_D1); //D1
	DDRD |= (1 << LCD_D2); //D2
	DDRD |= (1 << LCD_D3); //D3
	
	PORTB = 0;
	PORTD = 0;
	
	set_command_mode();
	
	//Set controller to 8-bit operation
	PORTD = 0b00110000;
	send_command();
	_delay_ms(10);
	
	PORTD = 0b00110000;
	send_command();
	_delay_ms(10);
	
	PORTD = 0b00110000;
	send_command();
	_delay_ms(10);
	
	//Set controller to 4-bit operation
	PORTD = 0b00100000;
	send_command();

	//Commands must be sent 4 bits at time - high bits then low bits
	//Command reference: http://www.epemag.wimborne.co.uk/lcd1.pdf
	//Timing reference: http://www.epemag.wimborne.co.uk/lcd2.pdf
	
	//Two-line operation
	PORTD = 0b00100000;
	send_command();
	PORTD = 0b10000000;
	send_command();
	
	//Enable display
	PORTD = 0b00000000;
	send_command();
	PORTD = 0b10000000;
	send_command();
	
	//Clear display
	PORTD = 0b00000000;
	send_command();
	PORTD = 0b00010000;
	send_command();
	
	_delay_ms(5);
	
	//Enable cursor
	PORTD = 0b00000000;
	send_command();
	PORTD = 0b11110000;
	send_command();
}

void LCD::send_command(){
	_delay_ms(1);
	PORTB |= (1 << PORTB1);
	_delay_ms(1);
	PORTB &= ~(1 << PORTB1);
	_delay_ms(1);
};

void LCD::set_text_mode(){
	PORTB |= (1 << PORTB0);
	_delay_ms(1);
};

void LCD::set_command_mode(){
	PORTB &= ~(1 << PORTB0);
	_delay_ms(1);
};

void LCD::clear_display(){
	set_command_mode();
	PORTD = 0b00000000;
	send_command();
	PORTD = 0b00010000;
	send_command();
}

void LCD::shift_left(){
	set_command_mode();
	PORTD = 0b00010000;
	send_command();
	PORTD = 0b10000000;
	send_command();
}

void LCD::put_char(unsigned char c){
	//Need to split char in half and send two blocks of data to the LCD
	
	PORTD = 0;
	
	PORTD |= (((c & 0x10) > 0) << PORTD4);
	PORTD |= (((c & 0x20) > 0) << PORTD5);
	PORTD |= (((c & 0x40) > 0) << PORTD6);
	PORTD |= (((c & 0x80) > 0) << PORTD7);
	
	send_command();
	
	PORTD = 0;
	
	PORTD |= (((c & 0x01) > 0) << PORTD4);
	PORTD |= (((c & 0x02) > 0) << PORTD5);
	PORTD |= (((c & 0x04) > 0) << PORTD6);
	PORTD |= (((c & 0x08) > 0) << PORTD7);
	
	send_command();
}

void LCD::put_string(const char *str, char line){
	
	set_command_mode();
	
	if(line == LINE_1){
		PORTD = 0b10000000;
		send_command();
		PORTD = 0b00000000;
		send_command();
	} else {
		PORTD = 0b11000000;
		send_command();
		PORTD = 0b00000000;
		send_command();
	}
	
	set_text_mode();
	
	while(*str){
		if(*str != '\r' && *str != '\n'){
			put_char(*str++); //don't print \r\n to the LCD display
		}
	};
	
	set_command_mode();
}

LCD::LCD(int _LCD_RS, int _LCD_E, int _LCD_D0, int _LCD_D1, int _LCD_D2, int _LCD_D3){
	LCD_RS = _LCD_RS;
	LCD_E = _LCD_E;
	LCD_D0 = _LCD_D0;
	LCD_D1 = _LCD_D1;
	LCD_D2 = _LCD_D2;
	LCD_D3 = _LCD_D3;
}