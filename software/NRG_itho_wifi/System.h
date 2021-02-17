#ifndef System_h
#define System_h

#include <Arduino.h>

/**
  System Class.

  @author Matthias Maderer
  @version 1.1.7
*/
class System {
  public:
    /**
      Returns the uptime of the arduino with a char pointer.
      Format: DAYS:HOURS:MINUTES:SECONDS
      Sample: 1:20:23:50 = 1 day, 20 hours, 23 minutes and 50 seconds
      @return char *: pointer!
    */
    char * uptime();

    /**
      Returns the free RAM
      @return int: free RAM
    */
    int ramFree();

    /**
      Returns the size of the RAM
      @return int: RAM size
    */
    int ramSize();

    bool updateFreeMem();

    int getMemHigh();

    int getMemLow();

    uint32_t getMaxFreeBlockSize();

  private:
    
    int memHigh;
    int memLow { 1000000 };
    uint32_t memMaxBlock;
    char retval[25];
};

#endif
