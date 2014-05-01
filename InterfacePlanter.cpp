/*
  InterfacePlanter - a libary for the MeijWorks interface
 Copyright (C) 2011-2014 J.A. Woltjer.
 All rights reserved.
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <InterfacePlanter.h>

// -----------
// Constructor
// -----------
#ifndef UART
InterfacePlanter::InterfacePlanter(LiquidCrystal_I2C * _lcd,
                     ImplementPlanter * _implement,
                     VehicleTractor * _tractor,
                     VehicleGps * _gps){
  // Pin assignments and configuration
  // Schmitt triggered inputs
  pinMode(LEFT_BUTTON, INPUT);
  digitalWrite(LEFT_BUTTON, LOW);
  pinMode(RIGHT_BUTTON, INPUT);
  digitalWrite(RIGHT_BUTTON, LOW);
  pinMode(MODE_PIN, INPUT);
  digitalWrite(MODE_PIN, LOW);
  
  // Mode
  mode = 2; // MANUAL
  
  // Button flag
  buttons = 0;
  button1_flag = false;
  button2_flag = false;

  // Connected classes
  lcd = _lcd;
  implement = _implement;
  tractor = _tractor;
  gps = _gps;
}

#else
InterfacePlanter::InterfacePlanter(LiquidCrystal_UART * _lcd,
                     ImplementPlanter * _implement,
                     VehicleTractor * _tractor,
                     VehicleGps * _gps){
  // Pin assignments and configuration
  // Schmitt triggered inputs
  pinMode(LEFT_BUTTON, INPUT);
  digitalWrite(LEFT_BUTTON, LOW);
  pinMode(RIGHT_BUTTON, INPUT);
  digitalWrite(RIGHT_BUTTON, LOW);
  pinMode(MODE_PIN, INPUT);
  digitalWrite(MODE_PIN, LOW);
  
  // Mode
  mode = 2; // MANUAL
  
  // Button flag
  buttons = 0;
  button1_flag = false;
  button2_flag = false;

  // Connected classes
  lcd = _lcd;
  implement = _implement;
  tractor = _tractor;
  gps = _gps;
}
#endif

// ------------------------
// Method for updating mode
// ------------------------
void InterfacePlanter::update(){
  // Check buttons
  checkButtons(255, 0);

#ifdef GPS
  gps->update();
#endif
  tractor->update();

  // Check for mode change
  
  //----------
  // Calibrate
  //----------
  if(buttons == 2){
    mode = 3;
    
    // Stop any adjusting
    implement->adjust(0);
    
    // Write complete screen
    lcd->write_screen(-1);
    
    // Calibrate
    calibrate();
    
    // After calibration rewrite total screen
    updateScreen(1);
  }
  //-------
  // Manual
  //-------
  else if(!digitalRead(MODE_PIN) ||
          tractor->getHitch() ||
          implement->getPlantingelement()){
    // set mode to manual
    mode = 2;
  }
  else {
  //-----
  // Hold
  //-----
#ifdef GPS
    if (millis() - gps->getGgaFixAge() > 2000 ||
        millis() - gps->getVtgFixAge() > 2000 ||
        millis() - gps->getXteFixAge() > 2000 ||
        gps->getQuality() != 4 ||
        !gps->minSpeed()){
      
      // set mode to hold
      mode = 1;
    }
//---------- 
// Automatic
//----------
    else {
      // set mode to automatic
      mode = 0;
    }
#else
    mode = 0;
#endif
  }
  
  // Update implement and adjust
  implement->update(mode);
  implement->adjust(buttons);
  
  // Update screen (no rewrite) and write one character
  updateScreen(0); 
  lcd->write_screen(1);
}

// --------------------------
// Method for updating screen
// --------------------------
void InterfacePlanter::updateScreen(boolean _rewrite){  
  int temp = 0;
  int temp2 = 0;

  // Update screen
  if (_rewrite){
    // Regel 0
    lcd->write_buffer(L_OFFSET, 0);

    // Regel 1
    lcd->write_buffer(L_A_POS, 1);

    // Regel 2
    lcd->write_buffer(L_SETPOINT, 2);

    // Regel 3
    lcd->write_buffer(L_XTE, 3);
    
    lcd->write_screen(-1);
  }

  // Regel 0
  temp2 = implement->getOffset();
  temp = abs(temp2);

  if (temp > 99){
    if (temp2 < 0){
      lcd->write_buffer('-', 0, 16);
    }
    else {
      lcd->write_buffer(' ', 0, 16);
    }
    lcd->write_buffer(temp / 100 + '0', 0, 17);
    temp = temp % 100;
    lcd->write_buffer(temp / 10 + '0', 0, 18);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 0, 19);
  }
  else if (temp > 9){
    lcd->write_buffer(' ', 0, 16);
    if (temp2 < 0){
      lcd->write_buffer('-', 0, 17);
    }
    else {
      lcd->write_buffer(' ', 0, 17);
    }
    lcd->write_buffer(temp / 10 + '0', 0, 18);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 0, 19);
  }
  else {
    lcd->write_buffer(' ', 0, 16);
    lcd->write_buffer(' ', 0, 17);
    if (temp2 < 0){
      lcd->write_buffer('-', 0, 18);
    }
    else {
      lcd->write_buffer(' ', 0, 18);
    }
    lcd->write_buffer(temp + '0', 0, 19);
  }

  // Regel 1
  temp2 = implement->getPosition();
  temp = abs(temp2);

  if (temp > 99){
    if (temp2 < 0){
      lcd->write_buffer('-', 1, 16);
    }
    else {
      lcd->write_buffer(' ', 1, 16);
    }
    lcd->write_buffer(temp / 100 + '0', 1, 17);
    temp = temp % 100;
    lcd->write_buffer(temp / 10 + '0', 1, 18);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 1, 19);
  }
  else if (temp > 9){
    lcd->write_buffer(' ', 1, 16);
    if (temp2 < 0){
      lcd->write_buffer('-', 1, 17);
    }
    else {
      lcd->write_buffer(' ', 1, 17);
    }
    lcd->write_buffer(temp / 10 + '0', 1, 18);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 1, 19);
  }
  else {
    lcd->write_buffer(' ', 1, 16);
    lcd->write_buffer(' ', 1, 17);
    if (temp2 < 0){
      lcd->write_buffer('-', 1, 18);
    }
    else {
      lcd->write_buffer(' ', 1, 18);
    }
    lcd->write_buffer(temp + '0', 1, 19);
  }


  // Regel 2
  temp2 = implement->getSetpoint();
  temp = abs(temp2);

  if (temp > 99){
    if (temp2 < 0){
      lcd->write_buffer('-', 2, 16);
    }
    else {
      lcd->write_buffer(' ', 2, 16);
    }
    lcd->write_buffer(temp / 100 + '0', 2, 17);
    temp = temp % 100;
    lcd->write_buffer(temp / 10 + '0', 2, 18);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 2, 19);
  }
  else if (temp > 9){
    lcd->write_buffer(' ', 2, 16);
    if (temp2 < 0){
      lcd->write_buffer('-', 2, 17);
    }
    else {
      lcd->write_buffer(' ', 2, 17);
    }
    lcd->write_buffer(temp / 10 + '0', 2, 18);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 2, 19);
  }
  else {
    lcd->write_buffer(' ', 2, 16);
    lcd->write_buffer(' ', 2, 17);
    if (temp2 < 0){
      lcd->write_buffer('-', 2, 18);
    }
    else {
      lcd->write_buffer(' ', 2, 18);
    }
    lcd->write_buffer(temp + '0', 2, 19);
  }

  // Regel  3
  temp2 = implement->getXte();
  temp = abs(temp2);

  if (temp > 99){
    if (temp2 < 0){
      lcd->write_buffer('-', 3, 5);
    }
    else {
      lcd->write_buffer(' ', 3, 5);
    }
    lcd->write_buffer(temp / 100 + '0', 3, 6);
    temp = temp % 100;
    lcd->write_buffer(temp / 10 + '0', 3, 7);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 3, 8);
  }
  else if (temp > 9){
    lcd->write_buffer(' ', 3, 5);
    if (temp2 < 0){
      lcd->write_buffer('-', 3, 6);
    }
    else {
      lcd->write_buffer(' ', 3, 6);
    }
    lcd->write_buffer(temp / 10 + '0', 3, 7);
    temp = temp % 10;
    lcd->write_buffer(temp + '0', 3, 8);
  }
  else {
    lcd->write_buffer(' ', 3, 5);
    lcd->write_buffer(' ', 3, 6);
    if (temp2 < 0){
      lcd->write_buffer('-', 3, 7);
    }
    else {
      lcd->write_buffer(' ', 3, 7);
    }
    lcd->write_buffer(temp + '0', 3, 8);
  }
    
  switch (mode){
  case 0: // AUTO
    lcd->write_buffer('A', 3, 14);
    lcd->write_buffer(' ', 3, 17);
    lcd->write_buffer(' ', 3, 18);
    break;
  case 1: // HOLD
    lcd->write_buffer('H', 3, 14);
    if (!gps->minSpeed()){
      lcd->write_buffer('S', 3, 17);
      lcd->write_buffer('!', 3, 18);
    }
    else if (gps->getQuality() != 4){
      lcd->write_buffer('Q', 3, 17);
      lcd->write_buffer('!', 3, 18);
    }
    else {
      lcd->write_buffer('G', 3, 17);
      lcd->write_buffer('!', 3, 18);
    }
    break;
  case 2: // MANUAL
    lcd->write_buffer('M', 3, 14);
    lcd->write_buffer(' ', 3, 17);
    lcd->write_buffer(' ', 3, 18);

    if (buttons == -1){
      lcd->write_buffer('<', 3, 17);
      lcd->write_buffer(' ', 3, 18);
    }
    else if (buttons == 1){
      lcd->write_buffer(' ', 3, 17);
      lcd->write_buffer('>', 3, 18);
    }
    else{
      lcd->write_buffer(' ', 3, 17);
      lcd->write_buffer(' ', 3, 18);
    }
    break;
  }
}

// ---------------------------
// Method for checking buttons
// ---------------------------
int InterfacePlanter::checkButtons(byte _delay1, byte _delay2){
  if (button1_flag){
    button1_timer = millis();
    button1_flag = false;
  }
  
  if (button2_flag){
    button2_timer = millis();
    button2_flag = false;
  }
  
  // Check for left/right button presses
  if(digitalRead(LEFT_BUTTON) && digitalRead(RIGHT_BUTTON)){
    if(millis() - button1_timer >= _delay1 * 4){ // delay * 4 because of byte type
      button1_flag = true;
      buttons = 2;
      return 2;
    }
    else{
      button2_flag = true;
      buttons = 0;
      return 0;
    }
  }
  else if(digitalRead(LEFT_BUTTON)){
    if(millis() - button2_timer >= _delay2){
      button2_flag = true;
      buttons = -1;
      return -1;
    }
    else{
      button1_flag = true;
      buttons = 0;
      return 0;
    }
  }
  else if(digitalRead(RIGHT_BUTTON)){
    if(millis() - button2_timer >= _delay2){
      button2_flag = true;
      buttons = 1;
      return 1;
    }
    else{
      button1_flag = true;
      buttons = 0;
      return 0;
    }
  }
  else{
    button1_flag = true;
    button2_flag = true;
    buttons = 0;
    return 0;
  }
}

// --------------------------------
// Method for calibrating implement
// --------------------------------
void InterfacePlanter::calibrate(){
  // Temporary variables
  int _temp, _temp2, _temp3;
  
  // Position calibration
  lcd->write_buffer(L_CAL_POS, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_POS, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Width calibration
      lcd->write_buffer(L_CAL_POS, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_POS_AD, 3);
      
      lcd->write_screen(-1);

      // Loop through calibration process
      for(int i = 0; i < 3; i++){
        _temp = implement->getPositionCalibrationPoint(i);
        _temp2 = _temp / 10;
        
        if (_temp < 0){
          lcd->write_buffer('-', 3, 12);
        }
        else{
          lcd->write_buffer(' ', 3, 12);
        }
        lcd->write_buffer(abs(_temp2) + '0', 3, 13);
        lcd->write_buffer(abs(_temp) % 10 + '0', 3, 14);

        lcd->write_screen(3);

        while(checkButtons(0, 0) != 0){
        }

        // Adjust loop
        while(true){
          lcd->write_screen(1);
          checkButtons(0, 0);
          implement->adjust(buttons);
          
          if (buttons == 1){
            lcd->write_buffer('>', 3, 19);
          }
          else if (buttons == -1){
            lcd->write_buffer('<', 3, 19);
          }
          else if (buttons == 2){
            implement->adjust(0);

            implement->setPositionCalibrationData(i);

            break;
          }
          else {
            lcd->write_buffer(' ', 3, 19);
          }
        }
      }
      lcd->write_buffer(L_CAL_POS, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);

      break;
    }
  }
  delay(1000);

#ifndef GPS
  // XTE calibration
  lcd->write_buffer(L_CAL_XTE, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){

    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_XTE, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);

      break;
    }
    else if(checkButtons(0, 0) == 1){
      // XTE calibration
      lcd->write_buffer(L_CAL_XTE, 0);
      lcd->write_buffer(L_BLANK, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_XTE_AD, 3);

      lcd->write_screen(-1);

      // Loop through calibration process
      for(int i = 0; i < 3; i++){
        _temp = implement->getXteCalibrationPoint(i);
        _temp2 = _temp / 10;
        
        if (_temp < 0){
          lcd->write_buffer('-', 3, 12);
        }
        else{
          lcd->write_buffer(' ', 3, 12);
        }
        lcd->write_buffer(abs(_temp2) + '0', 3, 13);
        lcd->write_buffer(abs(_temp) % 10 + '0', 3, 14);

        lcd->write_screen(3);

        while(checkButtons(0, 0) != 0){
        }

        delay(1000);

        // Adjust loop
        while(true){ 
          if (checkButtons(0, 0) == 2){
            implement->setXteCalibrationData(i);

            break;
          }
          lcd->write_screen(1);
        }
      }
      lcd->write_buffer(L_CAL_XTE, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);

      break;
    }
  }
  delay(1000);
#endif

  // Adjust KP
  lcd->write_buffer(L_CAL_KP, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_KP, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Adjust KP
      lcd->write_buffer(L_CAL_KP, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_KP_AD, 3);

      _temp = implement->getKP();
      _temp2 = _temp / 10;
      _temp3 = _temp / 100;

      lcd->write_buffer(_temp3 + '0', 3, 13);
      lcd->write_buffer('.', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      while(checkButtons(0, 0) != 0){
      }
      
      // Adjust loop
      while(true){
        lcd->write_screen(1);
        checkButtons(0, 255);

        if (buttons == 1){
          _temp ++;
        }
        else if (buttons == -1){
          _temp --;
        }
        else if (buttons == 2){
          break;
        }
        // Calculate derived variables
        _temp2 = _temp / 10;
        _temp3 = _temp / 100;

        // Write to screen
        lcd->write_buffer(_temp3 + '0', 3, 13);
        lcd->write_buffer('.', 3, 14);
        lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
        lcd->write_buffer(_temp  % 10 + '0', 3, 16);
      }
      lcd->write_buffer(L_CAL_KP, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_CAL_KP_AD, 3);

      lcd->write_buffer(_temp3 + '0', 3, 13);
      lcd->write_buffer('.', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      lcd->write_screen(-1);

      implement->setKP(_temp);
      break;
    }
  }
  delay(1000);

  // Adjust KI
  lcd->write_buffer(L_CAL_KI, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_KI, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Adjust KI
      lcd->write_buffer(L_CAL_KI, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_KI_AD, 3);

      _temp = implement->getKI();
      _temp2 = _temp / 10;
      _temp3 = _temp / 100;

      lcd->write_buffer(_temp3 + '0', 3, 13);
      lcd->write_buffer('.', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      while(checkButtons(0, 0) != 0){
      }

      while(true){
        lcd->write_screen(1);
        checkButtons(0, 255);
        
        if (buttons == 1){
          _temp ++;
        }
        else if (buttons == -1){
          _temp --;
        }
        else if (buttons == 2){
          break;
        }
        // Calculate derived variables
        _temp2 = _temp / 10;
        _temp3 = _temp / 100;

        // Write to screen
        lcd->write_buffer(_temp3 + '0', 3, 13);
        lcd->write_buffer('.', 3, 14);
        lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
        lcd->write_buffer(_temp  % 10 + '0', 3, 16);
      }
      lcd->write_buffer(L_CAL_KI, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_CAL_KI_AD, 3);

      lcd->write_buffer(_temp3 + '0', 3, 13);
      lcd->write_buffer('.', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      lcd->write_screen(-1);

      implement->setKI(_temp);
      break;
    }
  }
  delay(1000);

  // Adjust KD
  lcd->write_buffer(L_CAL_KD, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_KD, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Adjust KD
      lcd->write_buffer(L_CAL_KD, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_KD_AD, 3);

      _temp = implement->getKD();
      _temp2 = _temp / 10;
      _temp3 = _temp / 100;

      lcd->write_buffer(_temp3 + '0', 3, 13);
      lcd->write_buffer('.', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      while(checkButtons(0, 0) != 0){
      }

      while(true){
        lcd->write_screen(1);
        checkButtons(0, 255);
        
        if (buttons == 1){
          _temp ++;
        }
        else if (buttons == -1){
          _temp --;
        }
        else if (buttons == 2){
          break;
        }
        // Calculate derived variables
        _temp2 = _temp / 10;
        _temp3 = _temp / 100;

        // Write to screen
        lcd->write_buffer(_temp3 + '0', 3, 13);
        lcd->write_buffer('.', 3, 14);
        lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
        lcd->write_buffer(_temp  % 10 + '0', 3, 16);
      }
      lcd->write_buffer(L_CAL_KD, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_CAL_KD_AD, 3);

      lcd->write_buffer(_temp3 + '0', 3, 13);
      lcd->write_buffer('.', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      lcd->write_screen(-1);

      implement->setKD(_temp);
      break;
    }
  }
  delay(1000);

  // Adjust PWM manual
  lcd->write_buffer(L_CAL_PWM_M, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_PWM_M, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Adjust PWM manual
      lcd->write_buffer(L_CAL_PWM_M, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_PWM_M_AD, 3);

      _temp = implement->getPwmMan();
      _temp2 = _temp / 10;
      _temp3 = _temp / 100;

      lcd->write_buffer(_temp3 + '0', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      while(checkButtons(0, 0) != 0){
      }

      while(true){
        lcd->write_screen(1);
        checkButtons(0, 255);

        if (buttons == 1){
          _temp ++;
        }
        else if (buttons == -1){
          _temp --;          
        }
        else if (buttons == 2){
          break;
        }
        // calculate derived variables
        _temp2 = _temp / 10;
        _temp3 = _temp / 100;

        // write all to screen
        lcd->write_buffer(_temp3 + '0', 3, 14);
        lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
        lcd->write_buffer(_temp  % 10 + '0', 3, 16);
      }
      lcd->write_buffer(L_CAL_PWM_M, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_CAL_PWM_M_AD, 3);
      
      lcd->write_buffer(_temp3 + '0', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      lcd->write_screen(-1);

      implement->setPwmMan(byte(_temp));
      break;
    }
  }
  delay(1000);

  // Adjust PWM auto
  lcd->write_buffer(L_CAL_PWM_A, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_PWM_A, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Adjust PWM auto
      lcd->write_buffer(L_CAL_PWM_A, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_PWM_A_AD, 3);

      _temp = implement->getPwmAuto();
      _temp2 = _temp / 10;
      _temp3 = _temp / 100;

      lcd->write_buffer(_temp3 % 10 + '0', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      while(checkButtons(0, 0) != 0){
      }

      while(true){
        lcd->write_screen(1);
        checkButtons(0, 255);

        if (buttons == 1){
          _temp ++;
        }
        else if (buttons == -1){
          _temp --;
        }
        else if (buttons == 2){
          break;
        }
        // Calculate derived variables
        _temp2 = _temp / 10;
        _temp3 = _temp / 100;

        // Write all to screen
        lcd->write_buffer(_temp3 % 10 + '0', 3, 14);
        lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
        lcd->write_buffer(_temp  % 10 + '0', 3, 16);
      }
      lcd->write_buffer(L_CAL_PWM_A, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_CAL_PWM_A_AD, 3);

      lcd->write_buffer(_temp3 + '0', 3, 14);
      lcd->write_buffer(_temp2 % 10 + '0', 3, 15);
      lcd->write_buffer(_temp  % 10 + '0', 3, 16);

      lcd->write_screen(-1);

      implement->setPwmAuto(byte(_temp));
      break;
    }
  }
  delay(1000);

  // Adjust error
  lcd->write_buffer(L_CAL_OFFSET, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_OFFSET, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);
      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Adjust error
      lcd->write_buffer(L_CAL_OFFSET, 0);
      lcd->write_buffer(L_CAL_ADJUST, 1);
      lcd->write_buffer(L_CAL_ENTER, 2);
      lcd->write_buffer(L_CAL_OFFSET_AD, 3);

      _temp = implement->getOffset();
      _temp2 = _temp / 10;

      lcd->write_buffer(_temp2 + '0', 3, 15);
      lcd->write_buffer(_temp % 10 + '0', 3, 16);

      while(checkButtons(0, 0) != 0){
      }

      while(true){
        lcd->write_screen(1);
        checkButtons(0, 255);

        if (buttons == 1){
          _temp ++;
        }
        else if (buttons == -1){
          _temp --;
        }
        else if (buttons == 2){
          break;
        }
        // calculate derived variables
        _temp2 = _temp / 10;

        // write all to screen
        lcd->write_buffer(_temp2 + '0', 3, 15);
        lcd->write_buffer(_temp % 10 + '0', 3, 16);
      }
      lcd->write_buffer(L_CAL_OFFSET, 0);
      lcd->write_buffer(L_CAL_DONE, 1);
      lcd->write_buffer(L_BLANK, 2);
      lcd->write_buffer(L_CAL_OFFSET_AD, 3);

      lcd->write_buffer(_temp2 + '0', 3, 15);
      lcd->write_buffer(_temp % 10 + '0', 3, 16);

      lcd->write_screen(-1);

      implement->setOffset(_temp);
      break;
    }
  }
  delay(1000);

  // Store calibration data
  lcd->write_buffer(L_CAL_COMPLETE, 0);
  lcd->write_buffer(L_CAL_ACCEPT, 1);
  lcd->write_buffer(L_CAL_DECLINE, 2);
  lcd->write_buffer(L_BLANK, 3);

  lcd->write_screen(-1);

  while(checkButtons(0, 0) != 0){
  }

  while(true){
    if(checkButtons(0, 0) == -1){
      // print message to LCD
      lcd->write_buffer(L_CAL_COMPLETE, 0);
      lcd->write_buffer(L_CAL_DECLINED, 1);
      lcd->write_buffer(L_CAL_NOSAVE, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);

      implement->resetCalibration();

      break;
    }
    else if(checkButtons(0, 0) == 1){
      // Commit data
      implement->commitCalibration();

      // Print message to LCD
      lcd->write_buffer(L_CAL_COMPLETE, 0);
      lcd->write_buffer(L_CAL_DDONE, 1);
      lcd->write_buffer(L_CAL_SAVE, 2);
      lcd->write_buffer(L_BLANK, 3);

      lcd->write_screen(-1);

      break;
    }
  }
  delay(1000);
}