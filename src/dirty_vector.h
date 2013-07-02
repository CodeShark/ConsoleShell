#ifndef _DIRTY_VECTOR_H__
#define _DIRTY_VECTOR_H__

#include <vector>
#include <map>

template <typename T>
class dirty_vector : public std::vector<T>
{
private:
    std::map<unsigned int, T> dirt;

public:
    T& getDirty(unsigned int i);
    void clean() { dirt.clear(); }
};

template <typename T>
T& dirty_vector<T>::getDirty(unsigned int i)
{
    typename std::map<unsigned int, T>::iterator it = dirt.find(i);
    if (it != dirt.end()) return it->second;

    dirt[i] = this->at(i);
    return dirt[i]; 
}

#endif // _DIRTY_VECTOR_H__
