#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "lib/serializable.h"

class Serializer
{
public:
    virtual ~Serializer() = default;
    virtual void save(const Serializable &serializable, const QString &filepath, const QString &rootname = "") = 0; //third argument is only for XML
    virtual void load(Serializable &serializable, const QString &filepath) = 0;
};

#endif // SERIALIZER_H