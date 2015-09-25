/*
 * MeltedMAM.h
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#ifndef SRC_MELTEDMAM_H_
#define SRC_MELTEDMAM_H_

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <stdlib.h>
using namespace std;

#include <Mlt.h>
#include <MltMelted.h>
#include <MltResponse.h>
using namespace Mlt;

#include "Preview.h"

class MeltedMAM: public Melted {
public:
	MeltedMAM(char *name, int port, char *preview_url);
	virtual ~MeltedMAM();

	virtual Response* execute(char *command);

private:
	Event* show_event;
	Event* render_event;
	Event* property_event;
	Profile* profile;
	Consumer* consumer;
	Playlist* playlist;
	double last_playlist_speed;

	Preview preview;

	void frame_show_event(Frame &frame);
	void frame_render_event(Frame &frame);
	void property_changed_event(char *name);

	static void frame_show(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr);
	static void frame_render(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr);
	static void property_changed(mlt_consumer, MeltedMAM *self, char* name);
	static void filter_destructor(void *arg);
};

#endif /* SRC_MELTEDMAM_H_ */
