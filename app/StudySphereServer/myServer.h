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
    void readData();

private:
    QTcpServer *server;
    QString publicDecksFolder;

    void searchAndSendDecks(QTcpSocket* socket, const QString& searchQuery);
    void saveDeck(const QString& deckName, const QByteArray& deckData);

};

#endif // MYSERVER_H