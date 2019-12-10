#include <Servo.h>

const int timeSampling = 5;  // time to be in position (sec) [SETUP]

//___PINS board SETup______________________________________
const int BUTTON_PIN = 13; //press button control in pin 13
const int verde = 9; // pin 9 green LED
const int rojo = 8; // pin 8 red LED
const int pinArm_Base = 2; //servo Base control wire in pin 2
const int pinArm_A = 3; //servo A control wire in pin 3
const int pinArm_B = 4; //servo B control wire in pin 4
const int pinArm_C = 5; //servo C control wire in pin 5
int statusButton = 0; ///  initial value:   press -> [1]-> switch run/pause
boolean pausa = false; // pause initial value, controlled by press button

//___DECLARE SERVOS TO SEGMENTS OF THE ARM______________________________________
Servo Base;
Servo arm_A;
Servo arm_B;
Servo arm_C;

// [TO BE SET!]  ARM SEGMENT LENGTH IN MM_______________________________________
const float A_lenght = 131.0; // -> length segm A
const float B_lenght = 120.0; // -> length segm B
const float C_lenght = 107.0; // -> length segm C
const float radio = 10.0;     // -> distance from rotational axis Base to rotational axis servo_A in x-y plane
// the serv_A and segments A+B+C are on the left side, this shift must be taken into account to target properly

// [TO BE SET!]  DISTANCE ARM TO RACK AND HEIGHT TO OVERTAKE____________________
const float Target_vert =140.0;  // -> height on top of target were arm stop before going down
const float vert_down =100.0;    // -> lowest point when getting down to target
const float distance_rack_90 = 60.0;// -> distance from base of arm to rack
//**aux distance calculation
float Target_horiz ;     //-> it will be calculated according to rack position
float Target_horiz_B ; // ->calculated to reach real target, taking into account distance axis servoA and rotational axis;
float cateto_horiz;

/*___RACK DIMENSIONS____________________________________________________________
                 _
 |_|_|_|_|_|_| 1  |        first 1-1  -> [0,0]
 |_|_|_|_|_|_| 2  |row     last  3-6  -> [2,5]
 |_|_|_|_|_|_| 3 _|         _
 [1-2-3-4-5-6] <--- line     |                        _
 <----> <---->               | distance_rack_90      |_| <-lenght_cell
 left    right               |
       #                    _|
       ^_arm position
 */
// [TO BE SET!]  SET ACCORDING TO RACK__________________________________________
const int n_cell_Line = 6; // n cell per line
const int n_cell_row = 3; // n cell per row
int lenght_cell =34; //lenght of the side (square) in mm

int array_line[n_cell_Line]; // create array for n cells per line
int array_row[n_cell_row]; // create array for n cells per row

// [TO BE SET!]  baseline vial
boolean baseLine = true; // true if you want baseLine sample in between, false otherwise.
const int rowBaseLinePos = 0; //set vial for baseline in row
const int lineBaseLinePos= 0;//set vial for baseline in line

//___VAR_NAMES FOR DEGREES______________________________________________________
float res_AB;
float degree_resAB;
float degree_A;
float degree_B;
float degree_C;
float degree_base;

//___AUX VARS___________________________________________________________________
float pi = 3.1415926; // enough!

//
// SETUP ARDUINO BOARD N PINS___________________________________________________
void setup()
{
  Serial.begin(9600);
  //___ARRAY FOR N CELLS IN RACK BY LINE AND ROW_______________________________
  for (int n =0; n < n_cell_Line; n++) {
    array_line[n] = n+1;
  }
  for (int n =0; n < n_cell_row; n++) {
    array_row[n] = n+1;
  }

  //SET SERVO CONTROL PINS ON BOARD
  Base.attach(pinArm_Base);//
  arm_A.attach(pinArm_A);//
  arm_B.attach(pinArm_B);//
  arm_C.attach(pinArm_C);//

  //SET BUTTON PIN AND MODE
  pinMode(BUTTON_PIN, INPUT);

  // SET LED STATUS PINS
  pinMode(verde, OUTPUT); //green status LED 'running'
  pinMode(rojo, OUTPUT); // red status LED 'pause'
  digitalWrite(verde,HIGH); // green LED ON
  digitalWrite(rojo,LOW); // red LED ON
}


// MAIN LOOP FOR RUNNING THROUGH THE WHOLE RACK - it starts again after
void loop()
{
  for (int i = 0; i < n_cell_row; i++) //  going through rows numbers (m old)
  {
    for (int j = 0; j < n_cell_Line; j++)  // going through line  numbers (kk old)
    {
      // baseline ON or OFF in between samples

      if (baseLine == true)
      {
        Target_horiz = targetHCalc(rowBaseLinePos,lineBaseLinePos); // return Target_horiz and set base degreess
        //_ARM DOWN TO TARGET___________________________________________________
        moveUpDown(Target_horiz, 0, vert_down, 1);
        //_DOWN ON TARGET, PAUSE BUTTON OPTION IS ACTIVE here___________________
        stillDown();
        //_ARM UP from TARGET___________________________________________________
        moveUpDown(Target_horiz,vert_down,0, -1);
      }
      delay(1000);
      //------sample-------
      Target_horiz = targetHCalc(i,j); // return Target_horiz and set base degrees
      //_ARM DOWN TO TARGET_____________________________________________________
      moveUpDown(Target_horiz, 0, vert_down, 1);
      //_DOWN ON TARGET, PAUSE BUTTON OPTION IS ACTIVE here_____________________
      stillDown();
      //_ARM UP from TARGET_____________________________________________________
      moveUpDown(Target_horiz,vert_down,0, -1);

    }
  }
}

/* Caculate distance to target according to vial postion i j -> m k ,
 and set degrees for servo 'Base':
 ~takes cell position as int ->  'm' (n cell in ROW) and 'k' (n cell in LINE)
 ~return float targetH1 used for further calculations with other servos
 Target_horiz = targetHCalc(m,k)
 */

float targetHCalc(int m,int k)
{
  float fc = 1.0; //used to change + or - according to left or right half of rack
  float fr = 1.0;// idem
  float falph = 1.0;//idem
  float deg90 = 1.0;//idem
  float aux_catHoriz; //aux for flexible calculation

  if (array_line[k] <= (n_cell_Line/2) ) //check if target is on left or right half of rack to calculate consecuently
  {
    aux_catHoriz = ((n_cell_Line/2) - array_line[k]);
  }
  else {
    aux_catHoriz = (array_line[k])-(n_cell_Line/2)-1;
    falph = -1.0;
  }

  cateto_horiz = aux_catHoriz * lenght_cell + (lenght_cell/2) ;

  if (array_line[k] <= (n_cell_Line/2) && cateto_horiz> radio) //left side and cateto_horiz > radio
  {
    fr = -1;
    deg90 = -1.0;
  }
  else {
    fc = -1;
  }

  float x = (fr*radio) + (cateto_horiz*fc);
  float targetH2 = sqrt((sq((distance_rack_90 + (array_row[m])*lenght_cell)) + sq(x)) );
  float targetH1 = sqrt( (sq(targetH2) - sq(radio)) );

  float alpha = (asin(( (distance_rack_90 + (array_row[m])*lenght_cell)/targetH2)))*180.0/pi;
  float beta = (asin( (targetH1/targetH2) ))*180.0/pi;

  degree_base = (falph*alpha) + beta + 90.0*deg90 ;

  delay(800);
  Base.write(degree_base); // arm base rotation to get right direction for target
  delay(1200);

  return targetH1;

}


/* Caculate degrees for each angle Base-A & A-B & B-C
 and set them to corresponding servos:
 takes:
 ~targetXY - point xy to reach, calculated on targetHCalc()
 ~beginning - z point where it starts
 ~ending  - z point where it finishes
 ~stepNway - positive or negative depending on UP/DOWN
 Target_horiz = targetHCalc(m,k)
 moveUpDown(targetXY, beginning, ending, stepNway)
 */

void moveUpDown(float targetXY, int beginning, int ending, int stepNway)
{
  int statusStp = beginning;
  while (statusStp != ending)
  {
    statusStp = statusStp + stepNway;

    res_AB = sqrt((sq(targetXY) + sq(Target_vert-statusStp)));  // OK!!!!!!
    float inCalc = asin((Target_vert-statusStp)/ res_AB );   // OK!!!!!!
    degree_resAB = (180.0 * inCalc)/pi;// OK!!!!!!

    //_____________ arm_A
    float x = acos ((sq(B_lenght) - sq(A_lenght) - sq(res_AB))/(-2.0*A_lenght* res_AB));// OK!
    degree_A = 180.0-degree_resAB-((180.0 * x)/pi) ;

    //_____________ arm_B
    float y = acos((sq(res_AB) - (sq(A_lenght))-(sq(B_lenght)))/(-2.0*A_lenght*B_lenght));
    degree_B = (180.0 * y)/pi;

    //__________  arm_ C
    degree_C = 270.0 -degree_A - (180.0-degree_B);

    arm_A.write(degree_A); delay(2);// set degrees in servo A
    arm_B.write(degree_B);// set degrees in servo B
    arm_C.write(degree_C); delay(3);// set degrees in servo Cdss

  }
}

// stillDown keeps arm down in target for the set time timeSampling, checks BUTTON_PIN to switch ON/OFF pausa


void stillDown()
{

  for (int ks = 0; ks < timeSampling; ks++)
  {
    statusButton = digitalRead(BUTTON_PIN);
    delay(1000) ;//time in position ~ delay to set each sec to msec
    Serial.print(statusButton);

    if (statusButton == 1) // if pause button has been pressed = 1
    {
      pausa = true;

      while (pausa == true)
      {
        digitalWrite(verde,LOW);
        digitalWrite(rojo,HIGH);
        statusButton = digitalRead(BUTTON_PIN);
        delay(2000);
        Serial.print(statusButton);
        Serial.print("PAUSA");

        if (statusButton == 1)
        {
          pausa = false;
          digitalWrite(verde,HIGH);
          digitalWrite(rojo,LOW);
        }
      }
    }
  }
}
