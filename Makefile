CC=g++

dataSourceTest: src/dataSource.cpp include/dataSource.h src/main.cpp
	$(CC) -o dataSourceTest src/dataSource.cpp src/main.cpp
