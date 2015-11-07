/*
 * Preview.cpp
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#include "Preview.h"
#include <sstream>

Preview::Preview(const char* url) :
		consumer(NULL), scaler(NULL) {
	if (url != NULL)
		this->url = string(url);
}

Preview::~Preview() {
	if (consumer != NULL)
		delete consumer;
	if (scaler != NULL)
		delete scaler;
}

bool Preview::is_available() {
	return !url.empty();
}

void Preview::init() {
	std::unique_lock<std::mutex> lock(m);
	if (!is_available() || consumer != NULL)
		return;

	Profile profile("quarter_pal_wide");
	profile.set_width(640);
	profile.set_height(360);
	if (url == "console") {
		// sdl
		consumer = new PushConsumer(profile, "sdl");
	} else {
		// network stream
		consumer = new PushConsumer(profile, "avformat");
		consumer->set("pix_fmt", "yuv420p");
		if (url.find("http://") != string::npos) {
			// mpeg
			stringstream ss;
			ss << "/" << profile.width() << "/" << profile.height();
			url += ss.str();
			consumer->set("bf", 0);
			consumer->set("g", 12);
			consumer->set("vb", "1M");
			consumer->set("f", "mpeg1video");
			consumer->set("vcodec", "mpeg1video");
			consumer->set("an", 1);
		} else if (url.find("rtmp://") != string::npos) {
			// rtmp
			consumer->set("qscale", 16);
			consumer->set("f", "flv");
			consumer->set("acodec", "libfaac");
			consumer->set("ac", 2);
		}
	}
	//
	consumer->set("deinterlace_method", "onefield");
	consumer->set("target", url.c_str());
	consumer->start();
	//
	scaler = new Filter(profile, "avfilter");
	scaler->set("low_quality",true);
	//
	t = std::thread([this] {Preview::worker();});
}

void Preview::purge() {
	std::unique_lock<std::mutex> lock(m);
	frames.clear();
}

void Preview::render(Frame &frame) {
	std::unique_lock<std::mutex> lock(m);
	if (consumer != NULL && !consumer->is_stopped()) {
		Frame* f = new Frame(mlt_frame_clone(frame.get_frame(), true));
		frames.push_back(f);
		c.notify_all();
	}
}

void Preview::worker() {
	while (true) {
		std::unique_lock<std::mutex> lock(m);
		if (!frames.empty()) {
			Frame* frame = frames.front();
			frames.pop_front();
			lock.unlock();
			scaler->process(*frame);
			consumer->push(*frame);
			frame->dec_ref();
			delete frame;
		} else {
			c.wait(lock);
			continue;
		}
	}
}

