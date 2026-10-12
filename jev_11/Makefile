CXX = clang++
#CXX = g++
CXXFLAGS = -shared -fPIC -std=c++17 -lcurl -luuid -lpqxx -lpq

TARGET = libsample.so
all: $(TARGET)

$(TARGET): sample.o
	$(CXX) $(CXXFLAGS) sample.o -o $(TARGET)

sample.o: sample.cpp
	$(CXX) $(CXXFLAGS) -c sample.cpp

clean:
	rm *.o $(TARGET)
