/*
 * A library for controlling a Microchip RN2483 LoRa radio.
 *
 * @Author JP Meijers
 * @Date 18/12/2015
 *
 */

#include "Arduino.h"
#include "rn2483.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
}

/*
  @param serial Needs to be an already opened stream to write to and read from.
*/
rn2483::rn2483(SoftwareSerial& serial):
_serial(serial)
{
  Serial.setTimeout(2000);
}

rn2483::rn2483(HardwareSerial& serial):
_serial(serial)
{
  Serial.setTimeout(2000);
}

void rn2483::autobaud()
{
  String response = "";
  while (response=="")
  {
    delay(1000);
    Serial.write((byte)0x00);
    Serial.write(0x55);
    Serial.println();
    Serial.println("sys get ver");
    response = Serial.readStringUntil('\n');
  }
}

void rn2483::init()
{
  if(_use_wan) initWan();
  else if(_use_raw) initRaw();
  else if(_use_ttn) initTTN(_ttnAddr);
  else 
  {
    _use_wan = true;
    initWan(); //default is to use WAN
  }
}

void rn2483::initRaw()
{
  _use_raw = true;

  //clear serial buffer
  while(Serial.read() != -1);

  Serial.println("mac pause");
  Serial.readStringUntil('\n');
  Serial.println("radio set mod lora");
  Serial.readStringUntil('\n');
  Serial.println("radio set freq "+_raw_frequency);
  Serial.readStringUntil('\n');
  Serial.println("radio set pwr 14");
  Serial.readStringUntil('\n');
  Serial.println("radio set sf sf12"); //sf7 is fastest, sf12 is furthest
  Serial.readStringUntil('\n');
  Serial.println("radio set afcbw 41.7");
  Serial.readStringUntil('\n');
  Serial.println("radio set rxbw 25");
  Serial.readStringUntil('\n');
  Serial.println("radio set bitrate 50000");
  Serial.readStringUntil('\n');
  Serial.println("radio set fdev 25000");
  Serial.readStringUntil('\n');
  Serial.println("radio set prlen 8");
  Serial.readStringUntil('\n');
  Serial.println("radio set crc on");
  Serial.readStringUntil('\n');
  Serial.println("radio set iqi off");
  Serial.readStringUntil('\n');
  Serial.println("radio set cr 4/5");
  Serial.readStringUntil('\n');
  Serial.println("radio set wdt 60000");
  Serial.readStringUntil('\n');
  Serial.println("radio set sync 12");
  Serial.readStringUntil('\n');
  Serial.println("radio set bw 500"); //500kHz max, 250kHz, 125kHz min
  Serial.readStringUntil('\n');
}

void rn2483::initWan()
{
  _use_wan = true;

  //clear serial buffer
  while(Serial.read() != -1);

  Serial.println("sys get hweui");
  String addr = Serial.readStringUntil('\n');
  addr.trim();
  
  Serial.println("mac reset 868");
  Serial.readStringUntil('\n');
  Serial.println("mac set appeui "+_appeui);
  Serial.readStringUntil('\n');
  Serial.println("mac set appkey "+_appkey);
  Serial.readStringUntil('\n');

  if(addr!="" && addr.length() == 16)
  {
    Serial.println("mac set deveui "+addr);
  }
  else 
  {
    Serial.println("mac set deveui "+_default_deveui);
  }
  Serial.readStringUntil('\n');
  Serial.println("mac set pwridx 1");
  Serial.readStringUntil('\n');
  Serial.println("mac set adr on");
  Serial.readStringUntil('\n');
  // Serial.println("mac set retx 10");
  // Serial.readStringUntil('\n');
  // Serial.println("mac set linkchk 60");
  // Serial.readStringUntil('\n');
  // Serial.println("mac set ar on");
  // Serial.readStringUntil('\n');
  Serial.setTimeout(30000);
  Serial.println("mac save");
  Serial.readStringUntil('\n');
  bool joined = false;

  for(int i=0; i<10 && !joined; i++)
  {
    Serial.println("mac join otaa");
    Serial.readStringUntil('\n');
    String receivedData = Serial.readStringUntil('\n');

    if(receivedData.startsWith("accepted"))
    {
      joined=true;
      delay(1000);
    }
    else
    {
      delay(1000);
    }
  }
  Serial.setTimeout(2000);
}

void rn2483::initTTN(String addr)
{
  _use_ttn = true;
  _ttnAddr = addr;

  //clear serial buffer
  while(Serial.read() != -1);
  
  Serial.println("mac reset 868");
  Serial.readStringUntil('\n');

  Serial.println("mac set rx2 3 869525000");
  Serial.readStringUntil('\n');

  Serial.println("mac set nwkskey 2B7E151628AED2A6ABF7158809CF4F3C");
  Serial.readStringUntil('\n');
  Serial.println("mac set appskey 2B7E151628AED2A6ABF7158809CF4F3C");
  Serial.readStringUntil('\n');

  Serial.println("mac set devaddr "+addr);
  Serial.readStringUntil('\n');

  Serial.println("mac set adr off");
  Serial.readStringUntil('\n');
  Serial.println("mac set ar off");
  Serial.readStringUntil('\n');

  Serial.println("mac set pwridx 1"); //1=max, 5=min
  Serial.readStringUntil('\n');
  Serial.println("mac set dr 5"); //0= min, 7=max
  Serial.readStringUntil('\n');

  Serial.setTimeout(60000);
  Serial.println("mac save");
  Serial.readStringUntil('\n');
  Serial.println("mac join abp");
  Serial.readStringUntil('\n');
  Serial.readStringUntil('\n');
  Serial.setTimeout(2000);
  delay(1000);
}

void rn2483::tx(String data)
{
  if(_use_wan) txCnf(data); //if not specified use acks on WAN
  else if(_use_ttn) txUncnf(data); //ttn does not have downstream, thus no acks
  else if(_use_raw) txRaw(data);
  else txUncnf(data); //we are unsure which mode we're in. Better not to wait for acks.
}

void rn2483::txRaw(String data)
{
  txData("radio tx ", data);
}

void rn2483::txCnf(String data)
{
  txData("mac tx cnf 1 ", data);
}

void rn2483::txUncnf(String data)
{
  txData("mac tx uncnf 1 ", data);
}

bool rn2483::txData(String command, String data)
{
  bool send_success = false;
  uint8_t busy_count = 0;
  uint8_t retry_count = 0;

  while(!send_success)
  {
    //retransmit a maximum of 10 times
    retry_count++;
    if(retry_count>10)
    {
      return false;
    }

    Serial.print(command);
    sendEncoded(data);
    Serial.println();
    String receivedData = Serial.readStringUntil('\n');

    if(receivedData.startsWith("ok"))
    {
      Serial.setTimeout(30000);
      receivedData = Serial.readStringUntil('\n');
      Serial.setTimeout(2000);
	  
	  
      
      if(receivedData.startsWith("mac_tx_ok"))
      {
        //SUCCESS!!
        send_success = true;
		//Serial.println("Succesful message send");
        return true;
      }
	
      else if(receivedData.startsWith("mac_rx"))
      {
		//we received data downstream
        //TODO: handle received data
		Serial.println("data received");
        send_success = true;
        return true;
      }

      else if(receivedData.startsWith("mac_err"))
      {
        init();
      }

      else if(receivedData.startsWith("invalid_data_len"))
      {
        //this should never happen if the prototype worked
        send_success = true;
        return false;
      }

      else if(receivedData.startsWith("radio_tx_ok"))
      {
        //SUCCESS!!
        send_success = true;
        return true;
      }

      else if(receivedData.startsWith("radio_err"))
      {
        //This should never happen. If it does, something major is wrong.
        init();
      }

      else
      {
        //unknown response
        //init();
      }
    }

    else if(receivedData.startsWith("invalid_param"))
    {
      //should not happen if we typed the commands correctly
      send_success = true;
      return false;
    }

    else if(receivedData.startsWith("not_joined"))
    {
      init();
    }

    else if(receivedData.startsWith("no_free_ch"))
    {
      //retry
      delay(1000);
    }

    else if(receivedData.startsWith("silent"))
    {
      init();
    }

    else if(receivedData.startsWith("frame_counter_err_rejoin_needed"))
    {
      init();
    }

    else if(receivedData.startsWith("busy"))
    {
      busy_count++;
      if(busy_count>=10)
      {
        init();
      }
      else
      {
        delay(1000);
      }
    }

    else if(receivedData.startsWith("mac_paused"))
    {
      init();
    }

    else if(receivedData.startsWith("invalid_data_len"))
    {
      //should not happen if the prototype worked
      send_success = true;
      return false;
    }

    else
    {
      //unknown response after mac tx command
      init();
    }
  }

  return false; //should never reach this
}

void rn2483::sendEncoded(String input)
{
  char working;
  char buffer[3];
  for(int i=0; i<input.length(); i++)
  {
    working = input.charAt(i);
    sprintf(buffer, "%02x", int(working));
    Serial.print(buffer);
  }
}

String rn2483::base16encode(String input)
{
  char charsOut[input.length()*2+1];
  char charsIn[input.length()+1];
  input.trim();
  input.toCharArray(charsIn, input.length()+1);
  
  int i = 0;
  for(i = 0; i<input.length()+1; i++)
  {
    if(charsIn[i] == '\0') break;
    
    int value = int(charsIn[i]);
    
    char buffer[3];
    sprintf(buffer, "%02x", value);
    charsOut[2*i] = buffer[0];
    charsOut[2*i+1] = buffer[1];
  }
  charsOut[2*i] = '\0';
  String toReturn = String(charsOut);
  return toReturn;
}

String rn2483::base16decode(String input)
{
  char charsIn[input.length()+1];
  char charsOut[input.length()/2+1];
  input.trim();
  input.toCharArray(charsIn, input.length()+1);
  
  int i = 0;
  for(i = 0; i<input.length()/2+1; i++)
  {
    if(charsIn[i*2] == '\0') break;
    if(charsIn[i*2+1] == '\0') break;
    
    char toDo[2];
    toDo[0] = charsIn[i*2];
    toDo[1] = charsIn[i*2+1];
    int out = strtoul(toDo, 0, 16);
    
    if(out<128)
    {
      charsOut[i] = char(out);
    }
  }
  charsOut[i] = '\0';
  return charsOut;
}