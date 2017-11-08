#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <pigpio.h>

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>
#include <mpd/message.h>


/*
   Rotary encoder connections:

   Encoder A      - gpio 18   (pin P1-12)
   Encoder B      - gpio 7    (pin P1-26)
   Encoder Common - Pi ground (pin P1-20)
*/

#define ENCODER_A 23 // 18
#define ENCODER_B 24 //  7
#define PWM 0

static volatile int encoderPos;

/* forward declaration */

/* Pi GPIO callback */
void encoderPulse(int gpio, int lev, uint32_t tick);

/* MPD client helpers */
static int
handle_error(struct mpd_connection *c)
{
	assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);

	fprintf(stderr, "%s\n", mpd_connection_get_error_message(c));
	mpd_connection_free(c);
	return EXIT_FAILURE;
}

static void mpd_keepalive(void *conn)
{
  mpd_run_change_volume(conn,0);
}

int main(int argc, char * argv[])
{
   int pos=0;
   struct mpd_connection *conn;
   int incr = 0;

         /*
          * Init MPD connection 
          */
         conn = mpd_connection_new(NULL, 0, 30000);

         if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
           return handle_error(conn);

//         mpd_connection_set_keepalive(conn,true);

   /*
    * Init GPIO access
    */
   gpioCfgClock(5,PWM,0);

   if (gpioInitialise()<0) return 1;

   gpioSetMode(ENCODER_A, PI_INPUT);
   gpioSetMode(ENCODER_B, PI_INPUT);

   /* pull up is needed as encoder common is grounded */
   gpioSetPullUpDown(ENCODER_A, PI_PUD_UP);
   gpioSetPullUpDown(ENCODER_B, PI_PUD_UP);

   encoderPos = pos;

   /* monitor encoder level changes */
   gpioSetAlertFunc(ENCODER_A, encoderPulse);
   gpioSetAlertFunc(ENCODER_B, encoderPulse);
   gpioSetTimerFuncEx(0,10000,mpd_keepalive,conn);

   while (1)
   {
      if (pos != encoderPos)
      {
         if(pos > encoderPos)
           incr = 1;
         else
           incr = -1;

         if(!mpd_run_change_volume(conn,incr))
           if(!mpd_connection_clear_error(conn))
             handle_error(conn);


         pos = encoderPos;
//         printf("pos=%d\n", pos);
      }
      else
      {
//        mpd_run_change_volume(conn,0);
      }

      gpioDelay(20000); /* check pos 50 times per second */
   }

   gpioTerminate();
   mpd_connection_free(conn);
}

void encoderPulse(int gpio, int level, uint32_t tick)
{
   /*

             +---------+         +---------+      0
             |         |         |         |
   A         |         |         |         |
             |         |         |         |
   +---------+         +---------+         +----- 1

       +---------+         +---------+            0
       |         |         |         |
   B   |         |         |         |
       |         |         |         |
   ----+         +---------+         +---------+  1

   */

   static int levA=0, levB=0, lastGpio = -1;

   if (gpio == ENCODER_A) levA = level; else levB = level;

   if (gpio != lastGpio) /* debounce */
   {
      lastGpio = gpio;

      if ((gpio == ENCODER_A) && (level == 0))
      {
         if (!levB) ++encoderPos;
      }
      else if ((gpio == ENCODER_B) && (level == 1))
      {
         if (levA) --encoderPos;
      }
   }
}

