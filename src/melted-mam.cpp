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

class MeltedMAM: public Melted {
private:
	Event *event;
	PushConsumer *preview;
	string preview_url;

	Filter *crop;
	Filter *deinterlace;
	Filter *rescale;

public:
	MeltedMAM(char *name, int port, char *preview_url = NULL) :
			Melted(name, port, NULL), event(NULL), preview(NULL), crop(NULL), deinterlace(NULL), rescale(NULL) {
		if (preview_url != NULL)
			this->preview_url = string(preview_url);
	}

	virtual ~MeltedMAM() {
		if (event != NULL)
			delete event;
		if (preview != NULL)
			delete preview;
	}

	// Custom command execution
	Response *execute(char *command) {
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
		if (event == NULL && unit(0) != NULL) {
			//
			if (!preview_url.empty())
				create_preview();
			//
			Consumer consumer((mlt_consumer) (unit(0)->get_data("consumer")));
			event = consumer.listen("consumer-frame-show", this, (mlt_listener) frame_render);
			//
			Playlist playlist((mlt_playlist) (unit(0)->get_data("playlist")));
			playlist.set("eof", "loop");
		}

		return response;
	}

	void create_preview() {
		//
		Profile profile("quarter_pal_wide");
		crop = new Filter(profile, "crop");
		deinterlace = new Filter(profile, "deinterlace");
		rescale = new Filter(profile, "swscale");
		//
		if (preview_url == "console") {
			// sdl
			preview = new PushConsumer(profile, "sdl");
		} else {
			// network stream
			preview = new PushConsumer(profile, "avformat");
			preview->set("pix_fmt", "yuv420p");
			if (preview_url.find("http://") != string::npos) {
				// mpeg
				stringstream ss;
				ss << "/" << profile.width() << "/" << profile.height();
				preview_url += ss.str();
				preview->set("bf", 0);
				preview->set("g", 12);
				preview->set("vb", "1M");
				preview->set("f", "mpeg1video");
				preview->set("vcodec", "mpeg1video");
				preview->set("an", 1);
			} else {
				// rtmp
				preview->set("qscale", 16);
				preview->set("f", "flv");
				preview->set("vcodec", "flv1");
				preview->set("acodec", "libfaac");
				preview->set("ac", 2);
			}
		}
		//
		preview->set("deinterlace_method", "onefield");
		preview->set("target", preview_url.c_str());
		preview->start();
	}

	// Callback for frame render notification
	void frame_render_event(Frame &frame) {
		if (preview != NULL && !preview->is_stopped()) {
			crop->process(frame);
			deinterlace->process(frame);
			rescale->process(frame);
			preview->push(frame);
		}
	}

	static void frame_render(mlt_consumer, MeltedMAM *self, mlt_frame frame_ptr) {
		Frame frame(frame_ptr);
		self->frame_render_event(frame);
	}

	static void filter_destructor(void *arg) {
		Filter *filter = (Filter *) arg;
		delete filter;
	}

};

int main(int argc, char** argv) {
	//
	setenv("MLT_REPOSITORY", "/usr/local/mam/lib/mlt", true);
	setenv("MLT_DATA", "/usr/local/mam/share/mlt", true);
	//
	if (argc < 3) {
		cerr << "Usage : melted-mam <id> <profile> [<preview_url>]" << endl;
		cerr << "Profiles : dv_pal_wide , atsc_1080i_50, atsc_720p_50" << endl;
		return EXIT_FAILURE;
	}
	//
	int id;
	try {
		id = atoi(argv[1]);
	} catch (...) {
		cerr << "Invalid id " << argv[1] << endl;
		return EXIT_FAILURE;
	}
	setenv("MLT_PROFILE", argv[2], true);
	//
	char* url = NULL;
	if (argc == 4) {
		url = argv[3];
	}
	//
	stringstream port;
	port << "5250" << (id < 0 ? 0 : id);
	MeltedMAM server("melted-mam", atoi(port.str().c_str()), url);
	//
	stringstream unit;
	if (id < 0) {
		unit << "UADD sdl";
	} else {
		unit << "UADD decklink:" << id;
	}
	server.start();
	server.execute((char*)unit.str().c_str());
	server.wait_for_shutdown();
	return 0;
}

