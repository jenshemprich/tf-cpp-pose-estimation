#pragma once


#include <QCoreApplication>
#include <QThread>

class MediaWorker : public QThread {
	Q_OBJECT

public:
	MediaWorker(const std::function<void()>&& function);
	~MediaWorker();

	void end();

protected:
	void run() override;

private:
	const std::function<void()> function;
	volatile bool active = false;
};
