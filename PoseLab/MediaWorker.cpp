#include "pch.h"

#include "MediaWorker.h"

MediaWorker::MediaWorker(const std::function<void()>&& function)
	: function(function)
	, active(false)
{}

 MediaWorker::~MediaWorker() {
	end();
}


void MediaWorker::run() {
	active = true;
	function();

	while (active) {
		QCoreApplication::processEvents();
	}

	// TODO Won't end but may be blocked because QApplication ends
	// -> worker hangs in video frame processing
	// exec();

	active = false;
	return;
}

void MediaWorker::end() {
	active = false;

	// TODO Won't end exec()
	exit();
	quit();
	wait();
}