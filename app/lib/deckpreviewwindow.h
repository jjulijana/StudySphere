#ifndef DECKPREVIEWWINDOW_H
#define DECKPREVIEWWINDOW_H

#include "deck.h"
#include <QWidget>

namespace Ui
{
class DeckPreviewWindow;
}

class DeckPreviewWindow : public QWidget
{
	Q_OBJECT

  public:
	explicit DeckPreviewWindow(QWidget *parent = nullptr);
	DeckPreviewWindow(Deck *deck, User &user, QWidget *parent = nullptr);
	~DeckPreviewWindow() override;

  signals:
	void sendPublicDeck(QString deckNameID);

  private slots:
	void on_pushButton_flip_preview_clicked();

	void on_pushButton_cancel_clicked();

	void on_pushButton_add_clicked();

	void on_pushButton_next_clicked();

  private:
	Ui::DeckPreviewWindow *ui;
	bool m_answerShowed = false;
	unsigned m_currentCardIndex = 0;
	User m_user;
	Deck m_deck;

	bool hasNextCard();
	void setDifficultyColor();
};

#endif // DECKPREVIEWWINDOW_H
