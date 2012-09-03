//============================================================================+
//
// $HeadURL: $
// $Revision: $
// $Date:  $
// $Author: $
//
/// \brief multiwii telemetry interface
///
/// \file
/// - Protocol summary:
/// \code
///   byte | character | meaning
/// -------+-----------+------------------------
///    1   |   '$'     | start character
///    2   |   'M'     | MultiWii protocol
///    3   |   '<'     | direction of message
///    4   |    n      | payload size, max 255
///   ...  |           | payload, empty if n = 0
///   n+5  |    c      | checksum
/// -------+-----------+------------------------
/// \endcode
/// - for more info on protocol see:
/// \n         http://www.multiwii.com/forum/viewtopic.php?f=8&t=1516
/// - for Arduino implementation see:
/// \n         https://github.com/wertarbyte/multiwii-firmware/blob/master/Serial.ino
/// - for compatible GUI see:
/// \n WinGUI
/// \n         http://www.multiwii.com/forum/viewtopic.php?f=8&t=1229
/// \n mwGui
/// \n         http://www.multiwii.com/forum/viewtopic.php?f=8&t=1791
/// \n Android
/// \n         http://www.multiwii.com/forum/viewtopic.php?f=6&t=133
/// \n         https://play.google.com/store/apps/details?id=net.xrotor.andmultiwiiconf
/// \n         https://play.google.com/store/apps/details?id=net.loide.games.bicopter
/// \n         https://play.google.com/store/apps/details?id=com.naze32.configurator
/// - for VT100 terminal type addition see
/// \n         http://www.multiwii.com/forum/viewtopic.php?f=7&t=1096
/// - transmission order and scale factors of values for MSP_SET_PID command :
///    -  1 roll P          10
///    -  2 roll I          1000
///    -  3 roll D          1
///    -  4 pitch P         10
///    -  5 pitch I         1000
///    -  6 pitch D         1
///    -  7 yaw P           10
///    -  8 yaw I           1000
///    -  9 yaw D           1
///    - 10 altitude P      10
///    - 11 altitude I      1000
///    - 12 altitude D      1
///    - 13 position P      100
///    - 14 position I      100
///    - 15 position D      ? (not used)
///    - 16 position rate P 10
///    - 17 position rate I 100
///    - 18 position rate D 1000
///    - 19 navigation P    10
///    - 20 navigation I    100
///    - 21 navigation D    1000
///    - 22 level P         10
///    - 23 level I         100
///    - 24 level D         1
///    - 25 magnetic P      10
///    - 26 magnetic I      ? (not used)
///    - 27 magnetic D      ? (not used)
///    - 28 velocity P      10
///    - 29 velocity I      100
///    - 30 velocity D      1
///
//  Change added hexadecimal values of MultiWii protocol codes
//
//============================================================================*/

// ---- Include Files -------------------------------------------------------

#include "misc.h"

#include "config.h"
#include "nav.h"
#include "DCM.h"
#include "servodriver.h"
#include "usart1driver.h"
#include "multiwii.h"

/*--------------------------------- Definitions ------------------------------*/

#ifdef    VAR_STATIC
#   undef VAR_STATIC
#endif
#define   VAR_STATIC static

#ifdef    VAR_GLOBAL
#   undef VAR_GLOBAL
#endif
#define   VAR_GLOBAL

#define PAYLOAD_SIZE        32  //!< maximum size of payload

#define VERSION             0	//!< multiwii version
#define MULTITYPE           0	//!< type of multicopter
#define MSP_VERSION         0   //!< Multiwii Serial Protocol 0

/* outgoing messages */
#define MSP_IDENT           100 //!< 64 multitype + multiwii version + protocol version + capability variable
#define MSP_STATUS          101 //!< 65 cycletime & errors_count & sensor present & box activation
#define MSP_RAW_IMU         102 //!< 66 raw IMU data, 9 DOF
#define MSP_SERVO           103 //!< 67 8 servos
#define MSP_MOTOR           104 //!< 68 8 motors
#define MSP_RC              105 //!< 69 8 rc channels
#define MSP_RAW_GPS         106 //!< 6A fix, numsat, lat, lon, alt, speed
#define MSP_COMP_GPS        107 //!< 6B distance home, direction home
#define MSP_ATTITUDE        108 //!< 6C roll, pitch, heading
#define MSP_ALTITUDE        109 //!< 6D 1 altitude
#define MSP_BAT             110 //!< 6E vbat, powermetersum
#define MSP_RC_TUNING       111 //!< 6F rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define MSP_PID             112 //!< 70 up to 16 PID
#define MSP_BOX             113 //!< 71 up to 16 checkbox
#define MSP_MISC            114 //!< 72 powermeter trig + 8 free for future use
#define MSP_MOTOR_PINS      115 //!< 73 which pins are in use for motors & servos, for GUI
#define MSP_BOXNAMES        116 //!< 74 the aux switch names
#define MSP_PIDNAMES        117 //!< 75 the PID names
#define MSP_WP              118 //!< 76 get a WP, WP# is in the payload, returns (WP#, lat, lon, alt, flags) WP#0-home, WP#16-poshold
#define MSP_HEADING         125 //!< 7D headings and MAG configuration
#define MSP_DEBUGMSG        253 //!< FD debug string buffer
#define MSP_DEBUG           254 //!< FE debug1, debug2, debug3, debug4

/* incoming messages */
#define MSP_SET_RAW_RC      200 //!< C8 8 rc chan
#define MSP_SET_RAW_GPS     201 //!< C9 fix, numsat, lat, lon, alt, speed
#define MSP_SET_PID         202 //!< CA set up to 16 PID
#define MSP_SET_BOX         203 //!< CB set up to 16 checkbox
#define MSP_SET_RC_TUNING   204 //!< CC rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define MSP_ACC_CALIBRATION 205 //!< CD calibrate accelerometers
#define MSP_MAG_CALIBRATION 206 //!< CE calibrate magnetometers
#define MSP_SET_MISC        207 //!< CF powermeter trig + 8 free for future use
#define MSP_RESET_CONF      208 //!< D0 reset configuration
#define MSP_WP_SET          209 //!< D1 sets a given WP (WP#, lat, lon, alt, flags)

#define MSP_EEPROM_WRITE    250 //!< FA save configuration to eeprom

/*----------------------------------- Macros ---------------------------------*/

/*-------------------------------- Enumerations ------------------------------*/

/// Status of multiwii header parser
typedef enum E_STATE {
    IDLE,           //!< waiting
    HEADER_START,   //!< start of header, $ received
    HEADER_M,       //!< multiwii protocol, M received
    HEADER_ARROW,   //!< direction of message, > or < received
    HEADER_SIZE,    //!< size of payload
    HEADER_CMD,     //!< command identifier
  } enum_status_t;

/*----------------------------------- Types ----------------------------------*/

/*---------------------------------- Constants -------------------------------*/

/// Names of multiwii PID
const uint8_t pidnames[] =
  "ROLL;"
  "PITCH;"
  "YAW;"
  "ALT;"
  "Pos;"
  "PosR;"
  "NavR;"
  "LEVEL;"
  "MAG;"
  "VEL;"
;
/*---------------------------------- Globals ---------------------------------*/

/*----------------------------------- Locals ---------------------------------*/

VAR_STATIC enum_status_t MSP_Status = IDLE;         //!< status of communication
VAR_STATIC uint8_t MSP_Checksum;                    //!< message checksum
VAR_STATIC uint8_t MSP_Command;                     //!< command identifier
VAR_STATIC uint8_t MSP_Size;                        //!< payload size
VAR_STATIC uint8_t MSP_Index;                       //!< payload index
VAR_STATIC uint8_t MSP_Buffer[48];                  //!< payload buffer
VAR_STATIC float fPIDGain[TEL_GAIN_NUMBER] = {
    PITCH_KP,                                       //!< default pitch kp
    PITCH_KI,                                       //!< default pitch ki
    ROLL_KP,                                        //!< default roll kp
    ROLL_KI,                                        //!< default roll ki
    NAV_KP,                                         //!< default direction kp
    NAV_KI,                                         //!< default direction ki
    0.0f,                                           //!< dummy placeholder
    0.0f                                            //!< dummy placeholder
};                                                  //!< gains for PID loops

/*--------------------------------- Prototypes -------------------------------*/

/*---------------------------------- Functions -------------------------------*/

///----------------------------------------------------------------------------
///
/// \brief   reads a byte from message payload
/// \return  byte read
/// \param   -
/// \remarks -
///
///----------------------------------------------------------------------------
static uint8_t read8(void) {
   return (MSP_Buffer[MSP_Index++] & 0xFF);
}

///----------------------------------------------------------------------------
///
/// \brief   reads a word from message payload
/// \return  word read
/// \param   -
/// \remarks -
///
///----------------------------------------------------------------------------
static uint16_t read16(void) {
  uint16_t t = read8();
  t += (uint16_t)read8() << 8;
  return t;
}

///----------------------------------------------------------------------------
///
/// \brief   reads a long word from message payload
/// \return  long word read
/// \param   -
/// \remarks -
///
///----------------------------------------------------------------------------
static uint32_t read32(void) {
  uint32_t t = read16();
  t += (uint32_t)read16() << 16;
  return t;
}

///----------------------------------------------------------------------------
///
/// \brief   Append a byte to outgoing message
/// \return  -
/// \param   a = message byte
/// \remarks
///
///----------------------------------------------------------------------------
static void MSP_Append_8(uint8_t a) {
  USART1_Putch(a);
  MSP_Checksum ^= a;
}

///----------------------------------------------------------------------------
///
/// \brief   Append a word to outgoing message
/// \return  -
/// \param   a = message word
/// \remarks -
///
///----------------------------------------------------------------------------
static void MSP_Append_16(int16_t a) {
  MSP_Append_8((a     ) & 0xFF);
  MSP_Append_8((a >> 8) & 0xFF);
}

///----------------------------------------------------------------------------
///
/// \brief   Append a long word to outgoing message
/// \return  -
/// \param   a = message long word
/// \remarks -
///
///----------------------------------------------------------------------------
static void MSP_Append_32(uint32_t a) {
  MSP_Append_8((a      ) & 0xFF);
  MSP_Append_8((a >>  8) & 0xFF);
  MSP_Append_8((a >> 16) & 0xFF);
  MSP_Append_8((a >> 24) & 0xFF);
}

///----------------------------------------------------------------------------
///
/// \brief   Append a null terminated string to outgoing message
/// \return  -
/// \param   s = pointer to null terminated string
/// \remarks -
///
///----------------------------------------------------------------------------
static void MSP_Append_Name(const uint8_t* s) {
  while (*s != 0) {
  	MSP_Append_8(*s++);
  }
}

///----------------------------------------------------------------------------
///
/// \brief   Initialize multi Wii response message
/// \return  -
/// \param   length = message length
/// \remarks -
///
///----------------------------------------------------------------------------
static void __inline MSP_Init_Response(uint8_t length) {
  MSP_Append_8('$');            // header
  MSP_Append_8('M');            // multiwii protocol
  MSP_Append_8('>');            // outgoing message
  MSP_Checksum = 0;             // clear checksum
  MSP_Append_8(length);         // message length
  MSP_Append_8(MSP_Command);    // command
}

///----------------------------------------------------------------------------
///
/// \brief   Initialize multi Wii error message
/// \return  -
/// \param   length = message length
/// \remarks -
///
///----------------------------------------------------------------------------
static void __inline MSP_Init_Error(uint8_t length) {
  MSP_Append_8('$');            // header
  MSP_Append_8('M');            // multiwii protocol
  MSP_Append_8('!');            // error message
  MSP_Checksum = 0;             // clear checksum
  MSP_Append_8(length);         // message length
  MSP_Append_8(MSP_Command);    // command
}

///----------------------------------------------------------------------------
///
/// \brief   parses multiwii commands
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void MSP_Parse_Command( void ) {
  uint8_t i;

  switch (MSP_Command) {

    case MSP_SET_PID:                   // set PID values
     i = TEL_ROLL_KP;                   // start with first gain
     fPIDGain[i++] = read8() / 10.0f;   // roll P
     fPIDGain[i++] = read8() / 1000.0f; // roll I
     (void)read8();                     // skip roll D
     fPIDGain[i++] = read8() / 10.0f;   // pitch P
     fPIDGain[i++] = read8() / 1000.0f; // pitch I
     (void)read32();                    // skip pitch D and yaw P, I, D
     fPIDGain[i++] = read8() / 10.0f;   // alt P
     fPIDGain[i++] = read8() / 1000.0f; // alt I
     (void)read32();                    // skip alt D and pos P, I, D
     (void)read16();                    // skip pos rate P, I
     (void)read8();                     // skip pos rate D
     fPIDGain[i++] = read8() / 10.0f;   // nav P
     fPIDGain[i++] = read8() / 100.0f;  // nav I
     MSP_Init_Response(0);              // initialize response
     break;

    case MSP_IDENT:                     // requested identification
     MSP_Init_Response(7);              // initialize response
     MSP_Append_8(VERSION);             // append multiwii version
     MSP_Append_8(MULTITYPE);           // append type of multicopter
     MSP_Append_8(MSP_VERSION);         // MultiWii Serial Protocol Version
     MSP_Append_32(0);                  // append "capability"
     break;

    case MSP_STATUS:                    // requested status
     MSP_Init_Response(10);             //
     MSP_Append_16(0);                  // cycleTime, time needed to complete a loop
     MSP_Append_16(0);                  // i2c_errors_count, I2C errors
     MSP_Append_16(ACC | BARO << 1 |    // available sensors
                   MAG << 2 | GPS << 3 |
                   SONAR << 4 );
     MSP_Append_32(0                    // flags and options
                                        // f.ACC_MODE      << BOXACC |
                                        // f.BARO_MODE     << BOXBARO |
                                        // f.MAG_MODE      << BOXMAG |
                                        // f.ARMED         << BOXARM |
                                        // f.GPS_HOME_MODE << BOXGPSHOME |
                                        // f.GPS_HOLD_MODE << BOXGPSHOLD |
                                        // f.HEADFREE_MODE << BOXHEADFREE |
                                        // f.PASSTHRU_MODE << BOXPASSTHRU |
                                        // rcOptions[BOXCAMSTAB]  << BOXCAMSTAB |
                                        // rcOptions[BOXCAMTRIG]  << BOXCAMTRIG |
                                        // rcOptions[BOXBEEPERON] << BOXBEEPERON |
                                        // rcOptions[BOXLEDMAX]   << BOXLEDMAX |
                                        // rcOptions[BOXLLIGHTS]  << BOXLLIGHTS |
                                        // rcOptions[BOXHEADADJ]  << BOXHEADADJ
				  );
     break;

    case MSP_ATTITUDE:                  // requested attitude
     MSP_Init_Response(8);              // initialize response
     for (i = 0; i < 2; i++)			// append presumably roll, pitch, yaw
         MSP_Append_16(0);              // angle[i], roll pitch
     MSP_Append_16(0);                  // heading, heading
     MSP_Append_16(0);	                // headFreeModeHold, ?
     break;

    case MSP_HEADING:                   // requested heading
     MSP_Init_Response(7);              // initialize response
     MSP_Append_8(0                     // f.MAG_MODE      << 0 | mag stabilization mode
                                        // f.HEADFREE_MODE << 1   headfree mode
				  );
     MSP_Append_16(0);                  // heading, heading
     MSP_Append_16(0);                  // magHold, magnetic orientation to hold
     MSP_Append_16(0);                  // headFreeModeHold, ?
     break;

    case MSP_ALTITUDE:                  // requested altitude
     MSP_Init_Response(4);              // initialize response
     MSP_Append_32(0);                  // EstAlt, estimated barometric altitude
     break;

    case MSP_PID:                                       // requested PID values
     MSP_Init_Response(30);                             // initialize response
     i = TEL_ROLL_KP;                                   // start with first gain
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 10.0f));    // roll P
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 1000.0f));  // roll I
     MSP_Append_8(0);                                   // roll D: skip
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 10.0f));    // pitch P
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 1000.0f));  // pitch I
     MSP_Append_32(0);                                  // skip pitch D, yaw P, I, D
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 10.0f));    // alt P
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 1000.0f));  // alt I
     MSP_Append_32(0);                                  // skip alt D and pos P, I, D
     MSP_Append_16(0);                                  // skip pos rate P, I
     MSP_Append_8(0);                                   // skip pos rate D
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 10.0f));    // nav P
     MSP_Append_8((uint8_t)(fPIDGain[i++] * 100.0f));   // nav I
     MSP_Append_32(0);                                  // skip nav D and level P, I, D
     MSP_Append_32(0);                                  // skip mag P, I, D and velocity P
     MSP_Append_16(0);                                  // skip velocity I, D
     break;

    case MSP_PIDNAMES:                  // requested PID names
     MSP_Init_Response(sizeof(pidnames)); // initialize response
     MSP_Append_Name(pidnames);         // append PID names
     break;

#if defined(USE_MSP_WP)
   case MSP_WP:                         // requested waypoint
    i = read8();                        // get the number of required wp
    MSP_Init_Response(12);              // initialize response
    if (i == 0) {                       // home position (waypoint #0)
      MSP_Append_8(0);                  // append waypoint number
      MSP_Append_32(GPS_home[LAT]);     // append home latitude
      MSP_Append_32(GPS_home[LON]);     // append home longitude
      MSP_Append_16(0);                 // altitude will come here
      MSP_Append_8(0);                  // nav flag will come here
    } else if (i == 16) {               // hold position (waypoint #16)
      MSP_Append_8(16);                 // append waypoint number
      MSP_Append_32(GPS_hold[LAT]);     // append hold latitude
      MSP_Append_32(GPS_hold[LON]);     // append hold longitude
      MSP_Append_16(0);                 // altitude will come here
      MSP_Append_8(0);                  // nav flag will come here
    }
    break;
#endif

   case MSP_RESET_CONF:                 // reset configuration
     MSP_Init_Response(0);              // initialize response
     break;

   case MSP_EEPROM_WRITE:               // write parameters to eeprom
     MSP_Init_Response(0);              // initialize response
     break;

   case MSP_DEBUG:                      // debug message
     MSP_Init_Response(0);              // initialize response
     break;

   default :                            // indicate error "$M!"
     MSP_Init_Error(0);                 // initialize error message
     break;
  }
  MSP_Append_8(MSP_Checksum);           // append message checksum
  USART1_Transmit();                    // transmit
}

///----------------------------------------------------------------------------
///
/// \brief   manages multi wii serial protocol reception
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void MSP_Receive( void ) {
  uint8_t c;

  while (USART1_Getch(&c)) {                // received another character
    switch (MSP_Status) {
        case IDLE:                          // idle
            if (c == '$') {                 // message start
                MSP_Status = HEADER_START;  //
            } else {                        //
                MSP_Status = IDLE;          //
            }
            break;

        case HEADER_START:                  // message started
            if (c == 'M') {                 // Multiwii protocol
                MSP_Status = HEADER_M;      //
            } else {                        //
                MSP_Status = IDLE;          //
            }
            break;

        case HEADER_M:                      //
            if (c == '<') {                 // incoming message
                MSP_Status = HEADER_ARROW;  //
            } else {                        //
                MSP_Status = IDLE;          //
            }
            break;

        case HEADER_ARROW:                  //
            if (c > PAYLOAD_SIZE) {         // payload size too big
                MSP_Status = IDLE;          // ignore rest of message
            } else {                        // payload size fits
                MSP_Size = c;               // save payload size
                MSP_Index = 0;              // clear payload index
                MSP_Checksum = 0;           // clear message checksum
                MSP_Checksum ^= c;          // start computing checksum
                MSP_Status = HEADER_SIZE;   //
            }
            break;

        case HEADER_SIZE:                   //
            MSP_Command = c;                // save command byte
            MSP_Checksum ^= c;              // keep computing checksum
            MSP_Status = HEADER_CMD;        //
            break;                          //

        case HEADER_CMD:                    //
            if (MSP_Index < MSP_Size) {     // not all expected bytes have been received
                MSP_Checksum ^= c;          // compute checksum
                MSP_Buffer[MSP_Index++] = c;// store received byte
            } else if (MSP_Checksum == c) { // calculated and transferred checksums match
                MSP_Index = 0;              // clear payload index
                MSP_Parse_Command();        // valid packet, evaluate it
                MSP_Status = IDLE;          //
            }
            break;

        default :
            MSP_Status = IDLE;              //
            break;
    }
  }
}

///----------------------------------------------------------------------------
///
/// \brief   interface to simulator data : true air speed
/// \return  true air speed
/// \remarks -
///
///----------------------------------------------------------------------------
float Telemetry_Get_Speed(void) {
    return 0.0f;//fTrueAirSpeed;
}

///----------------------------------------------------------------------------
///
/// \brief   interface to simulator data : altitude
/// \return  altitude
/// \remarks -
///
///----------------------------------------------------------------------------
float Telemetry_Get_Altitude(void) {
    return 0.0f;//fAltitude;
}

///----------------------------------------------------------------------------
///
/// \brief   Get PID gain
/// \param   gain, gain number
/// \return  gain value
/// \remarks -
///
///----------------------------------------------------------------------------
float Telemetry_Get_Gain(telEnum_Gain gain) {
    if (gain < TEL_GAIN_NUMBER) {
        return fPIDGain[gain];
    } else {
        return 1.0f;
    }
}

///----------------------------------------------------------------------------
///
/// \brief   Get sensor value
/// \param   piSensor = pointer to sensor data array
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void Telemetry_Get_Sensors(int16_t * piSensor) {
/*
    uint8_t j;
    for (j = 0; j < 6; j++) {
        *piSensor++ = (int16_t)fSensor[j];
    }
*/
}
