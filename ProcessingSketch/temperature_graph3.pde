import processing.serial.*;
PFont font;

int NUM=2;
Serial myPort;


int xv=0;
float tmp;
String wing;
String datastr;

void setup(){
  size(1000,510);
  background(40,70,50);
  
  font=loadFont("MS-PGothic-18.vlw");
  textFont(font);
  
  
  println(Serial.list());
  myPort=new Serial(this,Serial.list()[1],9600);
  myPort.bufferUntil('\n');
  myPort.clear();
  
  tmp=23.5;
  wing="R";
  stroke(255,255,255);
  line(0,height/2,width,height/2);
  text("15",0,height/2);
  for(int i=1;i<30;i++){
    line(0,height/30*i,width,height/30*i);
    text(30-i,0,height/30*i);
  }
}

void draw(){
if(myPort.available()>0){
    String myString=myPort.readString();
    myString=trim(myString);
  String[] tmpdata=split(myString,',');
  
  if(tmpdata.length==3){
 
  wing=trim(tmpdata[1]);
  tmp=float(trim(tmpdata[2]));

  println(wing);
  println(tmp);
//  wing="R";
//  println(wing);
  
    if(wing.equals("R")==true){
      stroke(0,255,0);
      fill(0,255,0);
      ellipse(xv,map(tmp,0,30,510,0),5,5);
      println("R done");
    }else if(wing.equals("L")==true){
      stroke(255,0,0);
      fill(255,0,0);
      ellipse(xv,map(tmp,0,30,510,0),5,5);
      println("L done");
    }else{
      stroke(255,0,0);
      fill(255,0,0);
      ellipse(xv,map(tmp,0,30,510,0),5,5);
      println("nothig");
    }
    xv+=4;
    }
  }

//  xv+=1;
//  tmp+=0.01;
//  println(xv);
//  println(tmp);
//  println(wing);
//  println(map(tmp,30,0,500,0));
//  println(map(tmp,30,0,500,0)-500);
//  println(xv);
  if(xv>999){
     xv=0;
     background(40,70,50);
     noStroke();
  stroke(255,255,255);
  line(0,height/2,width,height/2);
  text("15",0,height/2);
  for(int i=1;i<30;i++){
    line(0,height/30*i,width,height/30*i);
    text(30-i,0,height/30*i);
  }
     println("redraw");
  }
  if(tmp==30.0){
    tmp=23.0;
  }
}

//http://yoppa.org/tau_bmaw13/4790.html
