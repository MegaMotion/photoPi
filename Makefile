CC=g++

photoPi: src/dataSource.cpp include/dataSource.h src/main.cpp
	$(CC) -o photoPi src/dataSource.cpp src/main.cpp
