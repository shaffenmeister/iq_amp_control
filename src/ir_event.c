#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>
#include <mpd/message.h>

#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

enum log_level {
	DEBUG = 0,
	INFO = 1,
	ERROR = 2
};

pthread_t tid[1];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int g_isRunning = 1;
int g_logLevel = DEBUG; //ERROR;

struct mpd_connection *conn;

void
INT_handler(){
	g_isRunning = 0;
}

void
log_message(int level, char *format, ...)
{
	if(level >= g_logLevel) {
		va_list arglist;
		va_start( arglist, format );
		vfprintf( stderr, format, arglist );
		va_end( arglist );
		fprintf(stderr,"\n");
	}
}

/* MPD client helpers */
static int
handle_error(struct mpd_connection *c)
{
	int err = mpd_connection_get_error(c);
	
	assert(err != MPD_ERROR_SUCCESS);

	log_message(DEBUG, "%i --> %s\n", mpd_connection_get_error_message(c));
	mpd_connection_free(c);
	return EXIT_FAILURE;
}

bool
run_play_pos(struct mpd_connection *c, int pos)
{
	if(!mpd_run_play_pos(c,pos)) {
		if(mpd_connection_get_error(c) != MPD_ERROR_SERVER)
			return handle_error(c);
	}
	
	return true;
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

/* mpd keepalive thread: set volume every five seconds */
void* monitor(void *arg)
{
	pthread_t id = pthread_self();
	unsigned long cnt=0;
	while(g_isRunning) {
		log_message(INFO,"%i: thread keepalive %10ld",id,cnt);
		pthread_mutex_lock( &mutex1 );
		if(!mpd_run_change_volume(conn,0))
			if(!mpd_connection_clear_error(conn))
				handle_error(conn);
		cnt++;
		pthread_mutex_unlock( &mutex1 );
		usleep(5000000);
	}
	log_message(INFO,"%i: leaving thread",id);
	pthread_exit(NULL);
}

/*
 * Main
 */
int main()
{
	char devname[] = "/dev/input/event0";
	int device;
	struct input_event ev;
	struct mpd_status* mpd_state_ptr;
	int rc;
	pthread_attr_t attr;
	void *status;

/* Initialize and set thread detached attribute */
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
/*
 * Init MPD connection 
 */
conn = mpd_connection_new(NULL, 0, 30000);

if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
	return handle_error(conn);

//mpd_connection_set_keepalive(conn,true);

rc = pthread_create(&(tid[0]), &attr, &monitor, NULL);
pthread_attr_destroy(&attr);

device = open(devname, O_RDONLY);
signal(SIGINT, INT_handler);

while(g_isRunning!=0)
{
	// Read event
	read(device, &ev, sizeof(ev));
	if(ev.type != EV_KEY)
		continue;

	pthread_mutex_lock( &mutex1 );
	switch(ev.code) {
		case KEY_POWER:
			log_message(INFO,">>> POWER");
			break;
		case KEY_MUTE:
			if(ev.value == 1) {
				if(!mpd_run_change_volume(conn,-1))
					if(!mpd_connection_clear_error(conn))
						handle_error(conn);
				log_message(INFO,">>> MUTE");
			}
			break;
		case KEY_STOP:
			if(ev.value == 1) {
				mpd_state_ptr = get_status(conn);

				if(mpd_status_get_state(mpd_state_ptr) == MPD_STATE_PLAY ||
					mpd_status_get_state(mpd_state_ptr) == MPD_STATE_PAUSE) {
					if(!mpd_run_stop(conn))
						handle_error(conn);
					log_message(INFO,">>> STOP");
				}

				mpd_status_free(mpd_state_ptr);
			}
			break;
		case KEY_PLAYPAUSE:
			if(ev.value == 1) {
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
				log_message(INFO,">>> PLAYPAUSE");
			}
			break;
		case KEY_VOLUMEUP:
			if(ev.value == 1 || ev.value == 2) {
				if(!mpd_run_change_volume(conn,1))
					if(!mpd_connection_clear_error(conn))
						handle_error(conn);
				log_message(INFO,">>> VOLUMEUP");
			}
			break;
		case KEY_VOLUMEDOWN:
			if(ev.value == 1 || ev.value == 2) {
				if(!mpd_run_change_volume(conn,-1))
					if(!mpd_connection_clear_error(conn))
						handle_error(conn);
				log_message(INFO,">>> VOLUMEDOWN");
			}
			break;
		case KEY_NEXT:
			if(ev.value == 1) {
				if(!mpd_run_next(conn))
					handle_error(conn);
				log_message(INFO,">>> NEXT");
			}
			break;
		case KEY_PREVIOUS:
			if(ev.value == 1) {
				if(!mpd_run_previous(conn))
					handle_error(conn);
				log_message(INFO,">>> PREVIOUS");
			}
			break;
		case KEY_1:
			if(ev.value == 1)
				run_play_pos(conn, 0);
			break;
		case KEY_2:
			if(ev.value == 1)
				run_play_pos(conn, 1);
			break;
		case KEY_3:
			if(ev.value == 1)
				run_play_pos(conn, 2);
			break;
		case KEY_4:
			if(ev.value == 1)
				run_play_pos(conn, 3);
			break;
		case KEY_5:
			if(ev.value == 1)
				run_play_pos(conn, 4);
			break;
		case KEY_6:
			if(ev.value == 1)
				run_play_pos(conn, 5);
			break;
		case KEY_7:
			if(ev.value == 1)
				run_play_pos(conn, 6);
			break;
		case KEY_8:
			if(ev.value == 1)
				run_play_pos(conn, 7);
			break;
		case KEY_9:
			if(ev.value == 1)
				run_play_pos(conn, 8);
			break;
		default:
			continue;
	} // case

	log_message(DEBUG,"\tKey: %i/0x%x Type: %i State: %i",ev.code,ev.code,ev.type,ev.value);
	pthread_mutex_unlock( &mutex1 );
} // while


pthread_mutex_destroy(&mutex1);

rc = pthread_join(tid[0], &status);
log_message(DEBUG,"%i: thread join (rc=%i)",tid[0],rc);

mpd_connection_free(conn);
pthread_exit(NULL);
}
