/*
 * Preview.cpp
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#include "Preview.h"
#include <sstream>

Preview::Preview(const char* url) :
		consumer(NULL), deinterlacer(NULL), scaler(NULL) {
	if (url != NULL)
		this->url = string(url);
}

Preview::~Preview() {
	if (consumer != NULL)
		delete consumer;
	if (deinterlacer != NULL)
		delete deinterlacer;
	if (scaler != NULL)
		delete scaler;
}

bool Preview::is_available() {
	return !url.empty();
}

void Preview::init() {
	if (!is_available())
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
		} else {
			// rtmp
			consumer->set("qscale", 16);
			consumer->set("f", "flv");
			consumer->set("vcodec", "flv1");
			consumer->set("acodec", "libfaac");
			consumer->set("ac", 2);
		}
	}
	//
	consumer->set("deinterlace_method", "onefield");
	consumer->set("target", url.c_str());
	consumer->start();
	//
	deinterlacer = new Filter(profile, "deinterlace");
	scaler = new Filter(profile,"swscale");
}

void Preview::render(Frame &frame) {
	if (consumer != NULL && !consumer->is_stopped()) {
		deinterlacer->process(frame);
		scaler->process(frame);
		consumer->push(frame);
	}
}

