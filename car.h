class Car
{
private:
    int from;
    int to;
    int speed;
public:
    Car();
    Car& operator=(const Car& other) = default;
    Car& operator=(Car&& other) noexcept = default;
    void changeSpeed(int);
    void changeFrom(int);
    int getFrom();
    int getTo();
    int getSpeed();
    void changeLocation(int,int);
};