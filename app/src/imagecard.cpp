#include "lib/imagecard.h"

ImageCard::ImageCard()
    : Card()
{}

ImageCard::ImageCard(QString questionText, QString answerText, Difficulty questionDifficulty)
    : Card(questionText, answerText, questionDifficulty)
{}

ImageCard::ImageCard(QString questionText, QString answerText, Difficulty questionDifficulty, QImage questionImage, QImage answerImage)
    : Card(questionText, answerText, questionDifficulty), m_questionImage(questionImage), m_answerImage(answerImage)
{}

ImageCard::~ImageCard()
{}

void ImageCard::setQuestionImage(const QImage& questionImage)
{
    m_questionImage = questionImage;
}

void ImageCard::setAnswerImage(const QImage& answerImage)
{
    m_answerImage = answerImage;
}
