#include "file_watcher.hpp"
#include <iostream>
#include <chrono>

file_watcher::file_watcher(fs::path path, std::chrono::milliseconds interval, callback_t callback) :
	_path(path), _interval(interval), _callback(callback), _should_continue(false), _started(false)
{
	_last_write_time = fs::last_write_time(path);
}

void file_watcher::check()
{
	using hr_clock = std::chrono::high_resolution_clock;

	while (_should_continue)
	{
		auto current_write_time = fs::last_write_time(_path);
		if (_last_write_time < current_write_time)
		{
			_callback();
			_last_write_time = current_write_time;
		}

		// This is to avoid blocking the main thread during the stop() call because of the join()
		auto sleep_starting_time = hr_clock::now();
		while ((hr_clock::now() - sleep_starting_time) < _interval)
		{
			if (!_should_continue)
				return;
			std::this_thread::sleep_for(5ms);
		}
	}
}

void file_watcher::start()
{
	if (_started)
		return;

	if (!fs::exists(_path))
	{
		std::cout << "File " << _path << " does not exists!" << std::endl;
		return;
	}

	_should_continue = true;
	_watcher = std::thread(&file_watcher::check, this);
	_started = true;
}

void file_watcher::stop()
{
	if (!_started)
		return;

	_should_continue = false;
	_watcher.join();
	_started = false;
}
