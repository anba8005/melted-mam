/*
 * MeltedMAM.cpp
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#include "MeltedMAM.h"

MeltedMAM::MeltedMAM(char *name, int port, char *preview_url) :
		Melted(name, port, NULL), show_event(NULL), render_event(NULL), preview(preview_url), profile(NULL), consumer(
		NULL), property_event(NULL), changed_event(NULL), playlist(NULL), last_playlist_speed(0), preload_index(-1) {
	// TODO Auto-generated constructor stub

}

MeltedMAM::~MeltedMAM() {
	if (show_event != NULL)
		delete show_event;
	if (render_event != NULL)
		delete render_event;
	if (property_event != NULL)
		delete property_event;
	if (changed_event != NULL)
		delete changed_event;
	if (consumer != NULL)
		delete consumer;
	if (playlist != NULL)
		delete playlist;
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
	std::unique_lock<std::mutex> lock(m);
	if (render_event == NULL && unit(0) != NULL) {
		//
		preview.init();
		//
		unit(0)->set("playing_position_fix", 1);
		unit(0)->set("sin_skip_goto", 1);
		unit(0)->set("sout_skip_goto", 1);
		consumer = new Consumer((mlt_consumer) (unit(0)->get_data("consumer")));
		consumer->set("priority", "max");
		consumer->set("buffer", 50);
		//
		playlist = new Playlist((mlt_playlist) (unit(0)->get_data("playlist")));
		//
		show_event = consumer->listen("consumer-frame-show", this, (mlt_listener) frame_show);
		render_event = consumer->listen("consumer-frame-render", this, (mlt_listener) frame_render);
		profile = new Profile(consumer->get_profile());
		//
		property_event = playlist->listen("property-changed", this, (mlt_listener) property_changed);
		changed_event = playlist->listen("playlist-current-changed", this, (mlt_listener) playlist_current_changed);
		//
		preload_thread = std::thread([this] {MeltedMAM::preload_worker();});

	}

	return response;
}

// Callback for frame show - after filters
void MeltedMAM::frame_show_event(Frame &frame) {
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
	double speed = playlist->get_speed();
	if (consumer->get_int("refresh") == 1) {
		consumer->purge();
		consumer->set("refresh", 0);
		if (last_playlist_speed != 0 && playlist->get_speed() == 0)
			playlist->seek(consumer->position());
		preview.purge();
	}
	last_playlist_speed = speed;
}

void MeltedMAM::frame_render(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr) {
	Frame frame(frame_ptr);
	self->frame_render_event(frame);
}

void MeltedMAM::property_changed(mlt_consumer, MeltedMAM *self, char* name) {
	if (name && !strcmp("length", name))
		self->preload_queue(self->playlist->current_clip());
}

void MeltedMAM::playlist_current_changed(mlt_playlist, MeltedMAM *self, int index) {
	self->preload_queue(index);
}

void MeltedMAM::preload_queue(int index) {
	std::unique_lock<std::mutex> lock(preload_mutex);
	preload_index = index;
	preload_condition.notify_one();
}

void MeltedMAM::preload_worker() {
	while (true) {
		std::unique_lock<std::mutex> lock(preload_mutex);
		if (preload_index > -1) {
			//
			int i = preload_index + 1;
			preload_index = -1;
			lock.unlock();
			//
			if (playlist->count() == i)
				i = 0;
			//
			if (playlist->current_clip() == i)
				continue;
			//
			Producer* producer = playlist->get_clip(i);
			mlt_log_info( NULL, "PRELOADING == current %i preload %i\n", playlist->current_clip(), i);
			if (producer->get_speed() == 0) {
				Frame* frame = producer->get_frame();
				frame->dec_ref();
				delete frame;
			}
			//
			lock.lock();
			if (preload_index > -1)
				continue;
		}
		//
		preload_condition.wait(lock);
	}
}

void MeltedMAM::filter_destructor(void *arg) {
	Filter *filter = (Filter *) arg;
	delete filter;
}

