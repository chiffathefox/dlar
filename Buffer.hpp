
#pragma once


template<typename T>
class Buffer
{


public:

    explicit Buffer(int intialSize);

    T *data();
    T at(int i);
    void setAt(int i, T a);
    int size() const;

    T &operator[](int i);

};
