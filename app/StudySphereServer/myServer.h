#ifndef MYSERVER_H
#define MYSERVER_H

#include <QObject>
#include <QTcpServer>

class MyServer : public QObject{
    Q_OBJECT

public:
    explicit MyServer(QObject *parent = nullptr);

signals:

public slots:
    void startServer();

private slots:
    void newConnection();

private:
    QTcpServer *server;
    QString publicDecksFolder;
    QString usersInfoFolder;
    QString userDecksFolder;

    void sendUserDecks(QTcpSocket* socket, const QString& searchQuery);
    void searchAndSendDecks(QTcpSocket* socket, const QString& searchQuery);
    void saveDeck(const QString& deckName, const QByteArray& deckData);
    void saveDeckForUser(const QString& username, const QString& deckName, QByteArray& deckData);
    void readData();
};

#endif // MYSERVER_H
