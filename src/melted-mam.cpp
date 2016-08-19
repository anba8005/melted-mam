#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
using namespace std;

#include <Mlt.h>
#include <MltMelted.h>
#include <MltResponse.h>
using namespace Mlt;

#include "MeltedMAM.h"

bool isCommandAvailable() {
	pollfd fdinfo;
	fdinfo.fd = fileno(stdin);
	fdinfo.events = POLLIN;
	return poll(&fdinfo, 1, 1) > 0;
}

int main(int argc, char** argv) {
	// always dump core
	struct rlimit core_limits;
	core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &core_limits);
	//
	setenv("MLT_REPOSITORY", "/usr/local/mam/lib/mlt", true);
	setenv("MLT_DATA", "/usr/local/mam/share/mlt", true);
	mlt_log_set_level(MLT_LOG_INFO);
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
	port << "5250" << (id < 0 ? abs(id) : id);
	MeltedMAM server("melted-mam", atoi(port.str().c_str()), url);

	//
	stringstream unit;
	if (id < 0) {
		unit << "UADD sdl";
	} else {
		unit << "UADD decklink:" << id;
	}
	//
	if (!server.start()) {
		cerr << "Error starting server" << endl;
		return EXIT_FAILURE;
	}
	server.execute("SET root=");
	server.execute((char*) unit.str().c_str())->error_code();
	//
	while (server.is_running()) {
		if (isCommandAvailable()) {
			// handle command
			string command;
			getline(cin, command);
			//
			if (!command.empty()) {
				char* c = (char*) command.c_str();
				Response* result = server.execute(c);
				for (int i = 0; i < result->count(); i++) {
					cout << result->get(i) << endl;
				}
				cout.flush();
			}
		} else {
			// wait
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
	//
	return EXIT_SUCCESS;
}

