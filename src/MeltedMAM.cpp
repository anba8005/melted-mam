/*
 * MeltedMAM.cpp
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#include "MeltedMAM.h"

MeltedMAM::MeltedMAM(char *name, int port, char *preview_url) :
		Melted(name, port, NULL), show_event(NULL), render_event(NULL), preview(preview_url), profile(NULL), consumer(
		NULL), property_event(NULL) {
	// TODO Auto-generated constructor stub

}

MeltedMAM::~MeltedMAM() {
	if (show_event != NULL)
		delete show_event;
	if (render_event != NULL)
		delete render_event;
	if (property_event != NULL)
		delete property_event;
	if (consumer != NULL)
		delete consumer;
}

// Custom command execution
Response* MeltedMAM::execute(char *command) {
	Response *response = NULL;

	if (!strcmp(command, "debug")) {
		// Example of a custom command
		response = new Response(200, "Diagnostics output");
		for (int i = 0; unit(i) != NULL; i++) {
			Properties *properties = unit(i);
			stringstream output;
			output << string("Unit ") << i << endl;
			for (int j = 0; j < properties->count(); j++)
				output << properties->get_name(j) << " = " << properties->get(j) << endl;
			response->write(output.str().c_str());
		}
	} else {
		// Use the default command processing
		response = Melted::execute(command);
	}

	// initialization
	if (render_event == NULL && unit(0) != NULL) {
		//
		preview.init();
		//
		consumer = new Consumer((mlt_consumer) (unit(0)->get_data("consumer")));
		consumer->set("priority", "max");
		consumer->set("buffer", 25);
		//
		show_event = consumer->listen("consumer-frame-show", this, (mlt_listener) frame_show);
		render_event = consumer->listen("consumer-frame-render", this, (mlt_listener) frame_render);
		property_event = consumer->listen("property-changed", this, (mlt_listener) property_changed);
		profile = new Profile(consumer->get_profile());
		//
		Playlist playlist((mlt_playlist) (unit(0)->get_data("playlist")));
		playlist.set("eof", "loop");
	}

	return response;
}

// Callback for frame show - after filters
void MeltedMAM::frame_show_event(Frame &frame) {
	//
	preview.render(frame);
}

void MeltedMAM::frame_show(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr) {
	Frame frame(frame_ptr);
	self->frame_show_event(frame);
}

// Callback for frame render - before filters
void MeltedMAM::frame_render_event(Frame &frame) {
	// IMX crop
	const char* width = frame.get("width");
	const char* height = frame.get("height");
	if (!strcmp("720", width) && !strcmp("608", height))
		frame.set("crop.top", 32);
	//
	if (consumer->get_int("refresh") == 1) {
		consumer->purge();
		consumer->set("refresh", 0);
	}
}

void MeltedMAM::frame_render(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr) {
	Frame frame(frame_ptr);
	self->frame_render_event(frame);
}

void MeltedMAM::property_changed_event(char *name) {
}

void MeltedMAM::property_changed(mlt_consumer, MeltedMAM *self, char* name) {
	self->property_changed_event(name);
}

void MeltedMAM::filter_destructor(void *arg) {
	Filter *filter = (Filter *) arg;
	delete filter;
}

