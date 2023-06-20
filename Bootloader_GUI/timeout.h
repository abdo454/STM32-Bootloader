/*
 * serialport.h
 *  Created on: 18 Jun, 2022
 *  Author: abdo daood < abdo454daood@gmail.com >
 */
#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <stdlib.h>
#include <sys/time.h>

class timeOut {
public:
  // Constructor
  timeOut();

  // Init the timer
  void initTimer();

  // Return the elapsed time since initialization
  unsigned long int elapsedTime_ms();

private:
  // Used to store the previous time (for computing timeout)
  struct timeval previousTime;
};
#endif // TIMEOUT_H
