/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       Student                                                   */
/*    Created:      11/6/2026, 7:05:41 PM                                     */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "vex.h"

using namespace vex;

// A global instance of competition
competition Competition;

// define your global instances of motors and other devices here
controller Controller = controller(primary);
brain Brain;
motor LF = motor(PORT2, ratio6_1, false);
motor LB = motor(PORT3, ratio6_1, true);
motor RF = motor(PORT4, ratio6_1, true);
motor RB = motor(PORT12, ratio6_1, false);

motor LARM = motor(PORT6, ratio18_1, true);
motor RARM = motor(PORT10, ratio18_1, false);
motor CL = motor(PORT1, true);
inertial gyroSensor = inertial(PORT15);
digital_out Toggle = digital_out(Brain.ThreeWirePort.A);
// int lspeed = 0;
// int rspeed = 0;
bool ArmPressed = false;
double pi = 3.14;
double gearRatio = 0.6;
double wheelSize = 3.25;
double circumference = pi * gearRatio * wheelSize;
bool TogglePressed = false;


void drive(double lspeed, double rspeed, double wt){
	LF.spin(forward, lspeed, pct);
	LB.spin(forward, lspeed, pct);
	RF.spin(forward, rspeed, pct);	
	RB.spin(forward, rspeed, pct);
	wait(wt, msec);
}

void driveBrake(){
	LF.stop(brake);
	LB.stop(brake);
	RF.stop(brake);
  RB.stop(brake);
}

void openClaw(){
	CL.spinToPosition(0, degrees);
}

void closeClaw(){
	CL.spinToPosition(30, degrees);
}

// void liftArmUp(){ 
// 	ArmPressed = !ArmPressed;
// 	if (ArmPressed == true){
// 		LARM.spin(forward, 50, pct);
// 		RARM.spin(forward, 50, pct);
// 	} else if (ArmPressed == false){
// 		LARM.stop(hold);
// 		RARM.stop(hold);
// 	}
// }

void liftArmUp(float speed, int wt){ 
	LARM.spin(fwd, speed, pct);
    RARM.spin(fwd, speed, pct);
    wait(wt, msec);
    LARM.stop(hold);
    RARM.stop(hold);

}

void liftArmDown(float speed, int wt){
	LARM.spin(reverse, speed, percent);
	RARM.spin(reverse, speed, percent);
	wait(wt, msec);
    LARM.stop(brake);
    RARM.stop(brake);
}

void toggleToggle(){
	TogglePressed = !TogglePressed;
	Toggle.set(TogglePressed);
}

// void ToggleUp(){
// 	Toggle.open();
// }

void inchDriveP(double target){
	float currentPosition = 0;
	float error = target - currentPosition;
	float kp = 1.5;
	float speed = kp*error;
  	float accuracy = 1.5;
	LF.setPosition(0.0, rev);
	while(fabs(error)>accuracy){
		drive(speed,speed,10);
		currentPosition = LF.position(rev)* circumference ;
		error = target - currentPosition;
		speed = kp*error;
	}
	driveBrake();
}

void gyroTurningP(double target){
	float currentHeading = 0;
	float error = target - currentHeading;
	float kp = 0.5;
	float speed = kp*error;
  	float accuracy = 2.0;
	gyroSensor.setRotation(0.0, deg);
	
	while(fabs(error)>=accuracy){
		speed = kp * error;
		drive(speed,-speed,10);
		currentHeading = gyroSensor.rotation(deg);
		error = target - currentHeading;
	}
	driveBrake();
}




int selectedAuton = 4; // 0 = None, 1 = Red 1, 2 = Red 1.5, 2 = Red , 3 = Blue 1, 4 = Blue 2, 5 = Do Nothing

// ============================================================================
// 2. AUTON SELECTOR FUNCTIONS
// ============================================================================

// Draws the 5 auton buttons on the LEFT half of the screen
void drawAutonButtons() {
  const char* autonNames[] = {"RED 1","RED 1.5", "RED 2", "BLUE 1", "BLUE 2", "SKILLS"};
  Brain.Screen.setFont(fontType::mono20);

  for (int i = 0; i < 6; i++) {
    int y = 5 + (i * 46); // Stack buttons vertically (42px tall + 4px gap)

    // Highlight the selected button green; keep others dark gray
    if (selectedAuton == (i + 1)) {
      Brain.Screen.setPenColor(color::white);
      Brain.Screen.setFillColor(color::green);
    } else {
      Brain.Screen.setPenColor(color::white);
      Brain.Screen.setFillColor(color(60, 60, 60)); // Dark gray
    }

    // Draw button rectangle: x, y, width, height
    Brain.Screen.drawRectangle(5, y, 225, 42);

    // Print button text inside
    Brain.Screen.setPenColor(color::black);
    Brain.Screen.printAt(20, y + 28, autonNames[i]);
  }
}

// Checks if the touch screen was pressed inside any of the 5 buttons
void checkAutonTouch() {
  if (Brain.Screen.pressing()) {
    int touchX = Brain.Screen.xPosition();
    int touchY = Brain.Screen.yPosition();

    // Check if the touch happened inside the left half
    if (touchX >= 5 && touchX <= 230) {
      for (int i = 0; i < 5; i++) {
        int buttonY = 5 + (i * 46);

        // Check which button height matches the touch
        if (touchY >= buttonY && touchY <= (buttonY + 42)) {
          selectedAuton = i + 1; // 1 to 5
          drawAutonButtons();    // Redraw buttons so the selected one turns green
          wait(250, msec);       // Debounce wait to avoid double-triggers
        }
      }
    }
  }
}

// ============================================================================
// 3. MOTOR MONITOR FUNCTIONS (8 Motors, 2 Columns x 4 Rows)
// ============================================================================

// Helper function to pick color based on temperature
color getTempColor(double temperatureC) {
  if (temperatureC <= 35.0) {
    return color::green;
  } else if (temperatureC < 45.0) {
    return color::yellow;
  } else {
    return color::red;
  }
}

// Helper function to draw an individual motor box
void drawMotorBox(const char* name, motor &m, int x, int y, int width, int height) {
  Brain.Screen.setPenColor(color::black);

  if (!m.installed()) {
    // Motor disconnected: White box with clear warning
    Brain.Screen.setFillColor(color::white);
    Brain.Screen.drawRectangle(x, y, width, height);

    Brain.Screen.setFont(fontType::mono15);
    Brain.Screen.printAt(x + 6, y + 20, name);
    Brain.Screen.setFont(fontType::mono12);
    Brain.Screen.printAt(x + 6, y + 40, "UNPLUGGED");
  } else {
    // Read live data from the smart motor
    double tempC = m.temperature(temperatureUnits::celsius);
    double amps  = m.current(currentUnits::amp);

    // Color code the background by temperature
    Brain.Screen.setFillColor(getTempColor(tempC));
    Brain.Screen.drawRectangle(x, y, width, height);

    // Line 1: Motor Name
    Brain.Screen.setFont(fontType::mono15);
    Brain.Screen.printAt(x + 6, y + 20, name);

    // Line 2: Temperature & Current (e.g., "31C 1.8A")
    // Using mono12 ensures both fit neatly across the 115px box width
    Brain.Screen.setFont(fontType::mono12);
    Brain.Screen.printAt(x + 6, y + 42, "%.0fC | %.1fA", tempC, amps);
  }
}
// Updates all 8 motor boxes on the RIGHT half of the screen (X: 240 - 480)
void updateMotorMonitor() {
  int col1X = 240;
  int col2X = 360;
  int boxW  = 115;
  int boxH  = 52;

  // Row 1 (y = 5)
  drawMotorBox("Left 1",  LF,  col1X, 5,   boxW, boxH);
  drawMotorBox("Right 1", RF, col2X, 5,   boxW, boxH);

  // Row 2 (y = 63)
  drawMotorBox("Left 2",  LB,  col1X, 63,  boxW, boxH);
  drawMotorBox("Right 2", RB, col2X, 63,  boxW, boxH);

  // Row 3 (y = 121)
  drawMotorBox("Left Arm", LARM,  col1X, 121, boxW, boxH);
  drawMotorBox("Right 3", RARM, col2X, 121, boxW, boxH);

//   // Row 4 (y = 179)
   drawMotorBox("Claw",  CL, col1X, 179, boxW, boxH);
//   drawMotorBox("Arm",     Arm,    col2X, 179, boxW, boxH);
}




/*void driveBrake(){
  LF.stop(brake);
  LM.stop(brake);
  LB.stop(brake);
  RF.stop(brake);
  RM.stop(brake);
  RB.stop(brake);
}*/

/*---------------------------------------------------------------------------*/
/*                          Pre-Autonomous Functions                         */
/*                                                                           */
/*  You may want to perform some actions before the competition starts.      */
/*  Do them in the following function.  You must return from this function   */
/*  or the autonomous and usercontrol tasks will not be started.  This       */
/*  function is only called once after the V5 has been powered on and        */
/*  not every time that the robot is disabled.                               */
/*---------------------------------------------------------------------------*/

void pre_auton(void) {
  printf("in pre-auton\n");
    CL.setPosition(-30,degrees);
    drawAutonButtons();
    updateMotorMonitor();
  // Keep checking for touches until the match starts
  while (!Competition.isEnabled()) {
    checkAutonTouch();
    updateMotorMonitor();
    wait(50, msec);
  }

  
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              Autonomous Task                              */
/*                                                                           */
/*  This task is used to control your robot during the autonomous phase of   */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

void autonomous(void) {
  switch (selectedAuton) {
    case 1:
    //right side RED Auton
    //toggle, score pin & go to loader
	
      //toggle
      liftArmUp(50, 800); //toggling 
      inchDriveP(5);
      wait(200, msec);
      liftArmDown(50, 800); //reseting
      wait(200, msec);
      inchDriveP(-5.75); 
      liftArmUp(50, 800); //toggling
      LF.spin(forward, -5, pct);
      LB.spin(forward, -5, pct);
      RF.spin(forward, -5, pct);	
      RB.spin(forward, -5, pct);
      wait(300,msec);
      driveBrake(); 
      inchDriveP(5); // moving out of the way of the toggle
      liftArmDown(20, 750); 
      inchDriveP(23.5); //moving into position to turn
      wait(20,msec);
      gyroTurningP(89); //turning
      wait (300,msec);
      inchDriveP(56); //heading toward the loader
      wait(20,msec);
      gyroTurningP(83); //turning toward the loader
      wait(20,msec);
      inchDriveP(25.5); //driving into the loader
      wait(20,msec);
      closeClaw(); //closing the claw on the cup
      wait(20,msec);
      inchDriveP(-20); //backing up
      liftArmUp(50, 500); //preparing arm
      gyroTurningP(170); //turning around

      break;
      
      case 2:
    //right side RED Auton
    //toggle and go to loader
    CL.setPosition(0,degrees);
      CL.spinToPosition(-30,degrees);
      inchDriveP(1);
      liftArmUp(60, 900); 
      inchDriveP(5);
      wait(200, msec);
      liftArmDown(40, 660); 
      wait(200, msec);
      inchDriveP(-5.7);
      liftArmUp(25, 400 );
      LF.spin(forward, -5, pct);
      LB.spin(forward, -5, pct);
      RF.spin(forward, -5, pct);	
      RB.spin(forward, -5, pct);
      wait(300,msec);
      driveBrake(); 
      inchDriveP(19.7);
      liftArmDown(40, 350);
      gyroTurningP(86);
      inchDriveP(15.2);
      liftArmDown(30,350);
      wait(100,msec);
      CL.spinToPosition(30,degrees);
      wait(100,msec);
      // inchDriveP(-16.1);
      //  gyroTurningP(-90);
      // inchDriveP(8); 
      //  wait(20,msec);
      // gyroTurningP(89);
      //  wait (300,msec);
      //  inchDriveP(56);
      //    wait(20,msec);
      //  gyroTurningP(83);
      //  wait(20,msec);
      //  inchDriveP(25);
      //  wait(20,msec);
      //  closeClaw();
      //  wait(20,msec);
      //  inchDriveP(-20);
      // liftArmUp(50, 500);
      //  gyroTurningP(180);
      //  inchDriveP(1);
      //  liftArmDown(50, 470);
      //  openClaw();
      

      break;

    case 3:
      CL.setBrake(hold); 
      //LEFT SIDE OF RED ALLIANCE - 15secs

      //lift goes up
     /* inchDriveP(10);
      wait(500, msec);
      //lift goes down
      inchDriveP(-10);
      //lift goes up again
      wait(500, msec);
      inchDriveP(14);
      wait(200, msec);
      gyroTurningP(90);
      wait(200, msec);
      inchDriveP(13);
      wait(500, msec);
      //lift goes down
      //claw opens
      gyroTurningP(160); // gyroturning is slightly off */
      
      //close the claw 
      CL.spin(forward, 100, pct ); 
      wait(250,msec);
      CL.stop(hold); //q
      //drive forward
    inchDriveP(16);
      //turn right
    gyroTurningP(89);
    

      //lift up
      liftArmUp(40,400);

      //drie forward to the goal 
      inchDriveP(16);

      //lift down and open claw 
      liftArmDown(40,2000);
      wait(2,msec);
      CL.spin(reverse, 70, pct);
      wait(250,msec);
      CL.stop(brake);


      //back up 

      //turn left 

      //drive foward 

      //turn right toward the loader
       LARM.spin(forward, 50, pct);
       RARM.spin(forward, 50, pct);
       wait(250,msec);
       LARM.stop(brake); 
       RARM.stop(brake);
       inchDriveP(5);
       LARM.spin(forward, -50, pct);
       RARM.spin(forward, -50, pct);
       wait(250,msec);
       LARM.stop(brake); 
       RARM.stop(brake);
       drive(-100, -100, 400);
       wait(250,msec);
       LARM.spin(forward, 100, pct);
       RARM.spin(forward, 100, pct);
       wait(250,msec);
       LARM.stop(brake); 
       RARM.stop(brake);
       inchDriveP(4);
      break;


    case 4:
      // Skills Version #2
      CL.spin(reverse, 100, pct );
      liftArmUp(60, 600);
      liftArmDown(80,600);
      wait(300, msec);
      liftArmUp(60, 800);
      wait(200,msec);
      inchDriveP(13);
      wait(300,msec);
      liftArmDown(100,900);
      CL.spin(forward, 100, pct );
      // inchDriveP(3);
    


      break;

    case 5:
      //skills version #1
      CL.setPosition(0,degrees);
      CL.spinToPosition(-150,degrees,false);
      inchDriveP(1);
      liftArmUp(60, 900); 
      inchDriveP(17);
      wait(200, msec);
      // liftArmDown(40, 600); 
      // wait(200, msec);
      // inchDriveP(-2.75);
      // liftArmUp(60,800);
      // inchDriveP(-3);
      LF.spin(forward, -5, pct);
      LB.spin(forward, -5, pct);
      RF.spin(forward, -5, pct);	
      RB.spin(forward, -5, pct);
      // wait(300,msec);
      // driveBrake(); 
      // inchDriveP(18);
      // liftArmDown(40, 500);
      gyroTurningP(-86);
      inchDriveP(14.5);
      liftArmDown(40,800);
      wait(100,msec);
      CL.spinToPosition(30,degrees);
      wait(100,msec);     
       break;

    case 6:
    default:
      // Do nothing / Safe mode
      break;
  }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              User Control Task                            */
/*                                                                           */
/*  This task is used to control your robot during the user control phase of */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

void usercontrol(void){
  // User control code here, inside the loop
  
//     Brain.Screen.clearScreen();
//   Brain.Screen.setCursor(1, 1);
//   Brain.Screen.print("DRIVER CONTROL RUNNING");

  while (true) {
	
	updateMotorMonitor();

    if(Controller.ButtonL1.pressing()){
		CL.spin(reverse, 100, pct); 
	}
	else if (Controller.ButtonL2.pressing() ){
		CL.spin(fwd, 100, pct); 
	}
	else { 
		CL.stop(brake); 
	}

	if(Controller.ButtonR1.pressing()){
		LARM.spin(fwd, 100, pct); 
		RARM.spin(fwd, 100, pct); 
	}
	else if (Controller.ButtonR2.pressing() ){
		RARM.spin(reverse, 50, pct); 
		LARM.spin(reverse, 50, pct); 
	} else { 
		LARM.stop(brake); 
		RARM.stop(brake); 
	}

	Controller.ButtonA.pressed(toggleToggle);
	// if(Controller.ButtonDown.pressing() ){
	// 	Toggle.close();
	// }
	// else if(Controller.ButtonLeft.pressing() ){
	// 	Toggle.open();
	// }
	
	
    int Lspeed = Controller.Axis3.position();
    int Rspeed = Controller.Axis2.position();

    drive(Lspeed, Rspeed, 10);

	// Controller.ButtonR1.pressed(liftArmUp);
	// Controller.ButtonR2.pressed(liftArmDown);

	// if (Controller.ButtonR1.pressing()){
 	// 	openClaw();
	// }else if(Controller.ButtonR2.pressing()){
	// 	closeClaw();
	// }


    wait(20, msec);
  }
}

//
// Main will set up the competition functions and callbacks.
//
int main() {
//    Brain.Screen.clearScreen();
//   Brain.Screen.setCursor(1, 1);
//   Brain.Screen.print("PROGRAM STARTED");
	
	
	// Register competition functions
  Competition.autonomous(autonomous);
  Competition.drivercontrol(usercontrol);

  //usercontrol(); 
  pre_auton();

  // Prevent main from exiting with an infinite loop.
  while (true) {
    wait(100, msec);
  }
}