#pragma once

#include <memory>

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QProgressBar;
class QPushButton;
class QSpinBox;

class GuiWindow final : public QWidget {
public:
	explicit GuiWindow(QWidget* parent = nullptr);
	~GuiWindow() override;

private:
	void StartGeneration();
	void UpdateSourceModeUi();
	void SetBusy(bool busy);
	void AppendLog(const QString& message);
	QString ResolveCliExecutable() const;

	QComboBox* source_mode_input_;
	QLineEdit* username_input_;
	QLineEdit* skin_file_input_;
	QPushButton* browse_skin_button_;
	QSpinBox* slot_input_;
	QComboBox* arm_override_input_;
	QLineEdit* output_dir_input_;
	QPushButton* browse_output_button_;
	QPushButton* open_output_button_;
	QPushButton* generate_button_;
	QProgressBar* progress_indicator_;
	QPlainTextEdit* log_output_;

	std::unique_ptr<QProcess> process_;
};
