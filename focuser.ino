#define MAXPOSITION 2100    //measured for my focuser
#define FAST 1500 
#define MEDIUM 2000 
#define SLOW 2500 

#define STEP_PIN 6
#define ENAB_PIN 7
#define DIR_PIN 8

int speed=FAST;  //in micro seconds, length of motor step 
int position=0;  //curent motor position
int time1=0,time=0,time0=0;
int pulse=0;
int enable=1;
char input[15];

void setup()
{
  //input pins that receive commands from hand controller
  pinMode(2,INPUT);     //move in
  pinMode(3,INPUT);     //move out
  pinMode(4,INPUT);     //decrease speed
  pinMode(5,INPUT);     //increase speed
  pinMode(9,INPUT);     //manual motor release
  pinMode(10,INPUT);    //no function
  //output pins that drive motor attached to a focuser
  pinMode(STEP_PIN,OUTPUT);   //step signal pin
  pinMode(ENAB_PIN,OUTPUT);   //enable and disable motor
  pinMode(DIR_PIN,OUTPUT);    //direction pin 
  //
  Serial.begin(9600);
  digitalWrite(ENAB_PIN,enable);
  //MS1 connected to vdd, MS2 and MS3 to gnd -> using half-step setting
}

//create and send step signal to step pin
//dir: direction in which the motor should turn
//speed: lenght of the motor step
void step(int dir, int speed)
{  
  digitalWrite(DIR_PIN,dir);  
  if(enable==1)    //enables motor if it is disconected
  {
    enable=0;
    digitalWrite(ENAB_PIN,enable);
    delayMicroseconds(5);    //wait a few us, stabilizes output
  }
  if(((time0-time1)>=speed))    //create step signal for motor
  {
    time1=micros();
    digitalWrite(STEP_PIN,pulse);
    pulse=1^pulse;      
    if(dir)
    {
      position++;
    }
    else
    {
      position--;
    }
  }
}

//read string from serail connection and save it into input variable
void readSerial()
{
  int data;
  int a=0;
  data=Serial.read();
  input[a]=data;
  if(data==0x3A)      //: is first char in ASCOM command
  {
    do
    {
      if(Serial.available())
      {
        a++;
        data=Serial.read();
        input[a]=data;
        input[a+1]='\0';
      }
    }while(data!=0x23);        //# is last char in ASCOM command
    interpretData();
  }
}

//execute recieved command
void interpretData()
{  
  switch(input[2])
  {
    case 'M' : moveTo(); break;
    case 'G' : printPosition(); break;
    case 'P' : setCurrentPosition(); break;
    case 'R' : break;   //reverse not yet implemented
    case 'L' : releaseMotor(); break;
    default : break;
  }
}

//move focuser to a positon that was specified in input ASCOM command
void moveTo()
{
  int dir;
  int toPosition=getInputInt();    //read desired position  
  if(toPosition<position) dir=0;
  else dir=1;
  while(toPosition!=position)
  {
    time0=micros(); 
    if(abs(toPosition-position)<150)step(dir,SLOW);    //slows down motor when near desired position
    else step(dir,FAST);
  }
  printPosition();
  releaseMotor();
}

//send current focuser position back to PC
void printPosition()
{
  Serial.print("P ");
  Serial.println(position);
}

//set current focuser posittion to a arbitary value that was specified in input ASCOM command
void setCurrentPosition()
{
  position=getInputInt();
  printPosition();
}

//parse number from input command
int getInputInt()    
{
  int b,j=0;
  char number[6];
  int leng=strlen(input);    //length of intput command
  for(b=4;b<(leng-2);b++)    //number always begins as 5th char in command 
  {
     number[j]=input[b];
     number[j+1]='\0';
     j++;
  }  
  int inputInt=atoi(number);
  if(inputInt>MAXPOSITION)inputInt=MAXPOSITION;    //desired postion can not be outside limits
  if(inputInt<0)inputInt=0;
  //Serial.println(inputInt);        //debuging
  return inputInt;
}

//disable motor because it can become very hot if it enabled for too long
void releaseMotor()
{
    enable=1;
    digitalWrite(ENAB_PIN,enable);
}

//--------------------------
//------ MAIN PROGRAM ------
//--------------------------
void loop() 
{ 
  
  if(digitalRead(2)&&(position>0))    //moves focuser inside on button press if it is not already at beginning
  {
    time0=micros();
    step(0,speed);
  }
  if(digitalRead(3)&&(position<MAXPOSITION))    //moves focuser out
  {
    time0=micros(); 
    step(1,speed);
  }
  if(digitalRead(4))
  {
    speed+=50;    //decrease speed
    delay(100);    //delay for button press is detected once
    //if(speed>3000)speed=3000;    //set min speed
  }
  if(digitalRead(5))
  {
    speed-=50;    //increase speed
    delay(100);
    //if(speed<200)speed=200;      //set max speed
  } 
  if(Serial.available())
  {
    readSerial();
  }
  
  if(digitalRead(9))
  {
    releaseMotor();
    delay(100); 
  }    
  /*with this motor is not working properly (loosing torque, changing speed), have no idea why
  if(((time1+300*speed)<micros())&&enable==0) //disconnects motor after some time, so it is not heating too much
  {
    releaseMotor();
  }
  */
}
