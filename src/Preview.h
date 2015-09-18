/*
 * Preview.h
 *
 *  Created on: Sep 18, 2015
 *      Author: anba8005
 */

#ifndef SRC_PREVIEW_H_
#define SRC_PREVIEW_H_

#include <string>
using namespace std;

#include <Mlt.h>
using namespace Mlt;

class Preview {
public:
	Preview(const char* url);
	virtual ~Preview();

	bool is_available();
	void init();
	void render(Frame &frame);
private:
	string url;
	PushConsumer *consumer;
	Filter *deinterlacer;
	Filter *scaler;
};


#endif /* SRC_PREVIEW_H_ */
