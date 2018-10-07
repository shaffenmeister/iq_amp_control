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


#define ENCODER_A 23 // BCM
#define ENCODER_B 24 // BCM
#define PWM 0

#define ENCODER_BUTTON 17 // BCM

static volatile int encoderPos;
struct mpd_connection *conn;

/* forward declaration */

/* Pi GPIO callbacks */
void encoderPulse(int gpio, int lev, uint32_t tick);
void buttonEvent(int gpio, int lev, uint32_t tick);

/* MPD client helpers */
static int
handle_error(struct mpd_connection *c)
{
	assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);

	fprintf(stderr, "%s\n", mpd_connection_get_error_message(c));
	mpd_connection_free(c);
	return EXIT_FAILURE;
}

void
finish_command(struct mpd_connection *conn)
{
	if (!mpd_response_finish(conn))
		handle_error(conn);
}

struct mpd_status *
get_status(struct mpd_connection *conn)
{
	struct mpd_status *ret = mpd_run_status(conn);
	if (ret == NULL)
		handle_error(conn);

	return ret;
}

static void mpd_keepalive(void *conn)
{
  mpd_run_change_volume(conn,0);
}

int main(int argc, char * argv[])
{
	int pos=0;
	int incr = 0;

	 /*
	  * Init MPD connection
	  */
	 conn = mpd_connection_new(NULL, 0, 30000);

	 if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
	   return handle_error(conn);

	/*
	 * Init GPIO access
	 */

	/* Configure gpio clock to use the PWM source and not the PCM clock */
	gpioCfgClock(5,PWM,0);

	if (gpioInitialise()<0) return 1;

	gpioSetMode(ENCODER_A, PI_INPUT);
	gpioSetMode(ENCODER_B, PI_INPUT);
	gpioSetMode(ENCODER_BUTTON, PI_INPUT);
	gpioGlitchFilter(ENCODER_BUTTON, 20000);
	gpioSetAlertFunc(ENCODER_BUTTON, buttonEvent);

	/* pull up is needed as encoder common is grounded */
	gpioSetPullUpDown(ENCODER_A, PI_PUD_UP);
	gpioSetPullUpDown(ENCODER_B, PI_PUD_UP);
	gpioSetPullUpDown(ENCODER_BUTTON, PI_PUD_UP);

	encoderPos = pos;

	/* monitor encoder level changes */
	gpioSetAlertFunc(ENCODER_A, encoderPulse);
	gpioSetAlertFunc(ENCODER_B, encoderPulse);

	/* regular timer callback to keep connection to mpd alive */
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

void buttonEvent(int gpio, int level, uint32_t tick)
{
	static int last_lvl = -1;
	static uint32_t t_start = 0;
	uint32_t t_cur = tick;
	struct mpd_status* mpd_state_ptr;

	if(gpio == ENCODER_BUTTON)
	{
		if(level == 0)
		// button pressed
		{
			mpd_state_ptr = get_status(conn);

				switch(mpd_status_get_state(mpd_state_ptr)) {
					case MPD_STATE_PLAY:
							mpd_send_pause(conn,true);
							finish_command(conn);
						break;
					case MPD_STATE_PAUSE:
							if(!mpd_run_play(conn))
								handle_error(conn);
						break;
					case MPD_STATE_STOP:
							if(!mpd_run_play(conn))
								handle_error(conn);
						break;
					default:
						break;
				}

			mpd_status_free(mpd_state_ptr);
		}
		else
		// button released
		{

		}

		last_lvl = level;
	}
}
