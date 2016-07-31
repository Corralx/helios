#include "application.hpp"

#if defined WIN32 && defined NDEBUG
#include <wincon.h> 
#endif

int main(int, char*[])
{
#if defined WIN32 && defined NDEBUG
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	application app;
	if (!app.init())
		return -1;

	app.run();

	app.cleanup();
	return 0;
}
