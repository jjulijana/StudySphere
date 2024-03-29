#include "lib/mainwindow.h"
#include "lib/createdeckdialog.h"
#include "lib/createdeckwindow.h"
#include "lib/deckpreviewwindow.h"
#include "lib/jsonserializer.h"
#include "lib/logindialog.h"
#include "lib/scheduleitem.h"
#include "lib/servercommunicator.h"
#include "lib/studysessionwindow.h"
#include "lib/user.h"
#include "ui_mainwindow.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>

#include <QApplication>
#include <QCheckBox>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

enum Page
{
	LIBRARY = 0,
	TODO,
	PLANER,
	CALENDAR,
	STATS,
	SETTINGS,
	HELP
};

MainWindow::MainWindow(QWidget *parent)
	: QWidget(parent), ui(new Ui::MainWindow), m_planner(), m_toDoList(), m_deckNames(), m_user()
{
	ui->setupUi(this);
	ui->stackedWidget->setCurrentIndex(LIBRARY);

	for (int i = 0; i < 7; ++i) {
		m_plannerScenes.push_back(new PlannerScene());
	}
	ui->graphicsView_monday->setScene(m_plannerScenes[Day::MONDAY]);
	ui->graphicsView_tuesday->setScene(m_plannerScenes[Day::TUESDAY]);
	ui->graphicsView_wednesday->setScene(m_plannerScenes[Day::WEDNESDAY]);
	ui->graphicsView_thursday->setScene(m_plannerScenes[Day::THURSDAY]);
	ui->graphicsView_friday->setScene(m_plannerScenes[Day::FRIDAY]);
	ui->graphicsView_saturday->setScene(m_plannerScenes[Day::SATURDAY]);
	ui->graphicsView_sunday->setScene(m_plannerScenes[Day::SUNDAY]);

	for (int i = 0; i < 7; ++i) {
		auto *scheduleItem = new ScheduleItem();
		scheduleItem->setWidth(ui->graphicsView_monday->width());
		m_plannerScenes[i]->addItem(scheduleItem);
	}

	auto *app = qobject_cast<QApplication *>(QApplication::instance());
	Settings &settings = Settings::instance(app);

	settings.setLanguage(Language::SERBIAN);
	settings.setTheme(Theme::BLUE);
	ui->retranslateUi(this);

	ui->dateTimeEdit_eventTime->setDate(QDate::currentDate());
	ui->dateTimeEdit_eventTime->setTime(QTime(12, 0));

	connect(ui->listWidget_todos, &QListWidget::itemChanged, this, &MainWindow::onTodoItemChanged);
}

MainWindow::~MainWindow()
{
	if (m_loggedIn) {
		saveOnServer();
	}

	ui->tableWidget_library->clear();
	ui->tableWidget_browser->clear();
	ui->listWidget_todos->clear();

	delete ui;

	for (auto scene : m_plannerScenes) {
		scene->clear();
		delete scene;
	}

	m_planner.deleteAll();
	m_calendar.deleteAll();
	m_toDoList.deleteAllToDos();
}

void MainWindow::saveOnServer()
{
	if (m_calendarLoaded) {
		saveCalendar();
	}
	if (m_plannerLoaded) {
		savePlanner();
	}
	if (m_todoLoaded) {
		saveToDoList();
	}
}

void MainWindow::savePlanner()
{

	QJsonObject requestObject;

	JSONSerializer serializer;
	QJsonDocument doc = serializer.createJson(m_planner);

	requestObject["action"] = "savePlanner";
	requestObject["username"] = m_user.username();
	requestObject["planner"] = doc.toVariant().toJsonObject();

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;

	QJsonObject jsonObj = communicator.sendRequest(request);

	if (jsonObj["status"].toString() != "success") {
		QMessageBox::information(this, tr("Čuvanje planera"), tr("Došlo je do greške, planer nije sačuvan!"));
		return;
	}
}

void MainWindow::createDeck_clicked()
{
	CreateDeckDialog popUp(this);
	if (popUp.exec() == QDialog::Accepted) {
		QString name = popUp.getDeckName();
		Privacy privacy = popUp.getDeckPrivacy();

		auto *createDeck = new CreateDeckWindow(name, privacy, m_user);
		connect(createDeck, &CreateDeckWindow::writeGeneratedID, this, &MainWindow::addNewDeck);
		createDeck->setAttribute(Qt::WA_DeleteOnClose);
		createDeck->show();
	}
}

void MainWindow::deckPreview_clicked()
{
	auto *chosenDeck = qobject_cast<QPushButton *>(sender());
	QString deckName = chosenDeck->text();

	QJsonObject requestObject;
	requestObject["action"] = "sendDeck";
	requestObject["username"] = m_user.username();
	requestObject["DeckId"] = deckName.split('_')[1].split('.')[0];
	requestObject["Privacy"] = "PUBLIC";

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);
	JSONSerializer jsonSerializer;

	if (jsonObj["status"].toString() != "success") {
		QMessageBox::information(this, tr("Pregled špila"),
								 tr("Došlo je do greške, pregled špila se nije pokrenuo, probajte ponovo!"));
		return;
	}
	Deck *deck = new Deck();

	QJsonObject deckObject = jsonObj[deckName + "_deck.json"].toObject();
	QJsonDocument deckDocument = QJsonDocument::fromVariant(deckObject.toVariantMap());
	jsonSerializer.loadJson(*deck, deckDocument);

	auto *preview = new DeckPreviewWindow(deck, m_user);
	connect(preview, &DeckPreviewWindow::sendPublicDeck, this, &MainWindow::addNewDeck);
	preview->setAttribute(Qt::WA_DeleteOnClose);
	preview->show();
}

void MainWindow::deckButton_clicked()
{
	auto *chosenDeck = qobject_cast<QPushButton *>(sender());

	QString deckName = chosenDeck->text();
	QStringList deckNameEdited = deckName.split('.')[0].split('_');

	QJsonObject requestObject;

	requestObject["action"] = "sendDeck";
	requestObject["username"] = m_user.username();
	requestObject["DeckId"] = deckName.split('_')[1];
	requestObject["Privacy"] = "PRIVATE";

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);

	if (jsonObj["status"].toString() != "success") {
		QMessageBox::information(this, tr("Učenje"), tr("Došlo je do greške, špil se nije pokrenuo, probajte ponovo!"));
		return;
	}
	Deck *deck = new Deck();

	JSONSerializer jsonSerializer;

	QJsonObject deckObject = jsonObj[deckNameEdited[0].append("_").append(deckNameEdited[1]) + "_deck.json"].toObject();
	QJsonDocument deckDocument = QJsonDocument::fromVariant(deckObject.toVariantMap());

	jsonSerializer.loadJson(*deck, deckDocument);

	auto *session = new StudySession(m_user, deck);
	auto *useDeck = new StudySessionWindow(session);
	useDeck->show();
}

void MainWindow::on_pushButton_library_clicked()
{
	ui->stackedWidget->setCurrentIndex(LIBRARY);
}

void MainWindow::on_pushButton_todo_clicked()
{
	ui->stackedWidget->setCurrentIndex(TODO);
	if (!m_todoLoaded) {

		QJsonObject requestObject;
		requestObject["action"] = "getTodo";
		requestObject["username"] = m_user.username();

		QJsonDocument request(requestObject);

		ServerCommunicator communicator;
		QJsonObject jsonObj = communicator.sendRequest(request);

		if (jsonObj["status"].toString() != "success") {
			QMessageBox::information(this, tr("TODO lista"),
									 tr("Došlo je do greške, todo lista nije učitana, probajte ponovo!"));
			return;
		}

		JSONSerializer jsonSerializer;

		QJsonObject deckObject = jsonObj["todo"].toObject();
		QJsonDocument deckDocument = QJsonDocument::fromVariant(deckObject.toVariantMap());

		jsonSerializer.loadJson(m_toDoList, deckDocument);

		showActivities();
		m_todoLoaded = true;

		for (auto &todo : m_toDoList.toDos()) {
			auto *item = new QListWidgetItem();
			item->setCheckState(todo.second ? Qt::Checked : Qt::Unchecked);
			item->setText(todo.first);
			onTodoItemChanged(item);
			ui->listWidget_todos->addItem(item);
		}
	}
}

void MainWindow::setPlannerWidth(int width)
{
	ui->graphicsView_monday->setFixedWidth(width);
	ui->graphicsView_tuesday->setFixedWidth(width);
	ui->graphicsView_wednesday->setFixedWidth(width);
	ui->graphicsView_thursday->setFixedWidth(width);
	ui->graphicsView_friday->setFixedWidth(width);
	ui->graphicsView_saturday->setFixedWidth(width);
	ui->graphicsView_sunday->setFixedWidth(width);
}

void MainWindow::on_pushButton_planer_clicked()
{
	ui->stackedWidget->setCurrentIndex(PLANER);

	if (!m_plannerLoaded) {

		QJsonObject requestObject;
		requestObject["action"] = "getPlanner";
		requestObject["username"] = m_user.username();

		QJsonDocument request(requestObject);

		ServerCommunicator communicator;
		QJsonObject jsonObj = communicator.sendRequest(request);

		if (jsonObj["status"].toString() != "success") {
			QMessageBox::information(this, tr("Planer"),
									 tr("Došlo je do greške, planer nije učitan, probajte ponovo!"));
			return;
		}
		JSONSerializer jsonSerializer;

		QJsonObject deckObject = jsonObj["planner"].toObject();
		QJsonDocument deckDocument = QJsonDocument::fromVariant(deckObject.toVariantMap());

		jsonSerializer.loadJson(m_planner, deckDocument);

		showActivities();
		m_plannerLoaded = true;
	}
}

void MainWindow::on_pushButton_calendar_clicked()
{
	ui->stackedWidget->setCurrentIndex(CALENDAR);
	if (!m_calendarLoaded) {

		QJsonObject requestObject;
		requestObject["action"] = "getCalendar";
		requestObject["username"] = m_user.username();

		QJsonDocument request(requestObject);

		ServerCommunicator communicator;
		QJsonObject jsonObj = communicator.sendRequest(request);

		if (jsonObj["status"].toString() != "success") {
			QMessageBox::information(this, tr("Kalendar"),
									 tr("Došlo je do greške, kalendar nije učitan, probajte ponovo!"));
			return;
		}

		JSONSerializer jsonSerializer;

		QJsonObject deckObject = jsonObj["calendar"].toObject();
		QJsonDocument deckDocument = QJsonDocument::fromVariant(deckObject.toVariantMap());

		jsonSerializer.loadJson(m_calendar, deckDocument);

		m_calendarLoaded = true;
	}
}

void MainWindow::on_pushButton_stats_clicked()
{
	ui->stackedWidget->setCurrentIndex(STATS);
	ui->number_timesUsed->display(0);
	ui->number_deckSize->display(0);
	ui->progressBar_skip->setValue(0);
	ui->progressBar_bad->setValue(0);
	ui->progressBar_good->setValue(0);
	ui->progressBar_excellent->setValue(0);
}

void MainWindow::on_pushButton_settings_clicked()
{
	ui->stackedWidget->setCurrentIndex(SETTINGS);
}

void MainWindow::on_pushButton_addEvent_clicked()
{
	QString eventName = ui->lineEdit_event->text();
	QDate date = ui->dateTimeEdit_eventTime->dateTime().date();
	QTime time = ui->dateTimeEdit_eventTime->dateTime().time();

	if (eventName.trimmed().isEmpty()) {
		QMessageBox::warning(this, tr("Pogrešan unos"), tr("Niste popunili polje za naziv dogadjaja!"));
		return;
	}

	m_calendar.addEvent(date, time, eventName);

	ui->lineEdit_event->clear();
	ui->dateTimeEdit_eventTime->setDate(QDate::currentDate());
	ui->dateTimeEdit_eventTime->setTime(QTime(12, 0));

	QMessageBox::information(this, tr("Uspešan unos"), tr("Uspešno ste dodali novi dogadjaj!"));
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);

	setupTableView(ui->tableWidget_library);
	setupTableView(ui->tableWidget_browser);
}

void MainWindow::on_calendarWidget_activated(const QDate &date)
{
	QString message = tr("Na izabrani dan imate sledeće dogadjaje:\n");
	if (!m_calendar.events().contains(date))
		QMessageBox::information(this, date.toString("dd.MM.yyyy."), tr("Na izabrani dan nemate nijedan dogadjaj!"));
	else {
		auto calendar = m_calendar.events();
		auto todaysEvents = calendar[date];
		for (const auto &event : todaysEvents) {
			message += "\t" + event.first.toString("hh:mm") + " - " + event.second + "\n";
		}
		QMessageBox::information(this, date.toString("dd.MM.yyyy."), message);
	}
}

void MainWindow::on_pushButton_addActivity_clicked()
{
	QString name = ui->lineEdit_activityName->text();

	QTime startTime = ui->timeEdit_start->time();
	QTime endTime = ui->timeEdit_end->time();

	if (name.trimmed().isEmpty()) {
		QMessageBox::warning(this, tr("Pogrešan unos"), tr("Niste popunili polje za naziv aktivnosti!"));
		return;
	}

	if (startTime >= endTime) {
		QMessageBox::warning(this, tr("Pogrešan unos"), tr("Vreme početka aktivnosti mora biti pre vremena kraja!"));
		return;
	}

	QString dayString = ui->comboBox_day->currentText();
	Day day = Planner::dayFromString(dayString);

	Activity activity(name, startTime, endTime);

	m_planner.addActivity(day, activity);

	auto *activityItem = new ActivityItem(activity);
	m_plannerScenes[day]->addActivity(activityItem);
	m_plannerScenes[day]->addItem(activityItem);

	auto *activityTime = new QGraphicsTextItem();
	activityTime->setPlainText(startTime.toString("hh:mm"));
	activityTime->setPos(activityItem->pos());
	m_plannerScenes[day]->addItem(activityTime);

	auto *activityText = new QGraphicsTextItem();
	activityText->setPlainText(name);
	qreal textWidth = ui->graphicsView_monday->width() - 10;
	activityText->setTextWidth(textWidth);
	activityText->setPos(activityItem->pos().x(), activityItem->pos().y() + activityTime->boundingRect().height());
	m_plannerScenes[day]->addItem(activityText);

	ui->lineEdit_activityName->setText("");
	ui->timeEdit_start->setTime(QTime(0, 0));
	ui->timeEdit_end->setTime(QTime(0, 0));
}

void MainWindow::showActivities()
{
	auto keys = m_planner.activities().keys();
	for (auto day : keys) {
		for (const auto &currentActivity : m_planner.activities().value(day)) {
			QString name = currentActivity.activityText();

			QTime startTime = currentActivity.start();

			auto *activityItem = new ActivityItem(currentActivity);
			m_plannerScenes[day]->addActivity(activityItem);
			m_plannerScenes[day]->addItem(activityItem);

			auto *activityTime = new QGraphicsTextItem();
			activityTime->setPlainText(startTime.toString("hh:mm"));
			activityTime->setPos(activityItem->pos());
			m_plannerScenes[day]->addItem(activityTime);

			auto *activityText = new QGraphicsTextItem();
			activityText->setPlainText(name);
			qreal textWidth = ui->graphicsView_monday->width() - 10;
			activityText->setTextWidth(textWidth);
			activityText->setPos(activityItem->pos().x(),
								 activityItem->pos().y() + activityTime->boundingRect().height());
			m_plannerScenes[day]->addItem(activityText);
		}
	}
}

void MainWindow::setupTableView(QTableWidget *table)
{
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	table->setRowHeight(0, table->height() * 0.43);
	table->setRowHeight(1, table->height() * 0.05);
	table->setRowHeight(2, table->height() * 0.43);
	table->setRowHeight(3, table->height() * 0.05);
}

void MainWindow::setEnabled(bool value)
{
	ui->pushButton_library->setEnabled(value);
	ui->pushButton_todo->setEnabled(value);
	ui->pushButton_planer->setEnabled(value);
	ui->pushButton_calendar->setEnabled(value);
	ui->pushButton_stats->setEnabled(value);
	ui->pushButton_settings->setEnabled(value);
	ui->pushButton_importDecks->setEnabled(value);
	ui->pushButton_exportDecks->setEnabled(value);
	ui->pushButton_addTodo->setEnabled(value);
	ui->pushButton_deleteTodo->setEnabled(value);
	ui->pushButton_deleteAllTodos->setEnabled(value);
	ui->pushButton_addActivity->setEnabled(value);
	ui->pushButton_addEvent->setEnabled(value);
	ui->tab_browser->setEnabled(value);
	ui->tabWidget_library->setTabEnabled(1, value);
	ui->tabWidget_library->setEnabled(value);
	ui->pushButton_importDecks->setEnabled(value);
	ui->pushButton_exportDecks->setEnabled(value);
}

void MainWindow::on_pushButton_login_clicked()
{
	for (auto scene : m_plannerScenes) {
		scene->clear();
		scene->addItem(new ScheduleItem());
		scene->clearActivities();
	}
	if (!m_loggedIn) {
		QString username;
		QString password;
		LoginDialog login(this);
		if (login.exec() != QDialog::Accepted) {
			return;
		}

		username = login.getUsername();
		password = login.getPassword();

		bool loginSuccess = false;
		if (login.isRegister()) {
			if (registerUser(username, password)) {
				loginSuccess = loginUser(username, password);
				setEnabled(true);
			}
			else {
				QMessageBox::warning(this, tr("Greška pri registrovanju"),
									 tr("Korisničko ime već postoji. Probajte ponovo."));
			}
		}
		else {
			loginSuccess = loginUser(username, password);
			if (!loginSuccess) {
				QMessageBox::critical(this, tr("Greška pri prijavljivanju"),
									  tr("Neuspešno prijavljivanje. Probajte ponovo."));
				return;
			}
			else
				setEnabled(true);
		}

		m_loggedIn = loginSuccess;
	}
	else {
		// logout
		saveOnServer();
		m_loggedIn = false;
		ui->label_username->setText(tr("Nema korisnika"));
		ui->pushButton_login->setText(tr("Prijavi se"));
		setEnabled(false);
		ui->tableWidget_library->clear();
		ui->tableWidget_browser->clear();
		m_planner.deleteAll();
		m_calendar.deleteAll();
		m_toDoList.deleteAllToDos();
		ui->listWidget_todos->clear();
		m_todoLoaded = false;
		m_plannerLoaded = false;
		m_calendarLoaded = false;
		ui->stackedWidget->setCurrentIndex(LIBRARY);

		ui->tableWidget_library->setColumnCount(0);
		ui->tableWidget_browser->setColumnCount(0);
		setupTableView(ui->tableWidget_library);
		setupTableView(ui->tableWidget_browser);

		m_deckNames.clear();
		m_deckCounter = 0;

		if (ui->comboBox_deck->count() != 0)
			ui->comboBox_deck->clear();
	}
}

bool MainWindow::loginUser(const QString &username, const QString &password)
{
	setupTableView(ui->tableWidget_library);
	setupTableView(ui->tableWidget_browser);
	QJsonObject requestObject;

	requestObject["action"] = "login";
	requestObject["username"] = username;
	requestObject["password"] = password;

	QJsonDocument request(requestObject);

	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);
	if (jsonObj["status"].toString() == "Failed to connect to the server") {
		return false;
	}
	if (jsonObj["status"] != QJsonValue::Undefined && jsonObj["status"] != "Password incorrect, try again") {
		ui->label_username->setText(request["username"].toString());
		ui->pushButton_login->setText(tr("Odjavi se"));
		m_user.setUsername(request["username"].toString());
	}
	else {
		return false;
	}

	QString deckNames = jsonObj.value("decks").toString();
	m_deckCounter = 0;
	if (deckNames != "") {
		QStringList deckNamesList = deckNames.split(", ");
		for (auto &deckNameID : deckNamesList) {
			addDeckToTable(deckNameID, ui->tableWidget_library, m_deckCounter);
			auto deckNameSplit = deckNameID.split("_");
			ui->comboBox_deck->addItem(deckNameSplit[0]);

			m_deckNames.push_back(deckNameID);
		}
	}
	addCreateDeckButton();

	return true;
}

bool MainWindow::registerUser(const QString &username, const QString &password)
{
	QJsonObject requestObject;
	requestObject["action"] = "register";
	requestObject["username"] = username;
	requestObject["password"] = password;

	QJsonDocument request(requestObject);

	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);

	if (jsonObj["status"] == "Username already exists, try again") {
		return false;
	}

	return true;
}

void MainWindow::saveCalendar()
{

	QJsonObject requestObject;

	JSONSerializer serializer;
	QJsonDocument doc = serializer.createJson(m_calendar);

	requestObject["action"] = "saveCalendar";
	requestObject["username"] = m_user.username();
	requestObject["calendar"] = doc.toVariant().toJsonObject();

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);

	if (jsonObj["status"].toString() != "success") {
		QMessageBox::information(this, tr("Kalendar"),
								 tr("Došlo je do greške, kalendar nije sačuvan, probajte ponovo!"));
		return;
	}
}

void MainWindow::on_pushButton_addTodo_clicked()
{
	if (ui->lineEdit_todo->text().trimmed().isEmpty()) {
		QMessageBox::warning(this, tr("Pogrešan unos"), tr("Niste popunili polje za naziv aktivnosti!"));
		return;
	}

	auto *item = new QListWidgetItem();
	item->setCheckState(Qt::Unchecked);
	item->setText(ui->lineEdit_todo->text());

	m_toDoList.addToDo(item->text(), item->checkState());

	ui->listWidget_todos->addItem(item);
	ui->lineEdit_todo->clear();
	ui->lineEdit_todo->setFocus();
}

void MainWindow::on_pushButton_deleteTodo_clicked()
{
	int currentRow = ui->listWidget_todos->currentRow();

	if (currentRow >= 0) {
		QListWidgetItem *item = ui->listWidget_todos->takeItem(currentRow);

		m_toDoList.deleteToDo(item->text());
		delete item;
	}
}

void MainWindow::on_pushButton_deleteAllTodos_clicked()
{
	m_toDoList.deleteAllToDos();
	ui->listWidget_todos->clear();
}

void MainWindow::onTodoItemChanged(QListWidgetItem *item)
{
	if (item) {
		m_toDoList.checkToDo(item->text(), item->checkState());

		if (item->checkState() == Qt::Checked) {
			item->setBackground(QBrush(QColor(140, 255, 140)));
		}
		else {
			item->setBackground(QBrush(Qt::white));
		}
	}
}

void MainWindow::saveToDoList()
{
	QJsonObject requestObject;

	JSONSerializer serializer;
	QJsonDocument doc = serializer.createJson(m_toDoList);

	requestObject["action"] = "saveTodo";
	requestObject["username"] = m_user.username();
	requestObject["todo"] = doc.toVariant().toJsonObject();

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);

	if (jsonObj["status"].toString() != "success") {
		QMessageBox::information(this, tr("To-Do lista"),
								 tr("Došlo je do greške, todo lista nije sačuvana, probajte ponovo!"));
		return;
	}
}

void MainWindow::on_pushButton_search_clicked()
{
	ui->tableWidget_browser->clear();
	setupTableView(ui->tableWidget_browser);

	QString query = ui->lineEdit_browser->text().trimmed();

	if (query.isEmpty()) {
		QMessageBox::warning(this, tr("Pogrešan unos"), tr("Niste popunili polje za naziv špila!"));
		return;
	}

	QJsonObject requestObject;
	requestObject["action"] = "search";
	requestObject["searchQuery"] = query;

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;
	QJsonObject jsonObj = communicator.sendRequest(request);

	if (jsonObj["status"] == "No result") {
		return;
	}

	QString deckNames = jsonObj.value("decks").toString();
	if (deckNames != "") {
		QStringList deckNamesList = deckNames.split(", ");
		int browserCounter = 0;
		for (auto &deckNameID : deckNamesList) {
			addDeckToTable(deckNameID, ui->tableWidget_browser, browserCounter);
		}
	}
	else {
		QMessageBox::warning(this, tr("Nema rezultata"), tr("Nije pronadjen nijedan špil!"));
	}
}

void MainWindow::on_pushButton_importDecks_clicked()
{
	QStringList filePaths =
		QFileDialog::getOpenFileNames(nullptr, "Choose JSON Files", "", "JSON Files (*.json);;All Files (*)");

	auto rows = ui->tableWidget_library->rowCount();
	auto cols = ui->tableWidget_library->columnCount();

	for (auto i = 0; i < rows; i += 2) {
		for (auto j = 0; j < cols; j++) {
			auto cell = ui->tableWidget_library->cellWidget(i, j);
			if (auto *button = dynamic_cast<QPushButton *>(cell)) {
				for (auto it = filePaths.begin(); it != filePaths.end(); it++) {
					QStringList tempDeckName = (*it).split('/');
					if (button->text() == *(--tempDeckName.end())) {
						filePaths.removeAt(it - filePaths.begin());
						if (filePaths.isEmpty()) {
							QMessageBox::warning(this, tr("Uvoz špilova"),
												 tr("Unosite špil ili špilove koje već imate!"));
							break;
						}
					}
				}
			}
		}
	}

	for (auto &filePath : filePaths) {
		QJsonObject request;
		Deck deck;
		JSONSerializer serializer;
		serializer.load(deck, filePath);

		request["action"] = "saveDeck";
		request["username"] = m_user.username();
		QJsonDocument deckDocument = serializer.createJson(deck);
		request["deck"] = deckDocument.toVariant().toJsonObject();

		QJsonDocument requestDocument(request);
		ServerCommunicator communicator;
		QJsonObject response = communicator.sendRequest(requestDocument);
		QStringList tempDeckName = filePath.split('/');
		addDeckToTable(*(--tempDeckName.end()), ui->tableWidget_library, m_deckCounter);
		addCreateDeckButton();
		ui->comboBox_deck->addItem(deck.name());
	}
	if (!filePaths.isEmpty()) {
		QMessageBox::information(this, tr("Uvoz špilova"), tr("Uspešan uvoz!"));
	}
}

void MainWindow::on_pushButton_exportDecks_clicked()
{
	QString selectedDirectory = QFileDialog::getExistingDirectory(
		nullptr, "Select Directory", QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	auto rows = ui->tableWidget_library->rowCount();
	auto cols = ui->tableWidget_library->columnCount();

	for (auto i = 0; i < rows; i += 2) {
		for (auto j = 0; j < cols; j++) {
			auto cell = ui->tableWidget_library->cellWidget(i, j);
			if (auto *button = dynamic_cast<QPushButton *>(cell)) {
				if (!button->children().isEmpty()) {
					auto *checkbox =
						dynamic_cast<QCheckBox *>(ui->tableWidget_library->cellWidget(i, j)->children().front());
					if (checkbox->isChecked()) {

						QString deckName = button->text();
						QStringList deckNameEdited = deckName.split('.')[0].split('_');

						QJsonObject requestObject;
						requestObject["action"] = "sendDeck";
						requestObject["username"] = m_user.username();
						requestObject["DeckId"] = deckName.split('_')[1];
						requestObject["Privacy"] = "PRIVATE";

						QJsonDocument request(requestObject);
						ServerCommunicator communicator;
						QJsonObject jsonObj = communicator.sendRequest(request);
						JSONSerializer jsonSerializer;

						if (jsonObj["status"].toString() != "success") {
							QMessageBox::information(this, tr("Uvoz špilova"),
													 tr("Došlo je do greške, nije sačuvano, probajte ponovo!"));
							return;
						}

						Deck *deck = new Deck();

						QJsonObject deckObject =
							jsonObj[deckNameEdited[0].append("_").append(deckNameEdited[1]) + "_deck.json"].toObject();
						QJsonDocument deckDocument = QJsonDocument::fromVariant(deckObject.toVariantMap());
						jsonSerializer.loadJson(*deck, deckDocument);
						jsonSerializer.save(*deck, selectedDirectory + "/" + deckName);

						checkbox->setCheckState(Qt::Unchecked);
					}
				}
			}
		}
	}
	QMessageBox::information(this, tr("Izvoz špilova"), tr("Uspešan izvoz!"));
}

void MainWindow::addNewDeck(QString deckNameID)
{
	addDeckToTable(deckNameID, ui->tableWidget_library, m_deckCounter);
	addCreateDeckButton();
	ui->comboBox_deck->addItem(deckNameID.split("_")[0]);
	m_deckNames.push_back(deckNameID);
}

void MainWindow::addDeckToTable(QString deckNameID, QTableWidget *table, int &counter)
{
	auto *button = new QPushButton(deckNameID);
	button->setStyleSheet("color: transparent; margin-left: 25%;");

	if (table == ui->tableWidget_library) {
		connect(button, &QPushButton::clicked, this, &MainWindow::deckButton_clicked);
		auto *checkbox = new QCheckBox(button);
		checkbox->setStyleSheet("padding: 5%");
	}
	else
		connect(button, &QPushButton::clicked, this, &MainWindow::deckPreview_clicked);

	auto *label = new QLabel(deckNameID.split("_")[0], table);
	label->setAlignment(Qt::AlignCenter);
	label->setStyleSheet("text-align: center; margin-left: 25%");

	if (counter % 2 == 0) {
		table->setColumnCount(table->columnCount() + 1);
		table->setColumnWidth(counter / 2, 220);
	}

	table->setCellWidget(counter % 2 * 2, counter / 2, button);
	table->setCellWidget(counter % 2 * 2 + 1, counter / 2, label);
	counter++;
}

void MainWindow::addCreateDeckButton()
{
	auto *button = new QPushButton("+", ui->tableWidget_library);
	connect(button, &QPushButton::clicked, this, &MainWindow::createDeck_clicked);
	button->setStyleSheet("margin-left: 25%; font-size: 150pt; border-radius: 50px;");
	if (m_deckCounter % 2 == 0) {
		ui->tableWidget_library->setColumnCount(ui->tableWidget_library->columnCount() + 1);
		ui->tableWidget_library->setColumnWidth(m_deckCounter / 2, 220);
	}
	ui->tableWidget_library->setCellWidget(m_deckCounter % 2 * 2, m_deckCounter / 2, button);
}

void MainWindow::on_comboBox_deck_currentIndexChanged(int index) {}

void MainWindow::loadStats(DeckStats *deckStats)
{
	ui->number_timesUsed->display(static_cast<int>(deckStats->timesUsed()));
	ui->number_deckSize->display(static_cast<int>(deckStats->grades().size()));

	ui->progressBar_skip->setRange(0, deckStats->grades().size());
	ui->progressBar_bad->setRange(0, deckStats->grades().size());
	ui->progressBar_good->setRange(0, deckStats->grades().size());
	ui->progressBar_excellent->setRange(0, deckStats->grades().size());

	ui->progressBar_skip->setValue(std::count(deckStats->grades().cbegin(), deckStats->grades().cend(), 0));
	ui->progressBar_bad->setValue(std::count(deckStats->grades().cbegin(), deckStats->grades().cend(), 1));
	ui->progressBar_good->setValue(std::count(deckStats->grades().cbegin(), deckStats->grades().cend(), 2));
	ui->progressBar_excellent->setValue(std::count(deckStats->grades().cbegin(), deckStats->grades().cend(), 3));
}

void MainWindow::on_comboBox_language_currentIndexChanged(int index)
{
	auto *app = qobject_cast<QApplication *>(QApplication::instance());
	Settings &settings = Settings::instance(app);

	settings.setLanguage(index);
	QString name = ui->label_username->text();
	ui->retranslateUi(this);
	ui->label_username->setText(name);
	ui->pushButton_login->setText(tr("Odjavi se"));
}

void MainWindow::on_comboBox_theme_currentIndexChanged(int index)
{
	auto *app = qobject_cast<QApplication *>(QApplication::instance());
	Settings &settings = Settings::instance(app);

	settings.setTheme(index);
	QString name = ui->label_username->text();
	ui->retranslateUi(this);
	ui->label_username->setText(name);
	ui->pushButton_login->setText(tr("Odjavi se"));
}

void MainWindow::on_comboBox_deck_activated(int index)
{
	if (index >= m_deckNames.size())
		return;

	QJsonObject requestObject;

	requestObject["action"] = "getStats";
	requestObject["username"] = m_user.username();
	requestObject["DeckId"] = m_deckNames[index].split("_")[1];

	QJsonDocument request(requestObject);
	ServerCommunicator communicator;
	QJsonObject statsObject = communicator.sendRequest(request);
	QJsonDocument statsDocument = QJsonDocument::fromVariant(statsObject.toVariantMap());
	JSONSerializer jsonSerializer;

	if (statsObject["status"].toString() != "no stats") {
		JSONSerializer jsonSerializer;
		auto deckStats = new DeckStats();
		jsonSerializer.loadJson(*deckStats, statsDocument);
		loadStats(deckStats);
		delete deckStats;
	}
}
