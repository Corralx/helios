#pragma once

#include <cstdint>
#include <functional>
#include <thread>
#include "common.hpp"

class file_watcher
{
public:
	using callback_t = std::function<void()>;
	using interval_t = std::chrono::milliseconds;

	explicit file_watcher(fs::path path = "", interval_t interval = interval_t::max(), callback_t callback = callback_t());
	file_watcher(const file_watcher&) = delete;
	file_watcher(file_watcher&&) = default;
	~file_watcher() = default;

	file_watcher& operator=(const file_watcher&) = delete;
	file_watcher& operator=(file_watcher&&) = default;

	void start();
	void stop();

	/* The path of the file to look for changes */
	fs::path path;

	/* The interval between checks in milliseconds */
	interval_t interval;

	/* The callback to call when a change is detected */
	callback_t callback;

private:
	void _check();

	bool _should_continue;
	fs::file_time_type _last_write_time;
	std::thread _watcher;
	bool _started;
};
