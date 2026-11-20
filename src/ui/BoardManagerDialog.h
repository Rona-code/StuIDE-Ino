#pragma once

#include "../core/cli/CoreManager.h"
#include <QDialog>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QLabel>

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QScrollArea;
class CoreManager;

class BoardManagerDialog : public QDialog {
	Q_OBJECT

public:
	explicit BoardManagerDialog(CoreManager* coreManager, QWidget* parent = nullptr);

	~BoardManagerDialog();

signals:
	void coresChanged();

private slots:
	void applyFilters();
	void onInstallRequested(const QString& coreId, const QString& version);
	void onRemoveRequested(const QString& coreId);

private:
	void setupUi();
	void rebuildList(const QVector<CorePackageInfo>& items);
	void setStatusText(const QString& text, bool isError = false);

	CoreManager* m_coreManager = nullptr;
	QVector<CorePackageInfo> m_allCores;

	QLineEdit* m_searchEdit = nullptr;
	QComboBox* m_typeCombo = nullptr;
	QScrollArea* m_scrollArea = nullptr;
	QWidget* m_listContainer = nullptr;
	QVBoxLayout* m_listLayout = nullptr;
	QLabel* m_statusLabel = nullptr;

	bool m_busy = false;
};