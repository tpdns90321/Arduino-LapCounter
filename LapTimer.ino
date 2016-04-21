#include <LiquidCrystal.h>
#include "Ultrasonic.h"
#include "MsTimer2.h"

#define LC_RS 12
#define LC_E 11
#define LC_1 5
#define LC_2 4
#define LC_3 3
#define LC_4 2
#define LC_Back 13
#define MILLIDELAY 25
#define U_TRIG 7
#define U_ECHO 8

LiquidCrystal lcd(LC_RS,LC_E,LC_1,LC_2,LC_3,LC_4);
Ultrasonic ul(U_TRIG,U_ECHO);

class Time{
	private:
	char text[12];
	int milli100p;
	int second;
	int minute;
	void refreshText();

	protected:
	bool enable;
	void set(long milli);
	void clear();
	char* getText();

	public:
	Time() : milli100p(0),second(0),minute(0),enable(true){}
	Time(long milli):enable(true){
		set(milli);
	};
	~Time(){
		clear();
	}
};

class TextTimer : public Time{
	private:
	long start;

	public:
	TextTimer() : Time(Time()){
		enable = false;
	}
	void startTimer();
	void stopTimer();
	void refresh();
	bool isOk();
};

class LapTimer : public TextTimer{
	private:
	int num;
	int x,y;

	public:
	LapTimer() : TextTimer(TextTimer()){}
	LapTimer(int _num) : TextTimer(TextTimer()){
		setup(_num);
	}
	void setup(int _num);
	void print();
};

void timeFormat(char **ch,int num,bool semi){
	if (num < 10){
		sprintf(*ch,"0%d",num);
	}else{
		sprintf(*ch,"%d",num);
	}

	(*ch) += strlen(*ch);
	
	if (semi){
		sprintf(*ch,":");
		(*ch) += 1;
	}
}

void Time::refreshText(){
	char* ch = text;
	timeFormat(&ch,minute,true);
	timeFormat(&ch,second,true);
	timeFormat(&ch,milli100p,false);
}

void Time::set(long milli){
	if (enable){
		milli100p = int(milli % 1000 / 10);
		second = milli / 1000 % 60;
		minute = (milli / 60000);
		refreshText();
	}
}

void Time::clear(){
	milli100p = 0;
	second = 0;
	minute = 0;
	strcpy(text,"");
}

char *Time::getText(){
	return text;
}

void TextTimer::startTimer(){
	clear();
	enable = true;
	start = millis();
}

void TextTimer::stopTimer(){
	enable = false;
}

void TextTimer::refresh(){
	set(millis()-start);
}

bool TextTimer::isOk(){
	return enable;
}

void LapTimer::setup(int _num){
	num = _num;
	x = (_num * 8) % 16;
	y = (_num * 8) / 16;
}

void LapTimer::print(){
	refresh();
	lcd.setCursor(x,y);
	lcd.print(getText());
}

LapTimer lt[4];

void timerRefresh(){
	for (int i = 0;i < 4;i++){
		lt[i].print();
	}
}

void setup(){
	lcd.begin(16,2);

	for(int i = 0;i < 4;i++){
		lt[i] = LapTimer();
		lt[i].setup(i);
	}

	MsTimer2::set(MILLIDELAY,timerRefresh);
	MsTimer2::start();
	lcd.clear();
}

int state;

void loop(){
	noInterrupts();
	int data = ul.Timing();
	interrupts();

	if (data < 300){
		if (state > 8){
			state = 0;
		}
		if (state < 4){
			lt[state].startTimer();
		}else if(state < 8){
			lt[state % 4].stopTimer();
		}
		state++;
		delay(100);
	}

	delay(10);
}