/*
 * serialport.h
 *  Created on: 18 Jun, 2022
 *  Author: abdo daood < abdo454daood@gmail.com >
 */

#include "timeout.h"

// Constructor
timeOut::timeOut() {}

/*
 * \brief  Initialise the timer. It writes the current time of the day in
 *         the structure PreviousTime
 */
void timeOut::initTimer() { gettimeofday(&previousTime, NULL); }

/*
 *  \brief      Returns the time elapsed since initialization.  It write the
 *  current time of the day in the structure CurrentTime. Then it returns the
 *  difference between CurrentTime and PreviousTime.
 *  \return     The number of microseconds elapsed since the functions InitTimer
 * was called.
 */

unsigned long int timeOut::elapsedTime_ms() {

  // Current time
  struct timeval CurrentTime;
  // Number of seconds and microseconds since last call
  int sec, usec;

  // Get current time
  gettimeofday(&CurrentTime, NULL);

  // Compute the number of seconds and microseconds elapsed since last call
  sec = CurrentTime.tv_sec - previousTime.tv_sec;
  usec = CurrentTime.tv_usec - previousTime.tv_usec;

  // If the previous usec is higher than the current one
  if (usec < 0) {
    // Recompute the microseonds and substract one second
    usec = 1000000 - previousTime.tv_usec + CurrentTime.tv_usec;
    sec--;
  }

  // Return the elapsed time in milliseconds
  return sec * 1000 + usec / 1000;
}
