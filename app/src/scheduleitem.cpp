#include "../lib/scheduleitem.h"
#include <QPainter>

ScheduleItem::ScheduleItem() = default;

QRectF ScheduleItem::boundingRect() const
{
	return {0, 0, m_width, height()};
}

void ScheduleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	for (auto i = 1; i <= 24; i++) {
		painter->setPen(QPen(QColor::fromRgb(0, 0, 0)));
		painter->drawLine(0, i * height() / 24, m_width, i * height() / 24);

		QString hours = "";
		if (i <= 10)
			hours = "0";
		hours += QString::number(i - 1) + ":00";
		QPointF textPosition(0, (i - 1) * height() / 24 + painter->font().pointSize() + 2);
		painter->drawText(textPosition, hours);

		painter->setPen(QPen(QColor::fromRgb(200, 200, 200)));
		painter->drawLine(0, i * height() / 24 - height() / 48, m_width, i * height() / 24 - height() / 48);
	}
}

void ScheduleItem::setWidth(unsigned int width)
{
	m_width = width;
}
