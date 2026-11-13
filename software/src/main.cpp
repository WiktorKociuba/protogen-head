#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "SoundData.h" // The sound data
#include <FastLED.h>
//Define frames
#include "frame0.h"
#include "frame1.h"
#include "frame2.h"
#include "frame3.h"
#include "frame4.h"

//Define Waveshare 22101 pins
#define R1_PIN 5
#define G1_PIN 6
#define B1_PIN 7
#define R2_PIN 15
#define G2_PIN 16
#define B2_PIN 17
#define A_PIN 18
#define B_PIN 21
#define C_PIN 8
#define D_PIN 14
#define E_PIN 47
#define LAT_PIN 13
#define OE_PIN 12
#define CLK_PIN 11
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 2

#define TOUCH_PIN 4
const int threshold = 20; // experimentally defined (not yet)
int touchValue = 100000;

#define NUM_LEDS 10
#define LED_PIN 10
#define LED_BRIGHTNESS 80
CRGB leds[NUM_LEDS];

#define SOUND_PIN 9
#define SAMPLE_RATE 8000

MatrixPanel_I2S_DMA *dma_display = nullptr;
//Create the pins vector
HUB75_I2S_CFG::i2s_pins _pins={R1_PIN,G1_PIN,B1_PIN,R2_PIN,G2_PIN,B2_PIN,A_PIN,B_PIN,C_PIN,D_PIN,E_PIN,LAT_PIN,OE_PIN,CLK_PIN};
const uint16_t* animFrames[] = {
    frame_00,
    frame_01,
    frame_02,
    frame_03,
    frame_04,
    frame_04,
    frame_03,
    frame_02,
    frame_01,
    frame_00
}; //An array containing all frames
const int totalFrames = 10;
int currentFrame = 0;
unsigned long lastFrameTime = 0;
const int frameDelay = 66; //Time between frames in ms
int closeTime = 0; //last close animation time
int openTime = 0; //last open animation time

const int soundLength = sizeof(sound);
hw_timer_t *audioTimer = NULL;
volatile int sampleIndex = 0;
volatile bool playSound = false;

void LEDMatrixAnim(){
    if(totalFrames > 0 && (millis() - lastFrameTime >= frameDelay) && currentFrame <= totalFrames/2 && millis()-openTime>4000){ // play the animations
        lastFrameTime = millis(); //Get the current time
        dma_display->drawRGBBitmap(0,0,(uint16_t*)animFrames[currentFrame],128,32); //draw the frame
        currentFrame = (currentFrame + 1);
        closeTime = millis();
    }
    if(totalFrames > 0 && (millis() - lastFrameTime >= frameDelay) && currentFrame > totalFrames/2 && millis()-closeTime>500){
        lastFrameTime = millis();
        dma_display->drawRGBBitmap(0,0,(uint16_t*)animFrames[currentFrame],128,32);
        currentFrame = (currentFrame + 1)%totalFrames;
        openTime = millis();
    }
}
void playSoundOnBoop(){
    touchValue = touchRead(TOUCH_PIN);
    if(touchValue>threshold && !playSound){ //triger the sound play when touch is detected
        sampleIndex = 0;
        playSound = true;
    }
}
void setLEDColor(){
    /*
        To control individual LEDs
        leds[i] = CRGB(R,G,B);
        To set everything the same
        fill_solid(leds,NUM_LEDS,CRGB(R,G,B));
        Then
        FastLED.show();
    */
    fill_solid(leds,NUM_LEDS,CRGB(75,0,130));
    FastLED.show();
}
void IRAM_ATTR onAudioTimer(){
    if(playSound){
        if(sampleIndex<soundLength){
            ledcWrite(SOUND_PIN,sound[sampleIndex]);
            sampleIndex++; //go trough the whole sound file
        }
        else{ //reset the sampleIndex and the sound level when the audio file ends
            playSound = false;
            sampleIndex = 0;
            ledcWrite(SOUND_PIN,128);
        }
    }
}
void setup(){
    Serial.begin(115200);
    HUB75_I2S_CFG mxconfig{
        PANEL_RES_X,
        PANEL_RES_Y,
        PANEL_CHAIN,
        _pins
    }; // Declare the Waveshare 22101 config
    dma_display = new MatrixPanel_I2S_DMA(mxconfig); // assign the matrix config
    dma_display->begin(); // initialize display
    dma_display->setBrightness(75); // set the brightness to low, so that it doesn't draw too much current
    dma_display->clearScreen();
    //Setup the LED strips and set it to one color
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds,NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();
    setLEDColor();
    //setup the sound pin for 62.5kHz and 8 bit sampling + start the timer
    ledcAttach(SOUND_PIN,62500,8);
    audioTimer = timerBegin(1000000);
    timerAttachInterrupt(audioTimer,&onAudioTimer);
    timerAlarm(audioTimer,1000000/SAMPLE_RATE,true,0);
}

void loop(){
    LEDMatrixAnim(); //play the animation in loop
    playSoundOnBoop(); //play the sound in loop
}