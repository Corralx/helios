#include "application.hpp"

int main(int, char*[])
{
	application app;
	if (!app.init())
		return -1;

	app.run();

	app.cleanup();
	return 0;
}
