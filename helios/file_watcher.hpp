#pragma once

#include <cstdint>
#include <functional>
#include <thread>
#include "common.hpp"

class file_watcher
{
public:
	using callback_t = std::function<void()>;

	file_watcher() = delete;
	file_watcher(fs::path path, std::chrono::milliseconds interval, callback_t callback);
	file_watcher(const file_watcher&) = delete;
	file_watcher(file_watcher&&) = default;
	~file_watcher() = default;

	file_watcher& operator=(const file_watcher&) = delete;
	file_watcher& operator=(file_watcher&&) = default;

	void start();
	void stop();
	
private:
	void check();

	fs::path _path;
	std::chrono::milliseconds _interval;
	callback_t _callback;

	bool _should_continue;
	fs::file_time_type _last_write_time;
	std::thread _watcher;
	bool _started;
};
