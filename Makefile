# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Targets
all: test_robot RobotWarz

RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -c RobotBase.cpp

test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot


Arena.o: Arena.cpp Arena.h
	$(CXX) $(CXXFLAGS) -c Arena.cpp

RobotWarz.o: RobotWarz.cpp Arena.h Arena.cpp 
	$(CXX) $(CXXFLAGS) -c RobotWarz.cpp

RobotWarz: RobotBase.o RobotWarz.o Arena.o
	$(CXX) $(CXXFLAGS) RobotWarz.o RobotBase.o Arena.o -o RobotWarz

clean:
	rm -f *.o test_robot RobotWarz *.so
