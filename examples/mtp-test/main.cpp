#include "Arduino.h"

#if 1
  #include "MTP.h"

  MTPStorage_SD storage;
  MTPD       mtpd(&storage);
#endif

void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
    Serial.println(txt);
    to=millis();
  }
}

void setup()
{ 
  while(!Serial);
  Serial.println("MTP test");  Serial.flush();

  storage.init();
  SD.ls();

}

void loop()
{ 
  mtpd.loop();

  logg(1000,"loop");

}