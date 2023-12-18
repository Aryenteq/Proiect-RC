#include "car.h"

Car::Car()
{
    this->speed=0;
    this->from=0;
    this->to=1;
}

void Car::ChangeSpeed(int speed)
{
    this->speed=speed;
}

void Car::ChangeLocation(int to, int from)
{
    if(to<from)
        std::swap(to, from);
    if(to==from)
    {
        std::cout<<"Nu exista drum de la "<<to<<" la"<<from;
        return;
    }
    this->to=to;
    this->from=from;
}
