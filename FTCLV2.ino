/*
Written by Freemansoft Inc.
Exercise Neopixel (WS2811 or WS2812) shield using the adafruit NeoPixel library
You need to download the Adafruit NeoPixel library from github, 
unzip it and put it in your arduino libraries directory

commands include
rgb   <led 0..39> <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9>: set RGB pattern to  pattern <0:off, 1:continuous>
rgb   <all -1>    <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9>: set RGB pattern to  pattern <0:off, 1:continuous>
debug <true|false> log all input to serial
blank clears all
demo random colors 

*/

#include <Adafruit_NeoPixel.h>
#include <MsTimer2.h>



boolean logDebug = false;
// pin used to talk to NeoPixel
#define PIN 6

const int NUM_PIXELS = 8;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);


typedef struct
{
	uint32_t activeValues;         // packed 32 bit created by Strip.Color
	uint32_t lastWrittenValues;    //for fade in the future
	byte currentState;             // used for blink
	byte pattern;
	unsigned long lastChangeTime;  // where are we timewise in the pattern
} pixelDefinition;
// should these be 8x5 intead of linear 40 ?
volatile pixelDefinition lightStatus[NUM_PIXELS];
volatile boolean stripShowRequired = false;


///////////////////////////////////////////////////////////////////////////

// time between steps
const int STATE_STEP_INTERVAL = 10; // in milliseconds - all our table times are even multiples of this
const int MAX_PWM_CHANGE_SIZE = 32; // used for fading at some later date

/*================================================================================
*
* bell pattern buffer programming pattern lifted from http://arduino.cc/playground/Code/MultiBlink
*
*================================================================================*/

typedef struct
{
	boolean  isActive;          // digital value for this state to be active (on off)
	unsigned long activeTime;    // time to stay active in this state stay in milliseconds 
} stateDefinition;

// the number of pattern steps in every blink  pattern 
const int MAX_STATES = 4;
typedef struct
{
	stateDefinition state[MAX_STATES];    // can pick other numbers of slots
} ringerTemplate;

const int NUM_PATTERNS = 10;
const ringerTemplate ringPatterns[] =
{
	//  state0 state1 state2 state3 
	// the length of these times also limit how quickly changes will occure. color changes are only picked up when a true transition happens
	{ /* no variable before stateDefinition*/ {{false, 1000}, {false, 1000}, {false, 1000}, {false, 1000}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{true,  1000},  {true, 1000},  {true, 1000},  {true, 1000}}   /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{true , 300}, {false, 300}, {false, 300}, {false, 300}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{false, 300}, {true , 300}, {true , 300}, {true , 300}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{true , 200}, {false, 100}, {true , 200}, {false, 800}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{false, 200}, {true , 100}, {false, 200}, {true , 800}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{true , 300}, {false, 400}, {true , 150}, {false, 400}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{false, 300}, {true , 400}, {false, 150}, {true , 400}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{true , 100}, {false, 100}, {true , 100}, {false, 800}}  /* no variable after stateDefinition*/ },
	{ /* no variable before stateDefinition*/ {{false, 100}, {true , 100}, {false, 100}, {true , 800}}  /* no variable after stateDefinition*/ },
};


///////////////////////////////////////////////////////////////////////////


void setup() {
	// 50usec for 40pix @ 1.25usec/pixel : 19200 is .5usec/bit or 5usec/character
	// there is a 50usec quiet period between updates 
	//Serial.begin(19200);
	// don't want to lose characters if interrupt handler too long
	// serial interrupt handler can't run so arduino input buffer length is no help
	Serial.begin(9600);


	strip.begin();
	strip.show(); // Initialize all pixels to 'off'
	stripShowRequired = false;

	// initialize our buffer as all LEDS off
	go_dark();

	//show a quickcolor pattern
	configureForDemo();
	delay(3000);
	go_dark();

	MsTimer2::set(STATE_STEP_INTERVAL, process_blink);
	MsTimer2::start();
}

void loop(){
	const int READ_BUFFER_SIZE = 4*6; // rgb_lmp_red_grn_blu_rng where ringer is only 1 digit but need place for \0
	char readBuffer[READ_BUFFER_SIZE];
	int readCount = 0;
	char newCharacter = '\0';
	while((readCount < READ_BUFFER_SIZE) && newCharacter !='\r'){
		// did the timer interrupt handler make changes that require a strip.show()?
		// note: strip.show() attempts to unmask interrupts as much as possible
		// must be inside character read while loop
		if (stripShowRequired) {
			stripShowRequired = false;
			strip.show();
		}
		if (Serial.available()){
			newCharacter = Serial.read();
			if (newCharacter != '\r'){
				readBuffer[readCount] = newCharacter;
				readCount++;
			}
		}
	}
	if (newCharacter == '\r'){
		readBuffer[readCount] = '\0';
		// this has to be before process_Command because buffer is wiped
		if (logDebug){
			Serial.print("received ");
			Serial.print(readCount);
			Serial.print(" characters, command: ");
			Serial.println(readBuffer);
		}
		// got a command so parse it
		process_command(readBuffer,readCount);
	} 
	else {
		// while look exited because too many characters so start over
	}
}
