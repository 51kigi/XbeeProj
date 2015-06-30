

/*
センサーの温度を読みLEDを光らせる
ペアとして設定済みのZBモジュールが前提（複数に拡張）
XBEEライブラリを使用してロガーにデータ送信
lilypad確認用にピンを変更
送信データの初期化を一連追加
不要コードを削除
ロジックを追加
Xbeeとの通信はソフトウェアシリアルに変更
値の採用を左右バランスするように変更　20150223
ちょっとしたりファクタリングも
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
アドレス：0013A200 40AE9C3A  一代目紛失により変更
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

#include <XBee.h>
#include <SoftwareSerial.h>

XBee xbee = XBee();
uint8_t payload[] = { 'I', 'K','T','T','T','T','T','X','Y','Z' };

// SH + SL Address of receiving XBee
//送信先の定義
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x40ae9c73);
//送信フレームの定義
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
//受領確認用の定義
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
//受信用の定義
ZBRxIoSampleResponse ioSample = ZBRxIoSampleResponse();

//ソフトウェアシリアルの定義
SoftwareSerial xbeeSerial(2,3);


int RightWingLED=5;  //右翼優勢(緑)
int LeftWingLED=6;  //左翼優勢(赤)
int AcceloRecieveLED=7;  //加速度計受信確認(黄)
int UnJudgeLED=8;  //判断なし(白)
int blueLED=9;//(青)
int BalanceCheckLED=10;//(桃) 左右のバランスが悪い時に転倒
int SlideSwitch=11;//(スライド）デバッグモードの切り替え
int PushSwitch=12;//(プッシュ) 計測開始フラグ送信

int arrayNum=10; //温度格納配列数
int LtempArrayCnt=0; //左翼温度格納用配列カウンタ
int RtempArrayCnt=0; //右翼温度格納用配列カウンタ
float LtempArray[10]; //左翼温度格納配列
float RtempArray[10]; //右翼温度格納配列

int arrayNumAvg=10; //平均温度格納用配列
float LAvg;
float RAvg;
int LtempAvgCnt=0;//左翼平均格納配列カウンタ
int RtempAvgCnt=0;//右翼平均格納配列カウンタ
float LtempAvg[10]; //左翼平均配列
float RtempAvg[10]; //右翼平均配列
float LtempAvgReorder[10]; //平均配列を並べなおした配列（差を取得）
float RtempAvgReorder[10]; //平均配列を並べなおした配列（差を取得）　　

float RChangeRate;
float LChangeRate;
float ChangeRateJudge;

int balanceJudge=15;//データのばらつきを許容しなくなる回数
int recvWinCntNum=20;
int recvWingCntCnt=0;//受信データバラつき確認配列用カウンタ
int recvWingCnt[20];//受信データのばらつき確認用配列
int recvWingCntJudge;//受信データバラつき判定用変数

//String lastRevWing; //前回受信した信号

float JudgeThread=0.01;//判定の閾値

//値の初期化
int tmpAnalogValue=0;
int analogRightValue=0;
int analogLeftValue=0;
int analogXaxisValue=0;
int analogYaxisValue=0;
int analogZaxisValue=0;
char lastRevWing='Z';

//送信用の値格納
//バイト単位で指定文字を指定する必要があるため、浮動小数点を
//とりあえずString型に格納し、要素数を指定して載せる
char SendID='Z';  //送信ID
char KIND='Z';  //データ種別(R:右翼,L:左翼,A:加速度)
char SendTmp[5];
byte SendTmp0=0;  //温度
byte SendTmp1=1;  //温度
byte SendTmp2=2;  //温度
byte SendTmp3=3;  //温度
byte SendTmp4=4;  //温度
byte SendXaxis=5;  //X軸
byte SendYaxis=6;  //Y軸
byte SendZaxis=7;  //Z軸

float tmp;
String SensAddress;  //アドレス一時格納変数
String RcvStr;  //受信データ種類一時格納変数
String frmStr;

void setup(){
  pinMode(RightWingLED,OUTPUT);
  pinMode(LeftWingLED,OUTPUT);
  pinMode(UnJudgeLED,OUTPUT);
  pinMode(AcceloRecieveLED,OUTPUT);
  pinMode(blueLED,OUTPUT);
  pinMode(BalanceCheckLED,OUTPUT);
    
  //PULLDOWN抵抗を入れていないので内部的にPULLUP　（OFFがHIGH)
  pinMode(SlideSwitch,INPUT_PULLUP);
  pinMode(PushSwitch,INPUT_PULLUP);
  
  Serial.begin(9600);
  xbeeSerial.begin(9600);  
  xbee.setSerial(xbeeSerial); 
  
}

void loop(){
  

  
  //attempt to read a packet    
  xbee.readPacket();

  if (xbee.getResponse().isAvailable()){
    // got something

    if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE) {
      //iosampleであれば読み込み
      xbee.getResponse().getZBRxIoSampleResponse(ioSample);

//    if(SlideSwitch==HIGH){
//      //アドレスの下位を取得する
//      Serial.print(ioSample.getRemoteAddress64().getMsb(), HEX);
//      Serial.println("");  
//    }
      //アドレスの下位を格納
      SensAddress=String(ioSample.getRemoteAddress64().getLsb());
      
      //LR判定
      if(SensAddress=="1085185082"){
        RcvStr="LEFT";
        KIND='L';
      }else if(SensAddress=="1081869357"){
        RcvStr="RIGHT";
        KIND='R';
      }else {
        RcvStr="ZZ";       
      }

      if (ioSample.containsAnalog()) {
       //受信したアナログデータを出力
        tmpAnalogValue=ioSample.getAnalog(0);

       //アナログデータを温度に変換
        tmp=((tmpAnalogValue/1023.0*1200.0) - 500.0) / 10.0 ;
      
        //左右のバランスが取れない形で受信していた場合はループを抜けて次の処理へ
        //左右のバランスが指定回数以上離れるとそれ以上カウントアップせずに計測もせずに抜ける
        if(KIND=='L' && lastRevWing=='L'){
            //処理を抜ける
            //エラーLED点灯
            if(recvWingCntJudge>=5){
              digitalWrite(BalanceCheckLED,HIGH);
              return;
            }
            recvWingCntJudge+=1;
            return;
            
        }else if(KIND=='L' && lastRevWing=='R'){
            //エラーLED消燈
            recvWingCntJudge+=1;
            digitalWrite(BalanceCheckLED,LOW);
            recvWingCnt[recvWingCntCnt]=1;
            LtempArray[LtempArrayCnt]=tmp;
            LtempArrayCnt+=1;
            lastRevWing='L';
            
            if(LtempArrayCnt>=arrayNum){
            LtempArrayCnt=0;
            }
        }else if(KIND=='R' && lastRevWing=='R'){
            //エラーLED点灯
            //処理を抜ける
            if(recvWingCntJudge<=-5){
              digitalWrite(BalanceCheckLED,HIGH);
              return;
            }
            recvWingCntJudge-=1;
            return;
            
        }else if(KIND=='R' && lastRevWing=='L'){
            //エラーLED消燈
            recvWingCntJudge-=1;
            digitalWrite(BalanceCheckLED,LOW);
            recvWingCnt[recvWingCntCnt]=-1;
            RtempArray[RtempArrayCnt]=tmp;
            RtempArrayCnt+=1;
            lastRevWing='R';
            
            if(RtempArrayCnt>=arrayNum){
              RtempArrayCnt=0;
            }
        }else{
          lastRevWing=KIND;
          return;
        }
        
        recvWingCntCnt+=1;//カウントアップ
        if(recvWingCntCnt>=recvWinCntNum){
         recvWingCntCnt=0;//カウンタが配列数以上になったら初期化
        }
              
       //平均算出
        LAvg=0;  //初期化
        RAvg=0;  //初期化
        for(int i=0;i<arrayNum;i++){
         LAvg+=LtempArray[i];
         RAvg+=RtempArray[i];
        }
       
        LAvg/=arrayNum;
        RAvg/=arrayNum;
       
       //平均格納
        LtempAvg[LtempAvgCnt]=LAvg;
        LtempAvgCnt+=1;
        if(LtempAvgCnt>=arrayNumAvg){
         LtempAvgCnt=0;
        }
       
        RtempAvg[RtempAvgCnt]=RAvg;
        RtempAvgCnt+=1;       
        if(RtempAvgCnt>=arrayNumAvg){
         RtempAvgCnt=0;
        }

      //平均を並べ直す(最後に格納した値が配列の一番最後に来るようにして差分をとりやすくする)
        for(int i=0;i<arrayNumAvg;i++){
         if((LtempAvgCnt+i)>(arrayNumAvg-1)){
           LtempAvgReorder[i]=LtempAvg[i+LtempAvgCnt-arrayNumAvg];
         }else{
          LtempAvgReorder[i]=LtempAvg[i+LtempAvgCnt];
         }
        }
        for(int i=0;i<arrayNumAvg;i++){
          if((RtempAvgCnt+i)>(arrayNumAvg-1)){
            RtempAvgReorder[i]=RtempAvg[i+RtempAvgCnt-arrayNumAvg];
          }else{
            RtempAvgReorder[i]=RtempAvg[i+RtempAvgCnt];
          }
        }
      
        LChangeRate=0;  //初期化
        RChangeRate=0;  //初期化
        //変化の平均を求める
        for(int i=0;i<(arrayNumAvg-1);i++){
          LChangeRate+=LtempAvgReorder[i+1]-LtempAvgReorder[i];
          RChangeRate+=RtempAvgReorder[i+1]-RtempAvgReorder[i];
        }
                   
        LChangeRate/=(arrayNumAvg-1);
        RChangeRate/=(arrayNumAvg-1);
        ChangeRateJudge=LChangeRate+(-1)*RChangeRate; //足し算して負の数は右優勢
        
        //優勢判断
        digitalWrite(RightWingLED,LOW);
        digitalWrite(LeftWingLED,LOW);
        digitalWrite(UnJudgeLED,LOW);
  
        digitalWrite(blueLED,HIGH);
        if(ChangeRateJudge<=JudgeThread && ChangeRateJudge>=(-1*JudgeThread)){
          digitalWrite(RightWingLED,LOW);
          digitalWrite(LeftWingLED,LOW);
          digitalWrite(UnJudgeLED,HIGH);
        }else if(ChangeRateJudge>JudgeThread){
          digitalWrite(RightWingLED,LOW);
          digitalWrite(LeftWingLED,HIGH);
          digitalWrite(UnJudgeLED,LOW);
        }else if(ChangeRateJudge<(-1*JudgeThread)){
          digitalWrite(RightWingLED,HIGH);
          digitalWrite(LeftWingLED,LOW);
          digitalWrite(UnJudgeLED,LOW);         
        }else{
          digitalWrite(RightWingLED,LOW);
          digitalWrite(LeftWingLED,LOW);
          digitalWrite(UnJudgeLED,LOW);
        }
        delay(5);
        digitalWrite(blueLED,LOW);
         
        //送信データ作成
        dtostrf(tmp,5,2,SendTmp);
        SendTmp0=SendTmp[0];
        SendTmp1=SendTmp[1];
        SendTmp2=SendTmp[2];
        SendTmp3=SendTmp[3];
        SendTmp4=SendTmp[4];
        SendXaxis='0';
        SendYaxis='0';
        SendZaxis='0';
        
        //計測開始フラグ
        if(digitalRead(PushSwitch)==LOW){
          SendID='S';
          digitalWrite(AcceloRecieveLED,HIGH);
        }else{
          SendID='Z';
          digitalWrite(AcceloRecieveLED,LOW);
        }
        
        payload[0] =SendID;
        payload[1] =KIND;
        payload[2] =SendTmp[0];
        payload[3] =SendTmp[1];
        payload[4] =SendTmp[2];
        payload[5] =SendTmp[3];
        payload[6] =SendTmp[4];
        payload[7] =SendXaxis;
        payload[8] =SendYaxis;
        payload[9] =SendZaxis;
        
        xbee.send(zbTx);
        
        //Debug出力
        if(digitalRead(SlideSwitch)==HIGH){
          //  データのタイプを出力
          Serial.print("type;");
          Serial.println(RcvStr);
          Serial.print("tmpAnalogValue;");
          Serial.println(tmpAnalogValue);
          //計算後の温度出力
          Serial.print("tmp;");
          Serial.println(tmp);
          //左右バランス判定を出力
          Serial.print("BalanceCheck:");
          Serial.println(recvWingCntJudge);
          //優勢判断に使用した計算結果を出力
          Serial.print("LChangeRate:");
          Serial.println(LChangeRate);
          Serial.print("RChangeRate:");
          Serial.println(RChangeRate);
          //判断結果を出力
          Serial.print("ChangeRateJudge:");
          Serial.println(ChangeRateJudge);      
          Serial.print("Flag:");
          Serial.println(SendID);
          //for processing
          Serial.print("ForProcessing");
          Serial.print(",");
          Serial.print(SendID);
          Serial.print(",");
          Serial.print(KIND);
          Serial.print(",");
          Serial.println(tmp);
          
          Serial.println("--end-------------------");
        }    
      }
    }    
  } else if (xbee.getResponse().isError()) {
    //Debug出力
    if(SlideSwitch==HIGH){
      Serial.print("Error reading packet.  Error code: ");  
      Serial.println(xbee.getResponse().getErrorCode());
      //libraryのDevelopersガイドより
      //1 - CHECKSUM_FAILURE 
      //2 - PACKET_EXCEEDS_BYTE_ARRAY_LENGTH 
      //3 - UNEXPECTED_START_BYTE 
    }    
  }
}


