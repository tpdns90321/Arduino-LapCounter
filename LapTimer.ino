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
#define MILLIDELAY 10
#define U_TRIG 7
#define U_ECHO 8
#define COUNTBUTTON A2
#define CONFBUTTON A3
#define SENSORED 700

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
	void numberSelect(byte &num,const byte m, const byte limit);
	void numberSelect(byte &num,const byte limit);
	void selectMode();
	void timeAttack();
	void lapAttack();

	public:
	RaceTimer(int p1, int p2, LapTimer* lt) : state(0),currentlap(0),countButton(p1),confButton(p2), lt(lt), enable(false) {
		pinMode(countButton,INPUT_PULLUP);
		pinMode(confButton,INPUT_PULLUP);
	}
 
	void stop();
	void start();
	void enter();
	void check();
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
			delay(MILLIDELAY*10);
		}
	return true;
	}
	return false;
}

void RaceTimer::numberSelect(byte &num,const byte m, const byte limit){
	int st = m;
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
				st = m;
			}
		}
		delay(MILLIDELAY);
	}
}

void RaceTimer::numberSelect(byte &num,const byte limit){
  numberSelect(num,1,limit);
}

void setPrint(char *set){
	lcd.setCursor(4,0);
	lcd.print(set);
}

void RaceTimer::selectMode(){
	setPrint("mode");
	numberSelect(mode,3);
	switch(mode){
		case 2:
		lcd.clear();
		setPrint("lap");
		numberSelect(lap,2,5);
		break;
	}
	lcd.clear();
	setPrint("car");
	numberSelect(num,5);
	lcd.clear();
}

void RaceTimer::stop(){
	for(int i = 0;i < 4;i++){
		(lt + i)->stopTimer();
	}
	currentlap = 0;
	state = 0;
	enable = false;
}

void RaceTimer::start(){
	reset();
	noInterrupts();
	selectMode();
	enable = true;
	interrupts();
}

void RaceTimer::timeAttack(){
	if (!enable)
		return;
	if(state < num){
		(lt + state)->startTimer();
	}else if(state >= num && state < (num * 2)){
		(lt + (state % num))->stopTimer();
	}
	state++;
	if (state >= (num * 2)){
		stop();
	}
}

void RaceTimer::lapAttack(){
	if (!enable)
		return;
	if(state < num && currentlap == 0){
		(lt + state)->startTimer();
	}else if(currentlap >= (lap - 1) && state < num){
		(lt + state)->stopTimer();
	}

	state++;
	if(state >= num){
		state = 0;
		currentlap++;
	}
	if (currentlap >= lap){
		stop();
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

void RaceTimer::check(){
	if (ButtonClick(confButton)){
		if(enable){
			stop();
			start();
		}else{
			start();
		}
	}
}

LapTimer lt[4];

RaceTimer rt = RaceTimer(COUNTBUTTON,CONFBUTTON,lt);

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

	rt.start();

	MsTimer2::set(MILLIDELAY,timerRefresh);
	MsTimer2::start();
}

void loop(){
	noInterrupts();
	int data = ul.Timing();
	interrupts();

	if (data < SENSORED){
		rt.enter();
		delay(MILLIDELAY * 10);
	}
	rt.check();

	delay(MILLIDELAY);
}
