/*
 * Preview.h
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#ifndef SRC_PREVIEW_H_
#define SRC_PREVIEW_H_

#include <string>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
using namespace std;

#include <Mlt.h>
using namespace Mlt;

class Preview {
public:
	Preview(const char* url);
	virtual ~Preview();

	bool is_available();
	void init();
	void purge();
	void render(Frame &frame);
	void worker();
private:
	string url;
	PushConsumer *consumer;
	Filter *scaler;
	thread t;
	mutex m;
	condition_variable c;
	deque<Frame*> frames;
};


#endif /* SRC_PREVIEW_H_ */
