//Esse código foi baseado no código aberto da Mindwave Mobile (produtora do dispositivo).

#include <SPI.h>
#include <Ethernet.h>
#include <HttpClient.h>
#include <Xively.h>

#define LED 13
#define BAUDRATE 57600
#define DEBUGOUTPUT 0

#define powercontrol 10

// checksum variables
byte generatedChecksum = 0;
byte checksum = 0;
int payloadLength = 0;
byte payloadData[64] = {0};
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;

// system variables
long lastReceivedPacket = 0;
boolean bigPacket = false;

void setup()
{

  pinMode(LED, OUTPUT);
  Serial.begin(BAUDRATE);
}

byte ReadOneByte()

{
  int ByteRead;
  while (!Serial.available());
  ByteRead = Serial.read();

#if DEBUGOUTPUT
  Serial.print((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif

  return ByteRead;
}

void loop()

{
  // Look for sync bytes
  if (ReadOneByte() == 170)
  {
    if (ReadOneByte() == 170)
    {
      payloadLength = ReadOneByte();

      if (payloadLength > 169)                     //Payload length can not be greater than 169
        return;
      generatedChecksum = 0;
      for (int i = 0; i < payloadLength; i++)
      {
        payloadData[i] = ReadOneByte();            //Read payload into memory
        generatedChecksum += payloadData[i];
      }

      checksum = ReadOneByte();                      //Read checksum byte from stream
      generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum

      if (checksum == generatedChecksum)
      {
        poorQuality = 200;
        attention = 0;
        meditation = 0;

        for (int i = 0; i < payloadLength; i++)
        { // Parse the payload
          switch (payloadData[i])
          {
            case 2:
              i++;
              poorQuality = payloadData[i];
              bigPacket = true;
              break;
            case 4:
              i++;
              attention = payloadData[i];
              break;
            case 5:
              i++;
              meditation = payloadData[i];
              break;
            case 0x80:
              i = i + 3;
              break;
            case 0x83:
              i = i + 25;
              break;
            default:
              break;
          } // switch
        } // for loop

#if !DEBUGOUTPUT

        // *** Add your code here ***

        if (bigPacket)
        {
          if (poorQuality == 0)
            digitalWrite(LED, HIGH);
          else
            digitalWrite(LED, LOW);

          Serial.print("PoorQuality: ");
          Serial.print(poorQuality, DEC);
          Serial.print(" Attention: ");
          Serial.print(attention, DEC);
          Serial.print(" Time since last packet: ");
          Serial.print(millis() - lastReceivedPacket, DEC);
          lastReceivedPacket = millis();
          Serial.print("\n");

          byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
          char xivelyKey[] = "U5KTrypyfdtNNI0wYOXmprH7CBrBpvUueZyEirkaKO0F1w6X";
          int sensorPin = 2;
          char atencao[] = "atencao_valor";
          char meditacao[] = "meditacao_valor";
          XivelyDatastream datastreams[] = {
            XivelyDatastream(atencao, strlen(atencao), DATASTREAM_FLOAT),
            XivelyDatastream(meditacao, strlen(meditacao), DATASTREAM_FLOAT)
          };
          XivelyFeed feed(472311520, datastreams, 2 /* number of datastreams */);
          EthernetClient client;
          XivelyClient xivelyclient(client);

          while (Ethernet.begin(mac) != 1)
          {
            Serial.println("Error getting IP address via DHCP, trying again...");
            delay(15000);
          }

          datastreams[0].setFloat(attention);

          Serial.print("Read sensor value ");
          Serial.println(datastreams[0].getFloat());

          datastreams[1].setFloat(meditation);
          Serial.print("Setting buffer value to:\n    ");
          Serial.println(datastreams[1].getBuffer());

          Serial.println("Uploading it to Xively");
          int ret = xivelyclient.put(feed, xivelyKey);
          Serial.print("xivelyclient.put returned ");
          Serial.println(ret);

          Serial.println();
          delay(15000);



        }
#endif
        bigPacket = false;
      }
      else {
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte
}
