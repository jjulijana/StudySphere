#ifndef CARDSTATS_H
#define CARDSTATS_H

#include "grading.h"

class CardStats : public Grading
{
public:
    CardStats();
    ~CardStats();

    // override
//    void addNewGrade(unsigned grade) override;
};

#endif // CARDSTATS_H
