
#include <MySensor.h>  
#include <SPI.h>
#include <SimpleTimer.h>
#include <avr/wdt.h>
#include "EmonLib.h"

 #define NDEBUG                        // enable local debugging information

#define NODE_ID 195 //На даче поменять на 101

/******************************************************************************************************/
/*                               				Сенсоры												  */
/******************************************************************************************************/

#define CHILD_ID_VOLTMETER				  60  
#define CHILD_ID_AMPERMETER				  61 
#define CHILD_ID_WATTMETER				  62 
#define CHILD_ID_RMSWATTMETER       63
//#define CHILD_ID_POWERFACTOR        64 

#define CHILD_ID_WATTMETER_KWH			  65 

#define REBOOT_CHILD_ID                       100
#define RECHECK_SENSOR_VALUES                 101 
#define NIGHTMODE_CHILD_ID                    105

/******************************************************************************************************/
/*                               				IO												                                    */
/******************************************************************************************************/

#define VOLTMETER_PIN              	A0      
#define AMPERMETER_PIN		      	A1
#define WATTMETER_PIN				3 


/*****************************************************************************************************/
/*                               				Common settings									      */
/******************************************************************************************************/
#define RADIO_RESET_DELAY_TIME 20 //Задержка между сообщениями
#define MESSAGE_ACK_RETRY_COUNT 5  //количество попыток отсылки сообщения с запросом подтверждения
#define DATASEND_DELAY  10

boolean gotAck=false; //подтверждение от гейта о получении сообщения 
int iCount = MESSAGE_ACK_RETRY_COUNT;

boolean boolRecheckSensorValues = false;
boolean boolNightMode = false;

int cycleCounter = 0;

float lastVolts = 0;
float lastAmpers = 0;
float lastWatts = 0;
float lastRMSWatts = 0;

EnergyMonitor emon;             // Create an instance

SimpleTimer checkVoltAmpers;

MyMessage VoltageMsg(CHILD_ID_VOLTMETER, V_VOLTAGE);
MyMessage AmpersMsg(CHILD_ID_AMPERMETER, V_CURRENT);
MyMessage WattMsg(CHILD_ID_WATTMETER, V_WATT);
MyMessage RMSWattMsg(CHILD_ID_RMSWATTMETER, V_WATT);
//MyMessage KwHMsg(CHILD_ID_WATTMETER_KWH, V_KWH);




MySensor gw;

void setup()  
{ 


     #ifdef NDEBUG
       Serial.println();
       Serial.println("Begin setup");
     #endif

pinMode(A0, INPUT);
pinMode(A1, INPUT);
pinMode(3, INPUT);


  emon.voltage(VOLTMETER_PIN, /*234.26*/ 593.589, /*1*/ 1.7);  // Voltage: input pin, calibration, phase_shift
 emon.current(AMPERMETER_PIN, 27.5 /*30*/);       // Current: input pin, calibration.

  // Startup and initialize MySensors library. Set callback for incoming messages. 
  gw.begin(incomingMessage, NODE_ID, false);
  gw.wait(RADIO_RESET_DELAY_TIME);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Power box Energy Meter", "1.0");

	//voltage sensor
    gw.wait(RADIO_RESET_DELAY_TIME);
  	gw.present(CHILD_ID_VOLTMETER, S_MULTIMETER); 

	//current sensor
    gw.wait(RADIO_RESET_DELAY_TIME);
  	gw.present(CHILD_ID_AMPERMETER, S_MULTIMETER); 

	//watt sensor
    gw.wait(RADIO_RESET_DELAY_TIME);
  	gw.present(CHILD_ID_WATTMETER, S_POWER); 

  //RMS watt sensor
    gw.wait(RADIO_RESET_DELAY_TIME);
    gw.present(CHILD_ID_RMSWATTMETER, S_POWER); 

  	//watt sensor
    //gw.wait(RADIO_RESET_DELAY_TIME);
  	//gw.present(CHILD_ID_WATTMETER_KWH, S_POWER); 

    //reboot sensor command
    gw.wait(RADIO_RESET_DELAY_TIME);
    gw.present(REBOOT_CHILD_ID, S_BINARY); //, "Reboot node sensor", true); 

    //reget sensor values
    gw.wait(RADIO_RESET_DELAY_TIME);
  	gw.present(RECHECK_SENSOR_VALUES, S_LIGHT); 

  
  	checkVoltAmpers.setInterval(2000, checkVoltAmpersData);


  	//Enable watchdog timer
  	wdt_enable(WDTO_8S);

     #ifdef NDEBUG
       Serial.println();
       Serial.println("End setup");
     #endif

}




void loop() {

  checkVoltAmpers.run();


  gw.process();

  }



  void incomingMessage(const MyMessage &message) {

  if (message.isAck())
  {
    gotAck = true;
    return;
  }

    if ( message.sensor == REBOOT_CHILD_ID && message.getBool() == true ) {
             wdt_enable(WDTO_30MS);
              while(1) {};

     }
     


    if ( message.sensor == NIGHTMODE_CHILD_ID  ) {
         
         if (message.getBool() == true)
         {

         }
         else
         {

            
         }

     }


    if ( message.sensor == RECHECK_SENSOR_VALUES) {
         
         if (message.getBool() == true)
         {
            boolRecheckSensorValues = true;


         }

     }

        return;      
} 


void checkVoltAmpersData()
{

  cycleCounter++;

  emon.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  
  //sleep(1000);
  //emon.serialprint();           // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)

    #ifdef NDEBUG
       Serial.println();
       Serial.print("Real power:");
       Serial.println(emon.realPower);       
       Serial.print("RMS Power:");
       Serial.println(emon.apparentPower);   
       Serial.print("Vrms:");
       Serial.println(emon.Vrms);   
       Serial.print("Irms:");
       Serial.println(emon.Irms);   
       Serial.print("powerFactor:");
       Serial.println(emon.powerFactor);                               
     #endif




  if (cycleCounter == DATASEND_DELAY)
  {
    #ifdef NDEBUG
       Serial.println();
       Serial.println("Send data to openhab");
     #endif

        if ( ( emon.Vrms != lastVolts) || boolRecheckSensorValues )
        {
            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(VoltageMsg.set(emon.Vrms,2), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                lastVolts = emon.Vrms; 
                gotAck = false;
          }

        if ( ( emon.Irms != lastAmpers) || boolRecheckSensorValues )
        {
            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(AmpersMsg.set(emon.Irms,2), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                lastAmpers = emon.Irms; 
                gotAck = false;
          }

        if ( ( emon.realPower != lastWatts) || boolRecheckSensorValues )
        {
            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(WattMsg.set(emon.realPower,2), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                lastWatts = emon.realPower; 
                gotAck = false;
          }

        if ( ( emon.apparentPower != lastRMSWatts) || boolRecheckSensorValues )
        {
            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   gw.send(AmpersMsg.set(emon.apparentPower,2), true);
                    gw.wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                lastRMSWatts = emon.apparentPower; 
                gotAck = false;
          }          

    cycleCounter = 0;
  }
  
  //float realPower       = emon1.realPower;        //extract Real Power into variable
  //float apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
  //float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
  //float supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
  //float Irms            = emon1.Irms;             //extract Irms into Variable

}
