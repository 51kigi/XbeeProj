/*
Baseが送信した結果をログとして記録する
(APIモードにした場合　できることなら避けたい(面倒くさそうなので））
*/
/*
送信側（ROUTER)(ATﾓｰﾄﾞ)(右翼側)
アドレス：0013A200 407C042D
PAN ID：49　XB24-ZB,ZIGBEE ROUTER AT　22A7
ATDH　　接続先上位アドレス　0013A200
ATDL　　接続先下位アドレス　407C04F8
ATJV1　起動時にコーディネータに接続
ADO02　0ピンをアナログモードに
ADIR64　サンプルレートを100ミリ秒に（16新表記で64)

送信側（ROUTER)(ATﾓｰﾄﾞ)(左翼側)
アドレス：0013A200 40AE9BF9
PAN ID：49　XB24-ZB,ZIGBEE ROUTER AT　22A7
ATDH　　接続先上位アドレス　0013A200
ATDL　　接続先下位アドレス　407C04F8
ATJV1　起動時にコーディネータに接続
ADO02　0ピンをアナログモードに
ADIR64　サンプルレートを100ミリ秒に（16新表記で64)

送信側(ROUTER)(ATﾓｰﾄﾞ)(加速度計)
アドレス：0013A200 40AE9BF7
ATDH　　接続先上位アドレス　0013A200
ATDL　　接続先下位アドレス　407C04F8
ATJV1　起動時にコーディネータに接続
ADO02　0ピンをアナログモードに
AD102  1ピンをアナログモードに
AD202  2ピンをアナログモードに
ADIR64　サンプルレートを100ミリ秒に（16新表記で64)

受信側（COORDINATOR)(API2モード)(ベース)
アドレス：0013A200 407C04F8
PAN ID：49　XB24-ZB､ZIGBEE COORDINATOR API,21A7
ATDH　　接続先上位アドレス　0013A200
ATDL　　接続先下位アドレス　40AE9C73(ロガーを向いている)

ロガー(ROUTER)(ATﾓｰﾄﾞ)
アドレス：0013A200 40AE9C73
ATDH　　接続先上位アドレス　0013A200
ATDL　　接続先下位アドレス　407C04F8
ATJV1　起動時にコーディネータに接続
ADO02　0ピンをアナログモードに
ADIR64　サンプルレートを100ミリ秒に（16新表記で64)

*/

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
#include <SD.h>
#include <XBee.h>
const int chipSelect = 4;
int StatusLED=8;  //状態表示
String writeStr;  //SD書き込み用文字列

XBee xbee = XBee(); 
XBeeResponse response = XBeeResponse(); 
// create reusable response objects for responses we expect to handle  
ZBRxResponse rx = ZBRxResponse(); 
ModemStatusResponse msr = ModemStatusResponse(); 

void setup(){
  pinMode(StatusLED,OUTPUT);
  Serial.begin(9600);  
  xbee.begin(9600);
  //SDカードライブラリのおまじない群  
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    //show status(failed)
    for(int i=0;i<3;i++){
       digitalWrite(StatusLED,HIGH);
       delay(100);
       digitalWrite(StatusLED,LOW); 
       delay(100);
       digitalWrite(StatusLED,HIGH);
       delay(500);
       digitalWrite(StatusLED,LOW); 
       delay(500);
    } 
    return;
  }
  Serial.println("card initialized.");
       //show status(initialized)
       for(int i=0;i<4;i++){
       digitalWrite(StatusLED,HIGH);
       delay(500);
       digitalWrite(StatusLED,LOW); 
       delay(500);
       } 
}

void loop(){
  char buf[16];
  xbee.readPacket();
  
  if(xbee.getResponse().isAvailable()){
    if(xbee.getResponse().getApiId()==ZB_RX_RESPONSE){
      xbee.getResponse().getZBRxResponse(rx);
      
      if(rx.getOption()==ZB_PACKET_ACKNOWLEDGED){
        //SenderはACKOWLEDGEを受け取った
      }else{
        //SenderはACKOWLEDGEを受け取らなかった
      }
      
//      Serial.print("Checksum is :");
//      Serial.println(rx.getChecksum(),HEX);
//      
//      Serial.print("packet length is:");
//      Serial.println(rx.getPacketLength(),DEC);
      
//      for(int i=0;i<rx.getDataLength();i++){
//        Serial.print("Payload[");
//        Serial.print(i,DEC);
//        Serial.print("] is ");
//        Serial.println(char(rx.getData()[i]));
////    Serial.println(rx.getData()[i],HEX);
//      }
      
      //書き込み文字列作成
      if(char(rx.getData()[1])=='R'||char(rx.getData()[1])=='L'){
        writeStr = String(millis()) + "," + char(rx.getData()[0]) + "," + char(rx.getData()[1]) + "," ;
        writeStr = writeStr + char(rx.getData()[2]) + char(rx.getData()[3]) ;
        writeStr = writeStr + char(rx.getData()[4]) + char(rx.getData()[5])  + char(rx.getData()[6]);
      }
      
      Serial.println(writeStr);
      
      //開始フラグ受信のときはLED点滅
      if(char(rx.getData()[0])=='S'){
        for(int i=0;i<3;i++){
         digitalWrite(StatusLED,HIGH);
         delay(10);
         digitalWrite(StatusLED,LOW); 
         delay(10);
         digitalWrite(StatusLED,HIGH);
         delay(10);
         digitalWrite(StatusLED,LOW); 
         delay(10);
        }
      }
      
//      for(int i=0;i<xbee.getResponse().getFrameDataLength();i++){
//        Serial.print("frame data [");
//        Serial.print(i,DEC);
//        Serial.print("] is ");
////        Serial.println(xbee.getResponse().getFrameData()[i],HEX);
//        Serial.println(char(xbee.getResponse().getFrameData()[i]));
////        Serial.println(String(xbee.getResponse().getFrameData()[i],DEC));
//      }
      
//  確認のためSD書き込みをコメントアウト
     File dataFile = SD.open("datalog.txt", FILE_WRITE);

     // if the file is available, write to it:
     if (dataFile) {
       dataFile.println(writeStr);
       dataFile.close();
//       
       digitalWrite(StatusLED,HIGH);
       delay(5); //100から短縮して書き込み密度を上げた
       digitalWrite(StatusLED,LOW); 
     }  
     // if the file isn't open, pop up an error:
     else {
     //Serial.println("error opening datalog.txt");
      for(int i=0;i<3;i++){
       digitalWrite(StatusLED,HIGH);
       delay(5);
       digitalWrite(StatusLED,LOW); 
       delay(5);
       digitalWrite(StatusLED,HIGH);
       delay(10);
       digitalWrite(StatusLED,LOW); 
       delay(10);
      }
     }
     

    }else if(xbee.getResponse().isError()){
      Serial.print("error code:");
      Serial.println(xbee.getResponse().getErrorCode());  
    }else{
    //RXRESPONSEのパケットにだけ反応　他のパケットに反応する場合はここに処理追加
    }
  }
}
