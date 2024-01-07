#include "car.h"

Car::Car()
{
    this->speed=0;
    this->from=0;
    this->to=1;
}

void Car::changeSpeed(int speed)
{
    this->speed=speed;
}

void Car::changeFrom(int from)
{
    this->from=from;
}

int Car::getFrom()
{
    return this->from;
}

int Car::getTo()
{
    return this->to;
}

int Car::getSpeed()
{
    return this->speed;
}

void Car::changeLocation(int to, int from)
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
