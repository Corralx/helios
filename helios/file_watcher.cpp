#include "file_watcher.hpp"
#include <iostream>
#include <chrono>

file_watcher::file_watcher(fs::path _path, interval_t _interval, callback_t _callback) :
	path(_path), interval(_interval), callback(_callback),
	_should_continue(false), _last_write_time(), _started(false)
{
}

void file_watcher::_check()
{
	using hr_clock = std::chrono::high_resolution_clock;

	_last_write_time = fs::last_write_time(path);

	while (_should_continue)
	{
		auto current_write_time = fs::last_write_time(path);
		if (_last_write_time < current_write_time)
		{
			callback();
			_last_write_time = current_write_time;
		}

		// This is to avoid blocking the main thread during the stop() call because of the join()
		auto sleep_starting_time = hr_clock::now();
		while ((hr_clock::now() - sleep_starting_time) < interval)
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

	if (path.empty())
	{
		std::cout << "No file set to watch for!" << std::endl;
		return;
	}

	if (!fs::exists(path))
	{
		std::cout << "File " << path << " does not exists!" << std::endl;
		return;
	}

	_should_continue = true;
	_watcher = std::thread(&file_watcher::_check, this);
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
