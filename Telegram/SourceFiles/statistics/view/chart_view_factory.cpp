/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "statistics/view/chart_view_factory.h"

#include "statistics/statistics_common.h"
#include "statistics/view/linear_chart_view.h"

namespace Statistic {

std::unique_ptr<AbstractChartView> CreateChartView(ChartViewType type) {
	switch (type) {
	case ChartViewType::Linear: {
		return std::make_unique<LinearChartView>();
	} break;
	default: Unexpected("Type in Statistic::CreateChartView.");
	}
}

} // namespace Statistic