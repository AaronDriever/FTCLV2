

/*
* blank out the LEDs and buffer
*/
void go_dark(){
	unsigned long ledLastChangeTime = millis();
	for ( int index = 0 ; index < NUM_PIXELS; index++){
		lightStatus[index].currentState = 0;
		lightStatus[index].pattern = 0;
		lightStatus[index].activeValues = strip.Color(0,0,0);
		lightStatus[index].lastChangeTime = ledLastChangeTime;
		strip.setPixelColor(index, lightStatus[index].activeValues);
	}
	// force them all dark immediatly so they go out at the same time
	// could have waited for timer but different blink rates would go dark at slighly different times
	strip.show();
}

//////////////////////////// handler  //////////////////////////////
//
/*
Interrupt handler that handles all blink operations
*/
void process_blink(){
	boolean didChangeSomething = false;
	unsigned long now = millis();
	for ( int index = 0 ; index < NUM_PIXELS; index++){
		byte currentPattern = lightStatus[index].pattern; 
		if (currentPattern >= 0){ // quick range check for valid pattern?
			if (now >= lightStatus[index].lastChangeTime 
				+ ringPatterns[currentPattern].state[lightStatus[index].currentState].activeTime){
					// calculate next state with rollover/repeat
					int currentState = (lightStatus[index].currentState+1) % MAX_STATES;
					lightStatus[index].currentState = currentState;
					lightStatus[index].lastChangeTime = now;

					// will this cause slight flicker if already showing led?
					if (ringPatterns[currentPattern].state[currentState].isActive){
						strip.setPixelColor(index, lightStatus[index].activeValues);
					} else {
						strip.setPixelColor(index,strip.Color(0,0,0));
					}
					didChangeSomething = true;
			}
		}
	}
	// don't show in the interrupt handler because interrupts would be masked
	// for a long time. 
	if (didChangeSomething){
		stripShowRequired = true;
	}
}

// first look for commands without parameters then with parametes 
boolean  process_command(char *readBuffer, int readCount){
	int indexValue;
	byte redValue;
	byte greenValue;
	byte blueValue;
	byte patternValue;

	// use string tokenizer to simplify parameters -- could be faster by only running if needed
	char *command;
	char *parsePointer;
	//  First strtok iteration
	command = strtok_r(readBuffer," ",&parsePointer);

	boolean processedCommand = false;
	if (strcmp(command,"h") == 0 || strcmp(command,"?") == 0){
		help();
		processedCommand = true;
	} else if (strcmp(command,"rgb") == 0){
		char * index   = strtok_r(NULL," ",&parsePointer);
		char * red     = strtok_r(NULL," ",&parsePointer);
		char * green   = strtok_r(NULL," ",&parsePointer);
		char * blue    = strtok_r(NULL," ",&parsePointer);
		char * pattern = strtok_r(NULL," ",&parsePointer);
		if (index == NULL || red == NULL || green == NULL || blue == NULL || pattern == NULL){
			help();
		} else {
			// this code shows how lazy I am.
			int numericValue;
			numericValue = atoi(index);
			if (numericValue < 0) { numericValue = -1; }
			else if (numericValue >= NUM_PIXELS) { numericValue = NUM_PIXELS-1; };
			indexValue = numericValue;

			numericValue = atoi(red);
			if (numericValue < 0) { numericValue = 0; }
			else if (numericValue > 255) { numericValue = 255; };
			redValue = numericValue;
			numericValue = atoi(green);
			if (numericValue < 0) { numericValue = 0; }
			else if (numericValue > 255) { numericValue = 255; };
			greenValue = numericValue;
			numericValue = atoi(blue);
			if (numericValue < 0) { numericValue = 0; }
			else if (numericValue > 255) { numericValue = 255; };
			blueValue = numericValue;
			numericValue = atoi(pattern);
			if (numericValue < 0) { numericValue = 0; }
			else if (numericValue > NUM_PATTERNS) { numericValue = NUM_PATTERNS-1; };
			patternValue = numericValue;
			/*
			Serial.println(indexValue);
			Serial.println(redValue);
			Serial.println(greenValue);
			Serial.println(blueValue);
			Serial.println(patternValue);
			*/
			if (indexValue >= 0){
				lightStatus[indexValue].activeValues = strip.Color(redValue,greenValue,blueValue);
				lightStatus[indexValue].pattern = patternValue;
			} else {
				for (int i = 0; i < NUM_PIXELS; i++){
					lightStatus[i].activeValues = strip.Color(redValue,greenValue,blueValue);
					lightStatus[i].pattern = patternValue;

				}
			}
			processedCommand = true;   
		}
	} else if (strcmp(command,"blank") == 0){
		go_dark();
		processedCommand = true;
	} else if (strcmp(command,"debug") == 0){
		char * shouldLog   = strtok_r(NULL," ",&parsePointer);
		if (strcmp(shouldLog,"true") == 0){
			logDebug = true;
		} else {
			logDebug = false;
		}
		processedCommand = true;
	} else if (strcmp(command,"demo") == 0){
		configureForDemo();
		processedCommand = true;
	} else {
		// completely unrecognized
	}
	if (!processedCommand){
		Serial.print(command);
		Serial.println(" not recognized ");
	}
	return processedCommand;
}

/*
* Simple method that displays the help
*/
void help(){
	Serial.println("h: help");
	Serial.println("?: help");
	Serial.println("rgb   <led 0..39> <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9>: set RGB pattern to  pattern <0:off, 1:continuous>");
	Serial.println("rgb   <all -1>    <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9>: set RGB pattern to  pattern <0:off, 1:continuous>");
	Serial.println("debug <true|false> log all input to serial");
	Serial.println("blank clears all");
	Serial.println("demo  color and blank wheel");
	Serial.flush();
}

//////////////////////////// demo  //////////////////////////////

/*
* show the various blink patterns
*/
void configureForDemo(){
	unsigned long ledLastChangeTime = millis();
	for ( int index = 0 ; index < NUM_PIXELS; index++){
		lightStatus[index].currentState = 0;
		lightStatus[index].pattern = (index%8)+1; // the shield is 8x5 so we do 8 patterns and we know pattern 0 is off
		lightStatus[index].activeValues = Wheel(index*index & 255);
		lightStatus[index].lastChangeTime = ledLastChangeTime;
	}
	uint16_t i;  
	for(i=0; i<strip.numPixels(); i++) {
		strip.setPixelColor(i,lightStatus[i].activeValues);
	}     
	strip.show();
}

//////////////////////////// stuff from the Adafruit NeoPixel sample  //////////////////////////////
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
	if(WheelPos < 85) {
		return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	} else if(WheelPos < 170) {
		WheelPos -= 85;
		return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	} else {
		WheelPos -= 170;
		return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}