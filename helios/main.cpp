#include "application.hpp"

// TODO(Corralx): Add a way to hide the console
int main(int, char*[])
{
	application app;
	if (!app.init())
		return -1;

	app.run();

	app.cleanup();
	return 0;
}
