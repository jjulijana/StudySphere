#ifndef DECKSTATS_H
#define DECKSTATS_H

#include "card.h"
#include "deck.h"
#include "user.h"
#include <QDate>
#include <QMap>

class DeckStats : public Serializable
{
  private:
	QVector<unsigned> m_grades;
	unsigned m_timesUsed;
	QString m_creationDate;

  public:
	DeckStats();
	DeckStats(unsigned numGrades);
	DeckStats(const DeckStats &deckStats);
	~DeckStats() override;

	inline QVector<unsigned> grades() { return m_grades; }

	inline unsigned int timesUsed() const { return m_timesUsed; }

	inline QString creationDate() const { return m_creationDate; }

	void addGrade(unsigned cardIndex, unsigned grade);
	void usedDeck();

	QVariant toVariant() const override;
	void fromVariant(const QVariant &variant) override;
};

#endif // DECKSTATS_H
