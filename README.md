# LEDLights
Code changes the brightness of an LED light strip to match the beat and melody of music. The strip is segmented into three different parts with the mids and highs of the music being displayed in the middle of the strip and the bass of the music on either end of the mids and highs. It uses the MSGEQ7 chipset to take in 7 different audio frequencies and sends that data to an Arduino Uno, which then changes the brightness of the LED strip accordingly. 

Currently trying to use Karn's Algorithm so that the volume of the music does not affect the brightness of the LED strip. So if music is being played at a low volume level, the LED strip will still be as bright as if the volume was all the way up. 
