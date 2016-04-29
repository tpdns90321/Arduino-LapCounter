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
#define COUNTBUTTON A5
#define CONFBUTTON A5

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
	char* getText();

	public:
	Time() : milli100p(0),second(0),minute(0),enable(true){}
	Time(long milli):enable(true){
		set(milli);
	};
	~Time(){
		clear();
	}
	void clear();
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

class RaceTimer{
	private:
	LapTimer *lt;
	byte mode;
	byte num;
	byte lap;
	byte state;
	byte currentlap;
	int countButton;
	int confButton;
	bool enable;
	void reset();
  void numberSelect(byte &num,const byte limit);
	void selectMode();
	void timeAttack();
	void lapAttack();

	public:
	RaceTimer(byte m, byte n,int p1, int p2, LapTimer* lt) : mode(m), num(n),state(0),currentlap(0),countButton(p1),confButton(p2), lt(lt), enable(false) {
		pinMode(countButton,INPUT_PULLUP);
		pinMode(confButton,INPUT_PULLUP);
	}
 
	void stop();
	void start();
	void enter();
};

void timeFormat(char **ch,int num,bool semi){
	if (num < 10){
		sprintf(*ch,"0%d",num);
	}else{
		sprintf(*ch,"%d",num);
	}

	*ch += strlen(*ch);
	
	if (semi){
		sprintf(*ch,":");
		*ch += 1;
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

void RaceTimer::reset(){
	lcd.clear();
	stop();
	for(int i = 0;i < 4;i++){
		(lt + i)->clear();
	}
}

bool ButtonClick(const int button){
	if(digitalRead(button) == LOW){
		while(digitalRead(button) == LOW){
			delay(MILLIDELAY * 5);
		}
   return true;
	}
	return false;
}

void RaceTimer::numberSelect(byte &num,const byte limit){
	int st = 1;
	while(true){
		lcd.setCursor(4,1);
		lcd.print(st);
		if(ButtonClick(confButton)){
			num = st;
			break;
		}
		if(ButtonClick(countButton)){
			st++;
			if(st >= limit){
				st = 1;
			}
		}
   delay(MILLIDELAY);
	}
}

void setPrint(char *set){
	lcd.setCursor(4,0);
	lcd.print(set);
}

void RaceTimer::selectMode(){
  noInterrupts();
	setPrint("mode");
	numberSelect(mode,3);
	switch(mode){
		case 2:
   lcd.clear();
		setPrint("lap");
		numberSelect(lap,5);
		break;
	}
 lcd.clear();
	setPrint("car");
	numberSelect(num,5);
  interrupts();
}

void RaceTimer::stop(){
	for(int i = 0;i < num;i++){
		(lt + i)->stopTimer();
	}
	enable = false;
}

void RaceTimer::start(){
	reset();
	selectMode();
	enable = true;
}

void RaceTimer::timeAttack(){
	if(state < num){
		(lt + state)->startTimer();
	}else if(state >= num && state < (num * 2)){
		(lt + (state % num))->stopTimer();
	}else {
		stop();
	}
	state++;
}

void RaceTimer::lapAttack(){
	if(num < state && currentlap == 0){
		(lt + state)->startTimer();
	}else if(currentlap == (lap - 1) && num < state){
		(lt + state)->stopTimer();
	}else if(currentlap >= lap){
		stop();
	}

	state++;
	if(num>=state){
		state = 0;
		currentlap++;
	}
}

void RaceTimer::enter(){
	if (!enable){
		return;
	}

	switch(mode){
		case 1:
		timeAttack();
		break;
		case 2:
		lapAttack();
		break;
	}
}

LapTimer lt[4];

void timerRefresh(){
	for (int i = 0;i < 4;i++){
		lt[i].print();
	}
}

RaceTimer rt = RaceTimer(4,1,COUNTBUTTON,CONFBUTTON,lt);

void setup(){
	lcd.begin(16,2);

	for(int i = 0;i < 4;i++){
		lt[i] = LapTimer();
		lt[i].setup(i);
	}

	rt.start();

	MsTimer2::set(MILLIDELAY,timerRefresh);
	MsTimer2::start();
	lcd.clear();
}

void loop(){
	noInterrupts();
	int data = ul.Timing();
	interrupts();

	if (data < 300){
		rt.enter();
		delay(MILLIDELAY * 10);
	}

	delay(MILLIDELAY);
}
