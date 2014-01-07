/*
 * lcd_controller.h
 *
 * Created: 1/6/2014 2:57:56 AM
 *  Author: Luigi
 */ 


#ifndef LCD_CONTROLLER_H_
#define LCD_CONTROLLER_H_

#define LINE_1 0x00
#define LINE_2 0x40


class LCD {
	
	struct Line {
		char buf[40];
		int pos;
		int length;
		int ticks_at_end;
		int ticks_at_start;
	};
	
	int LCD_RS;
	int LCD_E;
	
	int LCD_D0;
	int LCD_D1;
	int LCD_D2;
	int LCD_D3;
	
	public:
		LCD(int, int, int, int, int, int);
		void init();
		void put_char(unsigned char);
		void put_string(const char *str, char line);
		void send_command();
		void set_text_mode();
		void set_command_mode();
		void clear_display();
		void shift_left();
		Line Line1;
		Line Line2;
};

#endif /* LCD_CONTROLLER_H_ */