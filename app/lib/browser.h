#ifndef BROWSER_H
#define BROWSER_H

#include <QVector>
#include <QString>
#include "deck.h"
#include "library.h"

class Browser{
public:
    Browser(const QString& publicDecksFolderPath, Library& library);
    void addDeckToLibrary(Deck& deck);

private:
    QString m_publicDecksFolder;
    QVector<Deck> m_publicDeckList;
    QVector<Deck> m_foundDecks;
    Library& m_library;

    void searchDecks(const QString& searchQuery);
    void loadPublicDecks();
    void importDeck(QString& filePath);
};

#endif // BROWSER_H
