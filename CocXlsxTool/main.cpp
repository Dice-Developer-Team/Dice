#include "mainwindow.h"
#include <QApplication>

char* drag_file_path = NULL;

int main(int argc, char *argv[])
{
	if (argc >= 2)
		drag_file_path = argv[1];

	QApplication a(argc, argv);
    MainWindow w;
    w.show();
	
	return a.exec();
}
