/*
 * MeltedMAM.cpp
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#include "MeltedMAM.h"

MeltedMAM::MeltedMAM(char *name, int port, char *preview_url) :
		Melted(name, port, NULL), show_event(NULL), render_event(NULL), preview(preview_url), profile(NULL), consumer(
		NULL), property_event(NULL), changed_event(NULL), playlist(NULL), last_playlist_speed(0) {
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
		unit(0)->set("playing_position_fix", 50); // buffer size
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
	// attach timecode - CRASHES melted-mam :)
/*	int timecode = frame.get_position();
	int fps = 25; // hardcoded :)
	int hh = (timecode / (3600 * fps)) % 24;
	timecode %= 3600 * fps;
	int mm = timecode / (60 * fps);
	timecode %= 60 * fps;
	int ss = timecode / fps;
	int ff = timecode % fps;
	//
	char* tc = new char[11];
	sprintf(tc,"%i:%i:%i:%i",hh,mm,ss,ff);
	frame.set("meta.attr.vitc.markup",tc); */
}

void MeltedMAM::frame_render(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr) {
	Frame frame(frame_ptr);
	self->frame_render_event(frame);
}

void MeltedMAM::property_changed(mlt_consumer, MeltedMAM *self, char* name) {
	if (name && !strcmp("length", name)) {
		self->preload_queue.enqueue(1);
	}
}

void MeltedMAM::playlist_current_changed(mlt_playlist, MeltedMAM *self, int index) {
	self->preload_queue.enqueue(1);
}

void MeltedMAM::preload_worker() {
	Frame* frame = NULL;
	Producer* producer = NULL;
	while (true) {

		//
		// wait for playlist settle
		//

		// get clip index
		int cache;
		preload_queue.wait_dequeue(cache);

		// wait
		std::this_thread::sleep_for(std::chrono::seconds(1));

		// lock
		playlist->lock();

		// skip if there's more indexes, playlist is "in action"
		if (preload_queue.try_dequeue(cache)) {
			// drain queue
			int cache2;
			while (preload_queue.try_dequeue(cache2))
				cache = cache2;
			// re-add last
			preload_queue.enqueue(cache);
			//
			playlist->unlock();
			continue;
		}

		//
		// playlist is settled for 1 sec - load clip after current
		//

		// check
		int i = playlist->current_clip() + 1;
		if (playlist->count() == i)
			i = 0;

		// destroy old frame
		if (frame) {
			delete frame;
			frame = NULL;
		}

		// destroy old producer
		if (producer) {
			delete producer;
			producer = NULL;
		}

		// get desired producer
		producer = playlist->get_clip(i);
		if (!producer) {
			playlist->unlock();
			continue;
		}

		// unlock
		playlist->unlock();

		// get frame (loads clip)
		if (producer->get_speed() == 0) {
			mlt_log_info(NULL, "PRELOADING == current %i preload %i\n", i - 1, i);
			frame = producer->get_frame();
		}

	}
}


