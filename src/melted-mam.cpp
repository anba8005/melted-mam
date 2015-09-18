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

#include "MeltedMAM.h"

int main(int argc, char** argv) {
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
	server.execute((char*) unit.str().c_str());

	//server.execute("LOAD U0 /mnt/nfs/archive/bbbb-dv.mov");
//	server.execute("apnd U0 /mnt/nfs/archive/bbbb-imx.mxf");
//	server.execute("LOAD U0 /mnt/nfs/a.MP4");
	//server.execute("PLAY U0");

	server.wait_for_shutdown();
	return 0;
}

