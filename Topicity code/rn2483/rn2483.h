/*
 * A library for controlling a Microchip RN2483 LoRa radio.
 *
 * @Author JP Meijers
 * @Date 18/12/2015
 *
 */

#ifndef rn2483_h
#define rn2483_h

#include "Arduino.h"
#include <SoftwareSerial.h>

class rn2483
{
  public:
    /*
     * A simplified constructor taking only a SoftwareSerial object.
     * It is assumed that LoRa WAN will be used.
     * The serial port should already be initialised when initialising this library.
     */
    rn2483(SoftwareSerial& serial);

    /*
     * A simplified constructor taking only a HardwareSerial object.
     * It is assumed that LoRa WAN will be used.
     * The serial port should already be initialised when initialising this library.
     */
    rn2483(HardwareSerial& serial);

    /*
     * Transmit the correct sequence to the rn2483 to trigger its autobauding feature.
     * After this operation the rn2483 should communicate at the same baud rate than us.
     */
    void autobaud();

    /*
     * Initialise the RN2483 and join the LoRa network (if applicable).
     */
    void init();

    /*
     * Initalise the radio in raw mode. LoRa WAN is in this case not used.
     * Note: This library only supports raw transmission, and therefore can not
     * receive any raw data. This might be added in the future.
     */
    void initRaw();

    /*
     * Explicitly join the LoRa WAN network. This is the same than to call init().
     */
    void initWan();

    /*
     * Initialise the RN2483 and join The Things Network. 
     * This ignores your previous choice to use or not use the LoRa WAN.
     */
    void initTTN(String addr);

    /*
     * Transmit the provided data. The data is hex-encoded by this library,
     * so plain text can be provided.
     * If neither LoRa WAN nor TTN is used, this function will do a raw radio transmit.
     * If LoRa WAN is used, it is assumed that a confirmed transmission should be done.
     * If TTN is used an unconfirmed transmission is made.
     */
    void tx(String);

    /*
     * Do a confirmed transmission via LoRa WAN.
     * Note: Only use this function if LoRa WAN is used.
     */
    void txCnf(String);

    /*
     * Do an unconfirmed transmission via LoRa WAN.
     * Note: Only use this function if either LoRa WAN or TTN is used.
     */
    void txUncnf(String);



  private:
    Stream& _serial;

    //Flags to switch code paths. Default is to use WAN (join OTAA)
    bool _use_wan=false;
    bool _use_raw=false;
    bool _use_ttn=true;

    //The default address to use on TTN if no address is defined. 
    //This one falls in the "testing" address space.
    String _ttnAddr = "03FFBEEF";

    //if the hardware id can not be obtained from the module, 
    // use this deveui for LoRa WAN
    String _default_deveui = "02982215";

    //the appeui to use for LoRa WAN
    String _appeui = "02982215";

    //the appkey to use for LoRa WAN
    String _appkey = "replace with your own app key";

    //the frequency to use, in Hertz, if the radio is used in raw mode (without LoRa WAN)
    String _raw_frequency = "869100000";

    void txRaw(String);
    bool txData(String, String);

    void sendEncoded(String);
    String base16encode(String);
    String base16decode(String);
};

#endif
