#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTTRANSMISSIONCALC_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTTRANSMISSIONCALC_H_

#include "ui_IndirectTransmissionCalc.h"
#include "MantidQtCustomInterfaces/IndirectToolsTab.h"
#include "MantidAPI/ExperimentInfo.h"

#include <QComboBox>
#include <QMap>
#include <QStringList>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class DLLExport IndirectTransmissionCalc : public IndirectToolsTab
		{
			Q_OBJECT

		public:
			IndirectTransmissionCalc(QWidget * parent = 0);

			// Inherited methods from IndirectToolsTab
			QString help() { return "Transmission"; };

			bool validate();
			void run();

			/// Load default settings into the interface
			void loadSettings(const QSettings& settings);

		private:
			/// The ui form
			Ui::IndirectTransmissionCalc m_uiForm;

		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif
